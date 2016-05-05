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

#pragma once

#include <cstdint>

#include "boundedblockingqueue.h"
#include "ionode.h"
#include "semaphore.h"
#include "lib/ndebug/shared_structs.h"

namespace NDebug {

class USBIONode;

class MuxUSBIONode : public IONode {
public:
    MuxUSBIONode(const sys_channel_t ch, USBIONode *usb);
    virtual ~MuxUSBIONode();

    IONodeResult readBuf(std::vector<uint8_t> *buf) override;
    IONodeResult writeBuf(const std::vector<uint8_t> &buf) override;

    void queueBuf(const std::vector<uint8_t> &buf);
    void signalBufAvail();

private:
    BoundedBlockingQueue<std::vector<uint8_t> > queue_;
    const sys_channel_t ch_;
    USBIONode *usb_;
    Semaphore sem_;
};

}  // namespace ndebug