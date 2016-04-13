#pragma once

#include <cstdint>
#include <vector>

namespace NDebug {

enum class IONodeResult {
    Failure,   // I/O op failed
    Success,   // I/O op succeeded
    Finished,  // No more I/O ops remaining, perform graceful shutdown
    NotConnected
};

class IONode {
public:
    virtual IONodeResult readBuf(std::vector<uint8_t> *buf) = 0;
    virtual IONodeResult writeBuf(const std::vector<uint8_t> &buf) = 0;
};

}  // namespace NDebug