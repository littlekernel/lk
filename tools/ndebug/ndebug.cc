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

#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

#include "constants.h"
#include "ionode.h"
#include "lib/ndebug/shared_structs.h"
#include "tcpionode.h"
#include "usbionode.h"

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
    NDebug::USBIONode usb(NDebug::Constants::kVendorId,
                          NDebug::Constants::kProductId,
                          NDEBUG_PROTOCOL_SERIAL_PIPE);
    if (!usb.connect()) {
        std::cerr << "Could not connect to USB device." << std::endl;
        return -1;
    }

    NDebug::TCPIONode tcp(kListenPort);
    if (!tcp.open()) {
        std::cerr << "Could not open TCP connection." << std::endl;
        return -1;
    }

    std::thread deviceToHost(IOWorkerThread, &usb, &tcp);
    std::thread hostToDevice;

    while (true) {
        tcp.listenAndAccept();

        hostToDevice = std::thread(IOWorkerThread, &tcp, &usb);
        hostToDevice.join();
    }

    deviceToHost.join();

    return 0;
}
