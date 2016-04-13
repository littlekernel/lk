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