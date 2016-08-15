/*
 * Copyright (c) 2015 Carlos Pizano-Uribe <cpu@chromium.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <err.h>
#include <trace.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <list.h>
#include <compiler.h>
#include <endian.h>
#include <stdbool.h>
#include <lib/minip.h>
#include <platform.h>

#include <lib/tftp.h>

#define LOCAL_TRACE 0

// TFTP Opcodes:
#define TFTP_OPCODE_RRQ   1UL
#define TFTP_OPCODE_WRQ   2UL
#define TFTP_OPCODE_DATA  3UL
#define TFTP_OPCODE_ACK   4UL
#define TFTP_OPCODE_ERROR 5UL

// TFTP Errors:
#define TFTP_ERROR_UNDEF        0UL
#define TFTP_ERROR_NOT_FOUND    1UL
#define TFTP_ERROR_ACCESS       2UL
#define TFTP_ERROR_FULL         3UL
#define TFTP_ERROR_ILLEGAL_OP   4UL
#define TFTP_ERROR_UNKNOWN_XFER 4UL
#define TFTP_ERROR_EXISTS       6UL
#define TFTP_ERROR_NO_SUCH_USER 7UL

#define TFTP_PORT 69

#define RD_U16(ptr) \
    (uint16_t)(((uint16_t)*((uint8_t*)(ptr)+1)<<8)|(uint16_t)*(uint8_t*)(ptr))

static struct list_node tftp_list = LIST_INITIAL_VALUE(tftp_list);

// Represents tftp jobs and clients of them. If |socket| is not null the
// job is in progress and members below it are valid.
typedef struct {
    struct list_node list;
    // Registration info.
    const char *file_name;
    tftp_callback_t callback;
    void *arg;
    // Current job info.
    udp_socket_t *socket;
    uint32_t src_addr;
    uint16_t src_port;
    uint16_t listen_port;
    uint16_t pkt_count;
} tftp_job_t;

uint16_t next_port = 2224;

static void send_ack(udp_socket_t *socket, uint16_t count)
{
    // Packet is [4][count].
    status_t st;
    uint16_t ack[] = {htons(TFTP_OPCODE_ACK), htons(count)};
    st = udp_send(ack, sizeof(ack), socket);
    if (st < 0) {
        LTRACEF("send_ack failed: %d\n", st);
    }
}

static void send_error(udp_socket_t *socket, uint16_t code)
{
    // Packet is [5][error code][error in ascii-string][0].
    status_t st;
    uint16_t ncode = htons(code);
    uint16_t err[] = { htons(TFTP_OPCODE_ERROR), ncode,
                       0x7245, 0x2072, 0x3030 + ncode, 0
                     };
    st = udp_send(err, sizeof(err), socket);
    if (st < 0) {
        LTRACEF("send_err failed: %d\n", st);
    }
}

static void end_transfer(tftp_job_t *job, bool do_callback)
{
    udp_listen(job->listen_port, NULL, NULL);
    udp_close(job->socket);
    job->socket = NULL;
    job->src_addr = 0UL;
    if (do_callback) {
        job->callback(NULL, 0UL, job->arg);
    }
}

static void udp_wrq_callback(void *data, size_t len,
                             uint32_t srcaddr, uint16_t srcport,
                             void *arg)
{
    // Packet is [3][count][data]. All packets but the last have 512
    // bytes of data, including zero data.
    char *data_c = data;
    tftp_job_t *job = arg;
    job->pkt_count++;

    if (len < 4) {
        // Not to spec. Ignore.
        return;
    }

    if (!job->socket) {
        // It is possible to have the client sent another packet
        // after we called end_transfer().
        return;
    }

    if ((srcaddr != job->src_addr) || (srcport != job->src_port)) {
        LTRACEF("invalid source\n");
        send_error(job->socket, TFTP_ERROR_UNKNOWN_XFER);
        end_transfer(job, true);
        return;
    }

    if (RD_U16(data_c) != htons(TFTP_OPCODE_DATA)) {
        LTRACEF("invalid opcode\n");
        send_error(job->socket, TFTP_ERROR_ILLEGAL_OP);
        end_transfer(job, true);
        return;
    }

    send_ack(job->socket, job->pkt_count);

    if (job->callback(&data_c[4], len - 4, job->arg) < 0) {
        // The client wants to abort.
        send_error(job->socket, TFTP_ERROR_FULL);
        end_transfer(job, true);
    }

    // 512 bytes payload plus 4 of fixed header. The last packet.
    // has always less than 512 bytes of payload.
    if (len != 516) {
        end_transfer(job, true);
    }
}

static tftp_job_t *get_job_by_name(const char *file_name)
{
    DEBUG_ASSERT(file_name);
    tftp_job_t *entry;
    list_for_every_entry(&tftp_list, entry, tftp_job_t, list) {
        if (strcmp(entry->file_name, file_name) == 0) {
            return entry;
        }
    }
    return NULL;
}

static void udp_svc_callback(void *data, size_t len,
                             uint32_t srcaddr, uint16_t srcport,
                             void *arg)
{
    status_t st;
    uint16_t opcode;
    udp_socket_t *socket;
    tftp_job_t *job;

    st = udp_open(srcaddr, next_port, srcport, &socket);
    if (st < 0) {
        LTRACEF("error opening send socket %d\n", st);
        return;
    }

    opcode = ntohs(RD_U16(data));

    if (opcode != TFTP_OPCODE_WRQ) {
        // Operation not supported.
        LTRACEF("op not supported, opcode: %d\n", opcode);
        send_error(socket, TFTP_ERROR_ACCESS);
        udp_close(socket);
        return;
    }

    // Look for a client that can hadle the file. TODO: |data|
    // needs to be null terminated. A malicious client can crash us.
    job = get_job_by_name(((char *)data) + 2);

    if (!job) {
        // Nobody claims to handle that file.
        LTRACEF("no client registered for file\n");
        send_error(socket, TFTP_ERROR_UNKNOWN_XFER);
        udp_close(socket);
        return;
    }

    if (job->socket) {
        // There is already an ongoing job.
        // TODO: garbage collect the existing one if too long since the
        // last packet was processed.
        LTRACEF("existing job in progress\n");
        send_error(socket, TFTP_ERROR_EXISTS);
        udp_close(socket);
        return;
    }

    LTRACEF("write op accepted, port %d\n", srcport);
    // Request accepted. The rest of the transfer happens between
    // next_port <----> srcport via udp_wrq_callback().

    job->socket = socket;
    job->src_addr = srcaddr;
    job->src_port = srcport;
    job->pkt_count = 0UL;
    job->listen_port = next_port;

    st = udp_listen(job->listen_port, &udp_wrq_callback, job);
    if (st < 0) {
        LTRACEF("error listening on port\n");
        return;
    }

    send_ack(socket, 0UL);
    next_port++;
}

int tftp_set_write_client(const char *file_name, tftp_callback_t cb, void *arg)
{
    DEBUG_ASSERT(file_name);
    DEBUG_ASSERT(cb);

    tftp_job_t *job;

    list_for_every_entry(&tftp_list, job, tftp_job_t, list) {
        if (strcmp(file_name, job->file_name) == 0) {
            list_delete(&job->list);
            if (job->socket) {
                // There is a job in progress. It will be cancelled silently.
                end_transfer(job, false);
            }
            return 0;
        }
    }

    if ((job = malloc(sizeof(tftp_job_t))) == NULL) {
        return -1;
    }

    memset(job, 0, sizeof(tftp_job_t));
    job->file_name = file_name;
    job->callback = cb;
    job->arg = arg;

    list_add_tail(&tftp_list, &job->list);
    return 0;
}

int tftp_server_init(void *arg)
{
    status_t st = udp_listen(TFTP_PORT, &udp_svc_callback, 0);
    return st;
}

