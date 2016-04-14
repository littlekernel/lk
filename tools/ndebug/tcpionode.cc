/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
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

#include "tcpionode.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace NDebug {

const int kSockFdClosed = -1;

TCPIONode::TCPIONode(uint16_t port)
    : port_(port),
      socket_(kSockFdClosed) {}

TCPIONode::~TCPIONode()
{
    if (socket_ >= 0) {
        shutdown(socket_, SHUT_RDWR);
        close(socket_);
        socket_ = kSockFdClosed;
    }
}

IONodeResult TCPIONode::readBuf(std::vector<uint8_t> *buf)
{
    std::lock_guard<std::mutex> g(lock_);

    if (socket_ == kSockFdClosed) {
        return IONodeResult::NotConnected;
    }

    buf->clear();
    buf->resize(64);

    ssize_t bytes = read(socket_, &(*buf)[0], 64);
    if (bytes >= 0) {
        buf->resize(bytes);
        return IONodeResult::Success;
    }

    return IONodeResult::Failure;
}

IONodeResult TCPIONode::writeBuf(const std::vector<uint8_t> &buf)
{
    std::lock_guard<std::mutex> g(lock_);

    if (socket_ == kSockFdClosed) {
        return IONodeResult::NotConnected;
    }

    size_t written = 0;
    do {
        ssize_t bytes = write(socket_, &buf[written], buf.size() - written);
        if (bytes < 0) {
            return IONodeResult::Failure;
        }
        written += bytes;
    } while (written < buf.size());

    return IONodeResult::Success;
}

bool TCPIONode::listenAndConnect()
{
    std::lock_guard<std::mutex> g(lock_);

    struct sockaddr_in sa;
    int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        return false;
    }

    memset(&sa, 0, sizeof sa);

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port_);
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(sock,(struct sockaddr *)&sa, sizeof sa) == -1) {
        close(sock);
        return false;
    }

    if (listen(sock, 1) == -1) {
        close(sock);
        return false;
    }

    int client = accept(sock, NULL, NULL);
    if (client < 0) {
        close(sock);
        return false;
    }

    socket_ = client;

    close(sock);

    return true;
}

}  // namespace NDebug