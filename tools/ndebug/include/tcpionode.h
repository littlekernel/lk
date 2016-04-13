#pragma once

#include <cstdint>
#include <ionode.h>
#include <mutex>

namespace NDebug {

class TCPIONode : public IONode {
public:
    TCPIONode(uint16_t port);
    virtual ~TCPIONode();

    IONodeResult readBuf(std::vector<uint8_t> *buf) override;
    IONodeResult writeBuf(const std::vector<uint8_t> &buf) override;

    bool listenAndConnect();

private:
    // All public methods are protected by this global lock.
    std::mutex lock_;

    const uint16_t port_;
    int socket_;
};

}  // namespace ndebug