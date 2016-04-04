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

#include "usbionode.h"

#include <cassert>
#include <vector>

#include <libusb-1.0/libusb.h>

#include "lib/ndebug/shared_structs.h"

namespace NDebug {

const size_t kConnectRetryLimit = 5;
const unsigned int kDefaultUSBTimeoutMS = 5000;
const size_t kMaxUSBPacketSize = 64;

USBIONode::USBIONode(const uint16_t vendorId, const uint16_t productId)
    : vendorId_(vendorId),
      productId_(productId),
      ctx_(nullptr),
      dev_(nullptr) {}

USBIONode::~USBIONode()
{
    if (dev_) {
        libusb_release_interface(dev_, 0);
        libusb_close(dev_);
        dev_ = nullptr;
    }
}

IONodeResult USBIONode::readBuf(std::vector<uint8_t> *buf)
{
    assert(buf != nullptr);

    if (dev_ == nullptr) {
        return IONodeResult::NotConnected;
    }

    // Create a buffer long enough to hold the USB packet.
    std::vector<uint8_t> readBuffer(kMaxUSBPacketSize);

    int xfer = kMaxUSBPacketSize;
    int rc = libusb_bulk_transfer(dev_, epIn_, &readBuffer[0],
                                  kMaxUSBPacketSize, &xfer,
                                  kDefaultUSBTimeoutMS);
    if (rc < 0) {
        return IONodeResult::Failure;
    }

    // Packet must be long enought to contain at least the header.
    if (xfer < (int)sizeof(ndebug_ctrl_packet_t)) {
        return IONodeResult::Failure;
    }

    // Make sure the packet is well formed.
    ndebug_ctrl_packet_t *pkt =
        reinterpret_cast<ndebug_ctrl_packet_t *>(&readBuffer[0]);
    if (pkt->magic != NDEBUG_CTRL_PACKET_MAGIC) {
        return IONodeResult::Failure;
    }

    if (pkt->type == NDEBUG_CTRL_CMD_RESET) {
        return IONodeResult::Finished;
    } else if (pkt->type != NDEBUG_CTRL_CMD_DATA) {
        return IONodeResult::Failure;
    }

    buf->clear();
    buf->reserve(xfer);

    std::copy(readBuffer.begin() + sizeof(ndebug_ctrl_packet_t),
              readBuffer.begin() + xfer, std::back_inserter(*buf));

    return IONodeResult::Success;
}

IONodeResult USBIONode::writeBuf(const std::vector<uint8_t> &buf)
{
    if (dev_ == nullptr) {
        return IONodeResult::NotConnected;
    }

    std::vector<uint8_t> writeBuffer(kMaxUSBPacketSize);

    ndebug_ctrl_packet_t pkt;
    pkt.magic = NDEBUG_CTRL_PACKET_MAGIC;
    pkt.type = NDEBUG_CTRL_CMD_DATA;
    uint8_t *packetCursor = reinterpret_cast<uint8_t *>(&pkt);

    std::copy(packetCursor, packetCursor + sizeof(pkt), writeBuffer.begin());

    size_t stride = 0;
    for (size_t bufCursor = 0; bufCursor < buf.size(); bufCursor += stride) {
        stride =
            std::min(kMaxUSBPacketSize - sizeof(pkt), buf.size() - bufCursor);
        std::copy(buf.begin() + bufCursor, buf.begin() + bufCursor + stride,
                  writeBuffer.begin() + sizeof(pkt));
        int xfer = stride + sizeof(pkt);
        int rc =
            libusb_bulk_transfer(dev_, epOut_, &writeBuffer[0],
                                 stride + sizeof(pkt), &xfer,
                                 kDefaultUSBTimeoutMS);
        if (rc < 0) {
            return IONodeResult::Failure;
        }
    }

    return IONodeResult::Success;
}

bool USBIONode::connect()
{
    int rc = 0;

    rc = libusb_init(&ctx_);
    if (rc < 0) {
        return false;
    }

    bool success = openDeviceByParams(
                       vendorId_, productId_, NDEBUG_USB_CLASS_USER_DEFINED,
                       NDEBUG_SUBCLASS, NDEBUG_PROTOCOL_SERIAL_PIPE);
    if (!success) {
        return false;
    }

    rc = libusb_claim_interface(dev_, iface_);
    if (rc < 0) {
        libusb_close(dev_);
        dev_ = nullptr;
        return false;
    }

    ndebug_ctrl_packet_t pkt;
    uint8_t *buf = reinterpret_cast<uint8_t *>(&pkt);

    for (size_t i = 0; i < kConnectRetryLimit; i++) {
        // Send a connection RESET packet to the device.
        int xfer = sizeof(pkt);

        pkt.magic = NDEBUG_CTRL_PACKET_MAGIC;
        pkt.type = NDEBUG_CTRL_CMD_RESET;
        rc = libusb_bulk_transfer(dev_, epOut_, buf, sizeof(pkt), &xfer,
                                  kDefaultUSBTimeoutMS);

        // Allow the device to ACK the connection request.
        rc = libusb_bulk_transfer(dev_, epIn_, buf, sizeof(pkt), &xfer,
                                  kDefaultUSBTimeoutMS);

        if (pkt.magic == NDEBUG_CTRL_PACKET_MAGIC &&
                pkt.type == NDEBUG_CTRL_CMD_ESTABLISHED) {
            return true;
        }
    }

    return false;
}

bool USBIONode::openDeviceByParams(const uint16_t vid, const uint16_t pid,
                                   const uint8_t interfaceClass,
                                   const uint8_t interfaceSubClass,
                                   const uint8_t interfaceProtocol)
{
    libusb_device *device = nullptr;
    bool success = false;

    struct libusb_device **deviceList;
    size_t numDevices = libusb_get_device_list(ctx_, &deviceList);

    for (size_t i = 0; i < numDevices; i++) {
        libusb_device_descriptor desc = {0};
        libusb_get_device_descriptor(deviceList[i], &desc);

        if (desc.idVendor != vid || desc.idProduct != pid) {
            continue;
        }

        for (size_t j = 0; j < desc.bNumConfigurations; j++) {
            struct libusb_config_descriptor *cfg;
            libusb_get_config_descriptor(deviceList[i], j, &cfg);

            for (size_t k = 0; k < cfg->bNumInterfaces; k++) {
                libusb_interface_descriptor iface =
                    cfg->interface[k].altsetting[0];
                if (iface.bInterfaceClass == interfaceClass &&
                        iface.bInterfaceSubClass == interfaceSubClass &&
                        iface.bInterfaceProtocol == interfaceProtocol) {
                    iface_ = k;
                    device = deviceList[i];
                    for (size_t l = 0; l < iface.bNumEndpoints; l++) {
                        if (iface.endpoint[l].bEndpointAddress & 0x80) {
                            epIn_ = iface.endpoint[l].bEndpointAddress;
                        } else {
                            epOut_ = iface.endpoint[l].bEndpointAddress;
                        }
                    }
                }
            }
            libusb_free_config_descriptor(cfg);
        }
    }

    if (device != nullptr) {
        int rc = libusb_open(device, &dev_);
        if (rc == 0) {
            success = true;
        }
    }

    libusb_free_device_list(deviceList, 0);

    return success;
}

}  // namespace NDebug;