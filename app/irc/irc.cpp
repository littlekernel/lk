/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <app.h>
#include <kernel/mutex.h>
#include <kernel/thread.h>
#include <lib/minip.h>
#include <lib/version.h>
#include <lk/cpp.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdio.h>
#include <string.h>

#define LOCAL_TRACE 0

namespace {

// constexpr IRC_SERVER = IPV4(176,58,122,119) // irc.libera.chat
constexpr ipv4_addr_t IRC_SERVER = IPV4(88, 99, 244, 4); // irc.sortix.org
constexpr uint16_t IRC_PORT = 6667;
constexpr const char *IRC_USER = "geist";
constexpr const char *IRC_NICK = "geist-lk";
constexpr const char *IRC_CHAN = "#sortix";

class irc_client {
  public:
    irc_client() = default;
    ~irc_client() = default;

    void init();
    void shutdown();
    status_t connect();
    status_t handshake();
    status_t read_loop();
    status_t console_input_line(const char *line, bool &exit);

    void set_server(uint32_t server) { server_ip_ = server; }
    void set_server_port(uint16_t port) { server_port_ = port; }

  private:
    DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(irc_client);

    mutex_t lock_ = MUTEX_INITIAL_VALUE(lock_);
    tcp_socket_t *sock_ = nullptr;

    uint32_t server_ip_ = 0;
    uint16_t server_port_ = 0;
    enum class state : uint8_t {
        INITIAL,
        CONNECTED,
        HANDSHOOK,
    } state_ = state::INITIAL;

    void handle_ctcp_request(const char *nick, const char *ctcp_cmd);
};

void irc_client::init() {}

void irc_client::shutdown() {
    AutoLock al(&lock_);

    if (sock_) {
        if (state_ == state::HANDSHOOK) {
            // send quit
            tcp_write(sock_, "QUIT\r\n", strlen("QUIT\r\n"));
            // XXX flush socket
            thread_sleep(1000);
        }
        tcp_close(sock_);
        sock_ = nullptr;
    }
}

void irc_client::handle_ctcp_request(const char *nick, const char *ctcp_cmd) {
    char buf[256];

    // Handle VERSION request
    if (strncmp(ctcp_cmd, "VERSION", strlen("VERSION")) == 0) {
        snprintf(buf, sizeof(buf), "NOTICE %s :\001VERSION LK %s (%s) littlekernel\001\r\n", nick,
                 lk_version.buildid, lk_version.arch);
        tcp_write(sock_, buf, strlen(buf));
    }
    // Handle TIME request
    else if (strncmp(ctcp_cmd, "TIME", strlen("TIME")) == 0) {
        snprintf(buf, sizeof(buf), "NOTICE %s :\001TIME Wed Jan 01 00:00:00 2020\001\r\n", nick);
        tcp_write(sock_, buf, strlen(buf));
    }
}

status_t irc_client::connect() {
    AutoLock al(&lock_);

    if (server_ip_ == 0 || server_port_ == 0) {
        return ERR_NOT_CONFIGURED;
    }

    auto err = tcp_connect(&sock_, server_ip_, server_port_);
    if (err < 0) {
        printf("err %d connecting to server\n", err);
        return ERR_CHANNEL_CLOSED; // TODO: better one
    }

    state_ = state::CONNECTED;

    return NO_ERROR;
}

status_t irc_client::read_loop() {
    char line[1024];
    int pos = 0;
    for (;;) {
        char c;
        auto r = tcp_read(sock_, &c, sizeof(c));
        if (r < 0) {
            return static_cast<status_t>(r);
        }

        if (r == 0) {
            TRACEF("tcp_read returns 0?\n");
            return ERR_GENERIC;
        }

        // TODO: make sure we dont overwrite the line

        // append the char to our accumulated line
        if (c == '\r') {
            // consume \r
            continue;
        }

        // store the char
        line[pos++] = c;

        if (c != '\n') {
            // we're done, loop around
            continue;
        }

        // we've completed a line, process it
        line[pos] = 0; // terminate the string
        pos = 0;       // next time around we start over

        AutoLock al(&lock_);

        if (strncmp(line, "PING", strlen("PING")) == 0) {
            // handle a PONG
            tcp_write(sock_, "PONG\r\n", strlen("PONG\r\n"));
            printf("%s", line);
            printf("PING/PONG\n");
        } else if (strstr(line, " PRIVMSG ") != nullptr) {
            // Parse PRIVMSG lines: :nick!user@host PRIVMSG target :message
            char nick[64] = { 0 };
            char target[64] = { 0 };
            const char *message = nullptr;

            // Extract nick from ":nick!user@host" format
            if (line[0] == ':') {
                const char *exclaim = strchr(line, '!');
                if (exclaim != nullptr) {
                    ptrdiff_t nick_len = exclaim - line - 1; // -1 to skip the ':'
                    if (nick_len > 0 && nick_len < (ptrdiff_t)sizeof(nick)) {
                        strncpy(nick, &line[1], nick_len);
                        nick[nick_len] = 0;
                    }
                }
            }

            // Extract target (channel or nick) and message
            const char *privmsg_pos = strstr(line, "PRIVMSG");
            if (privmsg_pos != nullptr && nick[0] != 0) {
                const char *target_start = privmsg_pos + strlen("PRIVMSG") + 1; // +1 to skip space
                const char *colon = strchr(target_start, ':');
                if (colon) {
                    ptrdiff_t target_len =
                        colon - target_start - 1; // -1 to skip space before colon
                    if (target_len > 0 && target_len < (ptrdiff_t)sizeof(target)) {
                        strncpy(target, target_start, target_len);
                        target[target_len] = 0;
                    }
                    message = colon + 1;
                }
            }

            LTRACEF("Parsed PRIVMSG - Nick: '%s', Target: '%s', Message: '%s'\n", nick, target,
                    message ? message : "(null)");

            // Check for CTCP requests
            const char *ctcp_start = message ? strchr(message, '\001') : nullptr;
            if (ctcp_start != nullptr) {
                // Extract CTCP command (between \001 markers)
                char ctcp_cmd[64] = { 0 };
                const char *cmd_start = ctcp_start + 1;
                const char *cmd_end = strchr(cmd_start, '\001');
                if (cmd_end != nullptr && nick[0] != 0) {
                    ptrdiff_t cmd_len = cmd_end - cmd_start;
                    if (cmd_len > 0 && cmd_len < (ptrdiff_t)sizeof(ctcp_cmd)) {
                        strncpy(ctcp_cmd, cmd_start, cmd_len);
                        ctcp_cmd[cmd_len] = 0;
                        handle_ctcp_request(nick, ctcp_cmd);
                    }
                }
                printf("%s", line);
            } else if (strcmp(target, IRC_NICK) == 0 && message != nullptr) {
                // Check if it's a direct private message to the bot
                printf("[PM from %s]: %s", nick, message);
            } else if (strcmp(target, IRC_CHAN) == 0 && message != nullptr) {
                // If it's from the channel, just prefix the channel
                printf("[%s] %s: %s", IRC_CHAN, nick, message);
            } else {
                printf("%s", line);
            }
        } else {
            printf("%s", line);
        }
    }

    return NO_ERROR;
}

status_t irc_client::console_input_line(const char *line, bool &exit) {
    // printf("CONSOLE LINE '%s'\n", line);

    if (strlen(line) == 0) {
        return NO_ERROR;
    }

    // see if it starts with /
    if (line[0] == '/') {
        if (line[1] == 0) {
            // malformed command
            return NO_ERROR;
        }
        // look for quit command
        if (strncmp(&line[1], "quit", strlen("quit")) == 0) {
            shutdown();
            exit = true;
            return NO_ERROR;
        } else if (strncmp(&line[1], "me ", strlen("me ")) == 0) {
            // /me command - send CTCP ACTION
            AutoLock al(&lock_);
            const char *action_text = &line[4]; // skip "/me "
            char buf[256];
            snprintf(buf, sizeof(buf), "PRIVMSG #sortix :\001ACTION %s\001\r\n", action_text);
            auto err = tcp_write(sock_, buf, strlen(buf));
            return (err < 0) ? static_cast<status_t>(err) : NO_ERROR;
        } else {
            printf("bad command\n");
            return NO_ERROR;
        }
    }

    AutoLock al(&lock_);

    // send it as a privmsg
    char buf[256];
    snprintf(buf, sizeof(buf), "PRIVMSG #sortix :%s\r\n", line);
    auto err = tcp_write(sock_, buf, strlen(buf));
    return (err < 0) ? static_cast<status_t>(err) : NO_ERROR;
}

status_t irc_client::handshake() {
    AutoLock al(&lock_);

    // send USER and NICK
    char buf[128];
    snprintf(buf, sizeof(buf), "USER %s host server :geist\r\n", IRC_USER);
    auto err = tcp_write(sock_, buf, strlen(buf));
    if (err < 0) {
        printf("error %ld writing to server\n", err);
        return static_cast<status_t>(err);
    }

    snprintf(buf, sizeof(buf), "NICK %s\r\n", IRC_NICK);
    err = tcp_write(sock_, buf, strlen(buf));
    if (err < 0) {
        printf("error %ld writing to server\n", err);
        return static_cast<status_t>(err);
    }

    snprintf(buf, sizeof(buf), "JOIN %s\r\n", IRC_CHAN);
    err = tcp_write(sock_, buf, strlen(buf));
    if (err < 0) {
        printf("error %ld writing to server\n", err);
        return static_cast<status_t>(err);
    }

    state_ = state::HANDSHOOK;

    return NO_ERROR;
}

// console worker thread
int console_thread_worker(void *arg) {
    irc_client *irc = static_cast<irc_client *>(arg);

    LTRACEF("top of console thread\n");

    // read a line from the console, giving it to the irc client object at EOL
    status_t err;
    char line[256];
    int pos = 0;
    bool exit = false;
    while (!exit) {
        int c = getchar();
        if (c <= 0) {
            break;
        }

        if (pos == sizeof(line)) {
            printf("line too long, discarding\n");
            pos = 0;
        }

        // printf("char %c (%d)\n", c, c);
        switch (c) {
            case '\n':
            case '\r':
                if (pos > 0) {
                    putchar('\n');
                    // end of a line with characters in it, feed it to the irc client
                    line[pos++] = 0; // null terminate
                    err = irc->console_input_line(line, exit);
                    if (err < 0) {
                        exit = true;
                    }
                    pos = 0;
                }
                break;
            case '\b': // backspace
            case 127:  // DEL
                if (pos > 0) {
                    pos--;
                    fputs("\b \b", stdout); // wipe out a character
                }
                break;

            default:
                line[pos++] = (char)c;
                putchar(c);
                break;
        }
    }

    LTRACEF("console thread exiting\n");

    return 0;
};

void irc_app_entry(const struct app_descriptor *app, void *args) {
    LTRACE_ENTRY;
    printf("welcome to IRC!\n");

    // create a local state object
    irc_client *irc = new irc_client();
    if (!irc) {
        return;
    }

    status_t err;

    irc->init();

    // clean up and delete the object on the way out
    auto ac = lk::make_auto_call([&irc]() {
        printf("cleaning up IRC\n");
        irc->shutdown();
        delete irc;
    });

    // configure the parameters
    irc->set_server(IRC_SERVER);
    irc->set_server_port(IRC_PORT);

    err = irc->connect();
    if (err < 0) {
        return;
    }

    err = irc->handshake();
    if (err < 0) {
        return;
    }

    // start two threads
    thread_t *console_thread;
    thread_t *server_thread;

    console_thread = thread_create("irc console", console_thread_worker, irc, DEFAULT_PRIORITY,
                                   DEFAULT_STACK_SIZE);
    thread_resume(console_thread);

    auto server_thread_worker = [](void *arg) -> int {
        irc_client *_irc = static_cast<irc_client *>(arg);
        printf("top of server thread\n");

        _irc->read_loop();

        return 0;
    };
    server_thread = thread_create("irc socket", server_thread_worker, irc, DEFAULT_PRIORITY,
                                  DEFAULT_STACK_SIZE);
    thread_resume(server_thread);

    thread_join(server_thread, nullptr, INFINITE_TIME);
    thread_join(console_thread, nullptr, INFINITE_TIME);

    LTRACE_EXIT;
}

// clang-format off
APP_START(irc)
    .init = nullptr,
    .entry = irc_app_entry,
    .flags = APP_FLAG_NO_AUTOSTART,
    .stack_size = 0,
APP_END

} // namespace
