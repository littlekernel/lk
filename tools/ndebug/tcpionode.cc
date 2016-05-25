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
      connectionSocket_(kSockFdClosed),
      listenerSocket_(kSockFdClosed) {}

TCPIONode::~TCPIONode()
{
    stop();
}

IONodeResult TCPIONode::readBuf(std::vector<uint8_t> *buf)
{
    if (connectionSocket_ == kSockFdClosed) {
        return IONodeResult::NotConnected;
    }

    buf->clear();
    buf->resize(64);

    ssize_t bytes = read(connectionSocket_, &(*buf)[0], 64);
    if (bytes == 0) {
        return IONodeResult::Finished;
    } else if (bytes > 0) {
        buf->resize(bytes);
        return IONodeResult::Success;
    }

    return IONodeResult::Failure;
}

IONodeResult TCPIONode::writeBuf(const std::vector<uint8_t> &buf)
{
    if (connectionSocket_ == kSockFdClosed) {
        return IONodeResult::NotConnected;
    }

    size_t written = 0;
    do {
        ssize_t bytes = write(connectionSocket_, &buf[written], buf.size() - written);
        if (bytes < 0) {
            return IONodeResult::Failure;
        } else if (bytes == 0) {
            return IONodeResult::Finished;
        }
        written += bytes;
    } while (written < buf.size());

    return IONodeResult::Success;
}

bool TCPIONode::open()
{
    struct sockaddr_in sa;
    listenerSocket_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenerSocket_ == -1) {
        return false;
    }

    int enable = 1;
    setsockopt(listenerSocket_, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    memset(&sa, 0, sizeof sa);

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port_);
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(listenerSocket_, (struct sockaddr *)&sa, sizeof sa) == -1) {
        close(listenerSocket_);
        listenerSocket_ = kSockFdClosed;
        return false;
    }

    return true;
}

void TCPIONode::stop()
{
    if (connectionSocket_ >= 0) {
        shutdown(connectionSocket_, SHUT_RDWR);
        close(connectionSocket_);
        connectionSocket_ = kSockFdClosed;
    }
    if (listenerSocket_ >= 0) {
        shutdown(listenerSocket_, SHUT_RDWR);
        close(listenerSocket_);
        listenerSocket_ = kSockFdClosed;
    }
}

void TCPIONode::swapConnectionSocket(const int newSocket)
{
    if (connectionSocket_ != -1) {
        // Drop the previous connection.
        shutdown(connectionSocket_, SHUT_RDWR);
        close(connectionSocket_);
    }

    connectionSocket_ = newSocket;
}

bool TCPIONode::listenAndAccept()
{
    if (kSockFdClosed == listenerSocket_)
        return false;

    if (listen(listenerSocket_, 1) == -1) {
        return false;
    }

    int client = accept(listenerSocket_, NULL, NULL);
    if (client < 0) {
        return false;
    }

    swapConnectionSocket(client);

    return true;
}

}  // namespace NDebug