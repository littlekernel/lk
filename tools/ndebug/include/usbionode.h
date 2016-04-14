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
#include <ionode.h>

struct libusb_context;
struct libusb_device_handle;

namespace NDebug {

class USBIONode : public IONode {
public:
    USBIONode(const uint16_t vendorId, const uint16_t productId);
    virtual ~USBIONode();

    IONodeResult readBuf(std::vector<uint8_t> *buf) override;
    IONodeResult writeBuf(const std::vector<uint8_t> &buf) override;

    bool connect();

private:
    bool openDeviceByParams(const uint16_t vid, const uint16_t pid,
                            const uint8_t interfaceClass,
                            const uint8_t interfaceSubClass,
                            const uint8_t interfaceProtocol);

    const uint16_t vendorId_;
    const uint16_t productId_;

    uint8_t epOut_;
    uint8_t epIn_;
    uint8_t iface_;

    libusb_context *ctx_;
    libusb_device_handle *dev_;
};

}  // namespace ndebug