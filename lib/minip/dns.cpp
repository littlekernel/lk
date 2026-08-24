/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/* A small synchronous DNS resolver: one question at a time, A records only,
 * with a tiny TTL cache in front of it.
 *
 * The message builder and parser are pure functions over byte buffers and
 * are declared in minip-internal.h so the unit tests can drive them with
 * canned packets. They keep no state of their own and allocate nothing: a
 * reply is parsed on the stack worker's thread, inside the udp callback, so
 * every buffer here has to be either tiny or static.
 */

#include "minip-internal.h"

#include <ctype.h>
#include <endian.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOCAL_TRACE 0

namespace {

constexpr uint16_t DNS_PORT = 53;

constexpr uint16_t DNS_FLAG_QR = 0x8000;    // set on responses
constexpr uint16_t DNS_FLAG_TC = 0x0200;    // truncated
constexpr uint16_t DNS_FLAG_RD = 0x0100;    // recursion desired
constexpr uint16_t DNS_RCODE_MASK = 0x000f;
constexpr uint16_t DNS_RCODE_NXDOMAIN = 3;

constexpr size_t DNS_MAX_NAME = 255;        // encoded, including length bytes
constexpr size_t DNS_MAX_LABEL = 63;

/* Bounds the work a single name walk may do. A legal name is 255 bytes of
 * labels; pointers do not add to it, they only redirect. Any packet that
 * needs more steps than this is malformed or hostile.
 */
constexpr int DNS_WALK_BUDGET = 256;

struct dns_hdr {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __PACKED;

/* Walks the labels of a (possibly compressed) name. Compression pointers
 * must point strictly backwards, and the step budget bounds the walk even
 * for a chain of pointers that individually obey that rule.
 */
struct name_walker {
    const uint8_t *buf;
    size_t len;
    size_t pos;
    int budget;
};

void walker_init(name_walker *w, const uint8_t *buf, size_t len, size_t offset) {
    w->buf = buf;
    w->len = len;
    w->pos = offset;
    w->budget = DNS_WALK_BUDGET;
}

/* returns the label length and points *out at it, 0 at the end of the name,
 * or a negative error for a malformed one
 */
int walker_next(name_walker *w, const uint8_t **out) {
    for (;;) {
        if (w->budget-- <= 0) {
            return ERR_NOT_VALID;
        }
        if (w->pos >= w->len) {
            return ERR_NOT_VALID;
        }

        uint8_t l = w->buf[w->pos];
        if ((l & 0xc0) == 0xc0) {
            if (w->pos + 1 >= w->len) {
                return ERR_NOT_VALID;
            }
            size_t target = ((size_t)(l & 0x3f) << 8) | w->buf[w->pos + 1];
            if (target >= w->pos) {
                return ERR_NOT_VALID;   // must point strictly backwards
            }
            w->pos = target;
            continue;
        }
        if (l & 0xc0) {
            return ERR_NOT_VALID;       // reserved label type
        }
        if (l == 0) {
            w->pos++;
            return 0;
        }
        if (w->pos + 1 + l > w->len) {
            return ERR_NOT_VALID;
        }

        *out = w->buf + w->pos + 1;
        w->pos += 1 + l;
        return l;
    }
}

/* Offset of the byte just past a name in the record stream. Unlike the
 * walker this does not follow pointers: a compressed name occupies exactly
 * the two bytes of the pointer.
 */
ssize_t dns_skip_name(const uint8_t *buf, size_t len, size_t pos) {
    while (pos < len) {
        uint8_t l = buf[pos];
        if ((l & 0xc0) == 0xc0) {
            return (pos + 2 <= len) ? (ssize_t)(pos + 2) : ERR_NOT_VALID;
        }
        if (l & 0xc0) {
            return ERR_NOT_VALID;
        }
        if (l == 0) {
            return (ssize_t)(pos + 1);
        }
        pos += 1 + (size_t)l;
    }
    return ERR_NOT_VALID;
}

/* compare an encoded name against a dotted string, case insensitively */
bool dns_name_matches(const uint8_t *buf, size_t len, size_t offset, const char *name) {
    name_walker w;
    walker_init(&w, buf, len, offset);

    const char *p = name;
    for (;;) {
        const uint8_t *label = NULL;
        int l = walker_next(&w, &label);
        if (l < 0) {
            return false;
        }
        if (l == 0) {
            // the string must have ended too; a single trailing dot is fine
            return (p[0] == '\0') || (p[0] == '.' && p[1] == '\0');
        }

        for (int i = 0; i < l; i++) {
            if (*p == '\0' || tolower(*p) != tolower(label[i])) {
                return false;
            }
            p++;
        }
        if (*p == '.') {
            p++;
        } else if (*p != '\0') {
            return false;
        }
    }
}

/* compare two encoded names within the same message, case insensitively */
bool dns_names_equal(const uint8_t *buf, size_t len, size_t a, size_t b) {
    name_walker wa, wb;
    walker_init(&wa, buf, len, a);
    walker_init(&wb, buf, len, b);

    for (;;) {
        const uint8_t *la = NULL, *lb = NULL;
        int ra = walker_next(&wa, &la);
        int rb = walker_next(&wb, &lb);
        if (ra < 0 || rb < 0 || ra != rb) {
            return false;
        }
        if (ra == 0) {
            return true;
        }
        for (int i = 0; i < ra; i++) {
            if (tolower(la[i]) != tolower(lb[i])) {
                return false;
            }
        }
    }
}

uint16_t read_u16(const uint8_t *buf) {
    uint16_t val;
    memcpy(&val, buf, sizeof(val));
    return ntohs(val);
}

uint32_t read_u32(const uint8_t *buf) {
    uint32_t val;
    memcpy(&val, buf, sizeof(val));
    return ntohl(val);
}

} // anonymous namespace

/* Build a standard recursive query for one name. Returns the length of the
 * message or a negative error.
 */
ssize_t dns_build_query(void *_buf, size_t buflen, uint16_t id, const char *name, uint16_t type) {
    uint8_t *buf = (uint8_t *)_buf;

    if (!buf || !name) {
        return ERR_INVALID_ARGS;
    }
    if (buflen < sizeof(dns_hdr)) {
        return ERR_NOT_ENOUGH_BUFFER;
    }

    dns_hdr hdr = {};
    hdr.id = htons(id);
    hdr.flags = htons(DNS_FLAG_RD);
    hdr.qdcount = htons(1);
    memcpy(buf, &hdr, sizeof(hdr));

    size_t pos = sizeof(hdr);
    size_t encoded = 0;

    for (const char *label = name;;) {
        const char *dot = strchr(label, '.');
        size_t label_len = dot ? (size_t)(dot - label) : strlen(label);

        if (label_len == 0 || label_len > DNS_MAX_LABEL) {
            return ERR_NOT_VALID;
        }
        encoded += label_len + 1;
        if (encoded > DNS_MAX_NAME) {
            return ERR_NOT_VALID;
        }
        if (pos + 1 + label_len > buflen) {
            return ERR_NOT_ENOUGH_BUFFER;
        }

        buf[pos++] = (uint8_t)label_len;
        memcpy(buf + pos, label, label_len);
        pos += label_len;

        if (!dot || dot[1] == '\0') {
            break;      // end of the string, or a single trailing dot
        }
        label = dot + 1;
    }

    if (pos + 5 > buflen) {
        return ERR_NOT_ENOUGH_BUFFER;
    }
    buf[pos++] = 0;     // root label

    uint16_t val = htons(type);
    memcpy(buf + pos, &val, sizeof(val));
    pos += sizeof(val);
    val = htons(DNS_CLASS_IN);
    memcpy(buf + pos, &val, sizeof(val));
    pos += sizeof(val);

    return (ssize_t)pos;
}

/* Pick the answer to our question out of a response.
 *
 * Returns NO_ERROR with *out filled in, ERR_NOT_FOUND if the server
 * answered but has no address for the name, and ERR_NOT_VALID if the
 * message is malformed or is not a response to this query at all -- the
 * resolver keeps waiting on that one rather than failing the lookup.
 */
status_t dns_parse_response(const void *_buf, size_t len, uint16_t id, const char *name,
                            dns_record_t *out) {
    const uint8_t *buf = (const uint8_t *)_buf;

    if (!buf || !name || !out) {
        return ERR_INVALID_ARGS;
    }
    if (len < sizeof(dns_hdr)) {
        return ERR_NOT_VALID;
    }

    dns_hdr hdr;
    memcpy(&hdr, buf, sizeof(hdr));

    if (ntohs(hdr.id) != id) {
        return ERR_NOT_VALID;           // someone else's transaction
    }
    uint16_t flags = ntohs(hdr.flags);
    if ((flags & DNS_FLAG_QR) == 0) {
        return ERR_NOT_VALID;           // a query, not a response
    }
    if (ntohs(hdr.qdcount) != 1) {
        return ERR_NOT_VALID;
    }

    /* the question must be the one we asked, name and all: the id alone is
     * 16 bits of not very much
     */
    ssize_t pos = dns_skip_name(buf, len, sizeof(hdr));
    if (pos < 0 || (size_t)pos + 4 > len) {
        return ERR_NOT_VALID;
    }
    if (!dns_name_matches(buf, len, sizeof(hdr), name)) {
        return ERR_NOT_VALID;
    }
    if (read_u16(buf + pos) != DNS_TYPE_A || read_u16(buf + pos + 2) != DNS_CLASS_IN) {
        return ERR_NOT_VALID;
    }
    pos += 4;

    uint16_t rcode = flags & DNS_RCODE_MASK;
    if (rcode == DNS_RCODE_NXDOMAIN) {
        return ERR_NOT_FOUND;
    }
    if (rcode != 0) {
        return ERR_IO;
    }

    /* The name being chased, as an offset into the message. It starts at the
     * question and moves to the alias every time a CNAME for it turns up,
     * which keeps the whole walk free of name buffers.
     */
    size_t target = sizeof(hdr);

    uint16_t ancount = ntohs(hdr.ancount);
    for (uint16_t i = 0; i < ancount; i++) {
        size_t rname = (size_t)pos;

        pos = dns_skip_name(buf, len, rname);
        if (pos < 0 || (size_t)pos + 10 > len) {
            return ERR_NOT_VALID;
        }

        uint16_t type = read_u16(buf + pos);
        uint16_t cls = read_u16(buf + pos + 2);
        uint32_t ttl = read_u32(buf + pos + 4);
        uint16_t rdlen = read_u16(buf + pos + 8);
        pos += 10;

        if ((size_t)pos + rdlen > len) {
            return ERR_NOT_VALID;
        }

        if (cls == DNS_CLASS_IN && dns_names_equal(buf, len, rname, target)) {
            if (type == DNS_TYPE_A && rdlen == 4) {
                out->type = type;
                out->ttl = ttl;
                memcpy(&out->addr, buf + pos, sizeof(out->addr));
                return NO_ERROR;
            }
            if (type == DNS_TYPE_CNAME) {
                target = (size_t)pos;   // chase the alias
            }
        }

        pos += rdlen;
    }

    return (flags & DNS_FLAG_TC) ? ERR_NOT_VALID : ERR_NOT_FOUND;
}

/* --- the resolver --- */

namespace {

constexpr lk_time_t DNS_FIRST_TIMEOUT = 500;
constexpr lk_time_t DNS_MAX_ATTEMPT_TIMEOUT = 2000;
constexpr int DNS_MAX_ATTEMPTS = 3;
constexpr size_t DNS_QUERY_MAX = DNS_MAX_NAME + sizeof(dns_hdr) + 5;

/* The query in flight. One at a time: dns_lock serializes resolvers, so the
 * state and its buffers can be static, which keeps a 300 byte query buffer
 * off the caller's stack. The udp callback never takes dns_lock -- it runs
 * on the stack worker, and blocking there behind a resolver that is asleep
 * waiting for a reply would stop the reply from ever being delivered.
 */
struct {
    bool active;
    uint32_t gen;               // bumped per query; a late callback checks it
    uint16_t id;
    ipv4_addr_t server;
    char name[DNS_MAX_NAME + 1];

    bool have_result;
    status_t result;
    dns_record_t record;

    size_t query_len;
    uint8_t query[DNS_QUERY_MAX];
} query_state;

mutex_t dns_lock = MUTEX_INITIAL_VALUE(dns_lock);
spin_lock_t dns_state_lock = SPIN_LOCK_INITIAL_VALUE;
event_t dns_event = EVENT_INITIAL_VALUE(dns_event, false, EVENT_FLAG_AUTOUNSIGNAL);

/* --- the cache --- */

/* The cache is pure .bss, so keep it small where memory is: a name too long
 * for an entry is simply not cached, never truncated into one.
 */
#if LK_EMBEDDED
constexpr size_t DNS_CACHE_SIZE = 2;
constexpr size_t DNS_CACHE_NAME_MAX = 32;
#else
constexpr size_t DNS_CACHE_SIZE = 8;
constexpr size_t DNS_CACHE_NAME_MAX = 64;
#endif
constexpr uint32_t DNS_MAX_TTL_SECS = 3600;

struct dns_cache_entry {
    bool valid;
    char name[DNS_CACHE_NAME_MAX];
    ipv4_addr_t addr;
    lk_time_t expires;
};

dns_cache_entry dns_cache[DNS_CACHE_SIZE];
size_t dns_cache_next;
mutex_t dns_cache_lock = MUTEX_INITIAL_VALUE(dns_cache_lock);

bool dns_cache_lookup(const char *name, ipv4_addr_t *out) {
    bool found = false;

    mutex_acquire(&dns_cache_lock);
    for (size_t i = 0; i < countof(dns_cache); i++) {
        dns_cache_entry *e = &dns_cache[i];
        if (!e->valid) {
            continue;
        }
        if (TIME_GTE(current_time(), e->expires)) {
            e->valid = false;
            continue;
        }
        if (strcasecmp(e->name, name) == 0) {
            *out = e->addr;
            found = true;
            break;
        }
    }
    mutex_release(&dns_cache_lock);

    return found;
}

void dns_cache_add(const char *name, ipv4_addr_t addr, uint32_t ttl) {
    if (ttl == 0 || strlen(name) >= DNS_CACHE_NAME_MAX) {
        // a truncated key would compare equal to the wrong name, so skip it
        return;
    }
    if (ttl > DNS_MAX_TTL_SECS) {
        ttl = DNS_MAX_TTL_SECS;
    }

    mutex_acquire(&dns_cache_lock);

    /* reuse the entry for this name, or a dead one, before evicting */
    dns_cache_entry *slot = NULL;
    for (size_t i = 0; i < countof(dns_cache); i++) {
        dns_cache_entry *e = &dns_cache[i];
        if (!e->valid || TIME_GTE(current_time(), e->expires)) {
            if (!slot) {
                slot = e;
            }
        } else if (strcasecmp(e->name, name) == 0) {
            slot = e;
            break;
        }
    }
    if (!slot) {
        slot = &dns_cache[dns_cache_next];
        dns_cache_next = (dns_cache_next + 1) % countof(dns_cache);
    }

    strlcpy(slot->name, name, sizeof(slot->name));
    slot->addr = addr;
    slot->expires = current_time() + ttl * 1000;
    slot->valid = true;

    mutex_release(&dns_cache_lock);
}

/* Runs on the stack worker. Parses the reply here rather than handing the
 * buffer back to the waiter: it belongs to a pktbuf that is freed as soon
 * as this returns.
 */
void dns_udp_callback(void *data, size_t len, uint32_t srcaddr, uint16_t srcport, void *arg) {
    uint16_t id;
    uint32_t gen;

    {
        AutoSpinLock guard(&dns_state_lock);
        if (!query_state.active || query_state.have_result) {
            return;
        }
        if (srcaddr != query_state.server || srcport != DNS_PORT) {
            return;
        }
        id = query_state.id;
        gen = query_state.gen;
    }

    /* The name buffer is static, so parsing it outside the lock is safe even
     * if this query has since been abandoned; the generation check below
     * throws the result away in that case.
     */
    dns_record_t record = {};
    status_t err = dns_parse_response(data, len, id, query_state.name, &record);
    if (err == ERR_NOT_VALID) {
        LTRACEF("ignoring malformed or unrelated response\n");
        return;     // keep waiting; a retry may still be answered
    }

    AutoSpinLock guard(&dns_state_lock);
    if (!query_state.active || query_state.have_result || query_state.gen != gen) {
        return;
    }
    query_state.result = err;
    query_state.record = record;
    query_state.have_result = true;
    guard.release();

    event_signal(&dns_event, true);
}

uint16_t dns_random_port(void) {
    return (uint16_t)(49152 + (unsigned int)rand() % 16384);
}

} // anonymous namespace

void dns_cache_flush(void) {
    mutex_acquire(&dns_cache_lock);
    memset(dns_cache, 0, sizeof(dns_cache));
    dns_cache_next = 0;
    mutex_release(&dns_cache_lock);
}

status_t dns_resolve(const char *name, ipv4_addr_t *out, lk_time_t timeout) {
    if (!name || !out || name[0] == '\0') {
        return ERR_INVALID_ARGS;
    }

    /* The reply is delivered by the stack worker, so waiting for one from
     * inside it would wait forever.
     */
    DEBUG_ASSERT(!netstack_is_stack_thread());

    if (dns_cache_lookup(name, out)) {
        return NO_ERROR;
    }

    ipv4_addr_t server = minip_get_dns_server();
    if (server == IPV4_NONE) {
        return ERR_NOT_CONFIGURED;
    }

    mutex_acquire(&dns_lock);

    /* another resolver may have looked this up while we waited for the lock */
    if (dns_cache_lookup(name, out)) {
        mutex_release(&dns_lock);
        return NO_ERROR;
    }

    uint16_t id = (uint16_t)((unsigned int)rand() >> 8);
    ssize_t query_len = dns_build_query(query_state.query, sizeof(query_state.query), id,
                                        name, DNS_TYPE_A);
    if (query_len < 0) {
        mutex_release(&dns_lock);
        return (status_t)query_len;
    }

    /* an ephemeral source port, retried in the unlikely event it is taken */
    uint16_t port = 0;
    for (int i = 0; i < 4; i++) {
        port = dns_random_port();
        if (udp_listen(port, dns_udp_callback, NULL) == 0) {
            break;
        }
        port = 0;
    }
    if (port == 0) {
        mutex_release(&dns_lock);
        return ERR_NO_RESOURCES;
    }

    udp_socket_t *sock = NULL;
    status_t err = udp_open(server, port, DNS_PORT, &sock);
    if (err < 0) {
        udp_listen(port, NULL, NULL);
        mutex_release(&dns_lock);
        return err;
    }

    query_state.id = id;
    query_state.server = server;
    strlcpy(query_state.name, name, sizeof(query_state.name));
    query_state.query_len = (size_t)query_len;
    query_state.have_result = false;
    query_state.result = ERR_TIMED_OUT;
    event_unsignal(&dns_event);
    {
        AutoSpinLock guard(&dns_state_lock);
        query_state.gen++;
        query_state.active = true;
    }

    const lk_time_t deadline = (timeout == INFINITE_TIME) ? 0 : current_time() + timeout;
    lk_time_t attempt_timeout = DNS_FIRST_TIMEOUT;

    err = ERR_TIMED_OUT;
    uint32_t ttl = 0;
    for (int i = 0; i < DNS_MAX_ATTEMPTS; i++) {
        lk_time_t wait = MIN(attempt_timeout, DNS_MAX_ATTEMPT_TIMEOUT);
        if (timeout != INFINITE_TIME) {
            if (TIME_GTE(current_time(), deadline)) {
                break;
            }
            lk_time_t left = deadline - current_time();
            if (wait > left) {
                wait = left;
            }
        }

        status_t send_err = udp_send(query_state.query, query_state.query_len, sock);
        if (send_err < 0) {
            LTRACEF("udp_send returned %d\n", send_err);
        }

        event_wait_timeout(&dns_event, wait);

        AutoSpinLock guard(&dns_state_lock);
        if (query_state.have_result) {
            err = query_state.result;
            if (err == NO_ERROR) {
                *out = query_state.record.addr;
                ttl = query_state.record.ttl;
            }
            break;
        }
        guard.release();

        attempt_timeout *= 2;
    }

    {
        AutoSpinLock guard(&dns_state_lock);
        query_state.active = false;
    }

    udp_listen(port, NULL, NULL);
    udp_close(sock);

    mutex_release(&dns_lock);

    if (err == NO_ERROR) {
        dns_cache_add(name, *out, ttl);
    }

    return err;
}

status_t minip_resolve(const char *host, ipv4_addr_t *out) {
    if (!host || !out) {
        return ERR_INVALID_ARGS;
    }

    if (minip_parse_ipaddr_checked(host, strlen(host), out) == NO_ERROR) {
        return NO_ERROR;
    }

    return dns_resolve(host, out, DNS_DEFAULT_TIMEOUT);
}
