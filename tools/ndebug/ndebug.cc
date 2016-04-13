#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

#include "ionode.h"
#include "tcpionode.h"
#include "usbionode.h"

const uint16_t kVendorId = 0x9999;
const uint16_t kProductId = 0x9999;

const uint16_t kListenPort = 9091;

void IOWorkerThread(NDebug::IONode *in, NDebug::IONode *out)
{
    std::vector<uint8_t> buf;
    NDebug::IONodeResult res;
    while (true) {
        res = in->readBuf(&buf);
        if (res == NDebug::IONodeResult::Finished) {
            break;
        } else if (res == NDebug::IONodeResult::Failure) {
            continue;
        }

        res = out->writeBuf(buf);
        if (res == NDebug::IONodeResult::Finished) {
            break;
        } else if (res == NDebug::IONodeResult::Failure) {
            continue;
        }
    }
}

int main(int argc, char *argv[])
{
    NDebug::USBIONode usb(kVendorId, kProductId);
    if (!usb.connect()) {
        std::cerr << "Could not connect to USB device." << std::endl;
        return -1;
    }

    NDebug::TCPIONode tcp(kListenPort);
    if (!tcp.listenAndConnect()) {
        std::cerr << "Could not establish TCP connection." << std::endl;
        return -1;
    }

    // Create two worker threads. One that pushes bytes from the host to the
    // device and the other which works in the reverse direction.
    std::thread deviceToHost(IOWorkerThread, &usb, &tcp);
    std::thread hostToDevice(IOWorkerThread, &tcp, &usb);

    deviceToHost.join();
    hostToDevice.join();

    return 0;
}