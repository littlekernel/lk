/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <string.h>
#include <stdio.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <app.h>
#include <lib/minip.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>

#include "lktl.h"

#define LOCAL_TRACE 1

//#define IRC_SERVER IPV4(176,58,122,119) // irc.libera.chat
#define IRC_SERVER IPV4(88,99,244,30) // irc.sortix.org
//#define IRC_SERVER IPV4(192,168,1,110) // localhost
#define IRC_PORT 6667
#define IRC_USER "geist"
#define IRC_NICK "geist-lk"
#define IRC_CHAN "#sortix"
//#define IRC_CHAN "#osdev"

class irc_client {
public:
    irc_client();
    ~irc_client();

    void init();
    void shutdown();
    status_t connect();
    status_t handshake();
    status_t read_loop();
    status_t console_input_line(const char *line, bool &exit);

    void set_server(uint32_t server) { server_ip_ = server; }
    void set_server_port(uint16_t port) { server_port_ = port; }

private:
    mutex_t lock_ = MUTEX_INITIAL_VALUE(lock_);
    tcp_socket_t *sock_ = nullptr;

    uint32_t server_ip_ = 0;
    uint16_t server_port_ = 0;
    enum {
        INITIAL,
        CONNECTED,
        HANDSHOOK,
    } state_ = INITIAL;
};

irc_client::irc_client() = default;
irc_client::~irc_client() = default;

void irc_client::init() {
}

void irc_client::shutdown() {
    lktl::auto_lock al(&lock_);

    if (sock_) {
        if (state_ == HANDSHOOK) {
            // send quit
            tcp_write(sock_, "QUIT\r\n", strlen("QUIT\r\n"));
            // XXX flush socket
            thread_sleep(1000);
        }
        tcp_close(sock_);
        sock_ = nullptr;
    }
}

status_t irc_client::connect() {
    lktl::auto_lock al(&lock_);

    if (server_ip_ == 0 || server_port_ == 0) {
        return ERR_NOT_CONFIGURED;
    }

    auto err = tcp_connect(&sock_, server_ip_, server_port_);
    if (err < 0) {
        printf("err %d connecting to server\n", err);
        return ERR_CHANNEL_CLOSED; // TODO: better one
    }

    state_ = CONNECTED;

    return NO_ERROR;
}

status_t irc_client::read_loop() {
    char line[1024];
    int pos = 0;
    for (;;) {
        char c;
        ssize_t r = tcp_read(sock_, &c, sizeof(c));
        if (r < 0) {
            return r;
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
        pos = 0; // next time around we start over

        lktl::auto_lock al(&lock_);

        if (strncmp(line, "PING", strlen("PING"))== 0) {
            // handle a PONG
            tcp_write(sock_, "PONG\r\n", strlen("PONG\r\n"));
            printf("%s", line);
            printf("PING/PONG\n");
        } else {
            printf("%s", line);
        }
    }

    return NO_ERROR;
}

status_t irc_client::console_input_line(const char *line, bool &exit) {
    //printf("CONSOLE LINE '%s'\n", line);

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
        } else {
            printf("bad command\n");
            return NO_ERROR;
        }
    }

    lktl::auto_lock al(&lock_);

    // send it as a privmsg
    char buf[256];
    snprintf(buf, sizeof(buf), "PRIVMSG #sortix :%s\r\n", line);
    status_t err = tcp_write(sock_, buf, strlen(buf));
    return err;
}

status_t irc_client::handshake() {
    lktl::auto_lock al(&lock_);

    // send USER and NICK
    char buf[128];
    snprintf(buf, sizeof(buf), "USER %s host server :geist\r\n", IRC_USER);
    status_t err = tcp_write(sock_, buf, strlen(buf));
    if (err < 0) {
        printf("error %d writing to server\n", err);
        return err;
    }

    snprintf(buf, sizeof(buf), "NICK %s\r\n", IRC_NICK);
    err = tcp_write(sock_, buf, strlen(buf));
    if (err < 0) {
        printf("error %d writing to server\n", err);
        return err;
    }

    snprintf(buf, sizeof(buf), "JOIN %s\r\n", IRC_CHAN);
    err = tcp_write(sock_, buf, strlen(buf));
    if (err < 0) {
        printf("error %d writing to server\n", err);
        return err;
    }

    state_ = HANDSHOOK;

    return NO_ERROR;
}

// console worker thread
static int console_thread_worker(void *arg) {
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

        //printf("char %c (%d)\n", c, c);
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
            case 127: // DEL
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


static void irc_app_entry(const struct app_descriptor *app, void *args) {
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
    auto cleanup = [&irc]() {
        printf("cleaning up IRC\n");
        irc->shutdown();
        delete irc;
    };
    auto ac = lktl::make_auto_call(cleanup);

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

    console_thread = thread_create("irc console", console_thread_worker, irc, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(console_thread);

    auto server_thread_worker = [](void *arg) -> int {
        irc_client *_irc = static_cast<irc_client *>(arg);
        printf("top of server thread\n");

        _irc->read_loop();

        return 0;
    };
    server_thread = thread_create("irc socket", server_thread_worker, irc, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(server_thread);

    thread_join(server_thread, nullptr, INFINITE_TIME);
    thread_join(console_thread, nullptr, INFINITE_TIME);

    LTRACE_EXIT;
}

APP_START(irc)
.init = nullptr,
.entry = irc_app_entry,
.flags = APP_FLAG_NO_AUTOSTART,
.stack_size = 0,
APP_END

