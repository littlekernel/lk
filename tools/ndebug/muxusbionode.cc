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

#include <algorithm>

#include "muxusbionode.h"
#include "usbionode.h"
#include "lib/ndebug/shared_structs.h"

namespace NDebug {

MuxUSBIONode::MuxUSBIONode(const sys_channel_t ch, USBIONode *usb)
    : ch_(ch),
      usb_(usb),
      sem_(0) {}

MuxUSBIONode::~MuxUSBIONode() {}

IONodeResult MuxUSBIONode::readBuf(std::vector<uint8_t> *buf)
{
    *buf = queue_.pop();
    if (buf->empty()) {
        return IONodeResult::Finished;
    } else {
        return IONodeResult::Success;
    }
}

IONodeResult MuxUSBIONode::writeBuf(const std::vector<uint8_t> &buf)
{
    // Write the channel as the first byte of the result.
    uint32_t channel = ch_;

    std::vector<uint8_t> outBuf(
        reinterpret_cast<uint8_t *>(&channel),
        reinterpret_cast<uint8_t *>(&channel) + sizeof(channel)
    );

    size_t copied = 0;
    while (copied < buf.size()) {
        outBuf.resize(sizeof(channel));
        size_t toCopy = std::min(
            buf.size() - copied,
            (size_t)(64 - (sizeof(ndebug_system_packet_t) + sizeof(channel)))
        );
        std::copy_n(buf.begin() + copied, toCopy, std::back_inserter(outBuf));
        sem_.wait();
        IONodeResult res = usb_->writeBuf(outBuf);
        if (res != IONodeResult::Success) {
            return res;
        }
        copied += toCopy;
    }

    return IONodeResult::Success;
}

void MuxUSBIONode::queueBuf(const std::vector<uint8_t> &buf)
{
    if (buf.empty()) return;

    queue_.push(buf);
}

void MuxUSBIONode::signalBufAvail()
{
    sem_.signal();
}

void MuxUSBIONode::signalFinished()
{
    std::vector<uint8_t> empty;
    queue_.push(empty);
}


}  // namespace