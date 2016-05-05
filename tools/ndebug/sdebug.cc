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

#include <cassert>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

#include "constants.h"
#include "ionode.h"
#include "lib/ndebug/shared_structs.h"
#include "muxusbionode.h"
#include "tcpionode.h"
#include "usbionode.h"

const uint16_t kConsoleProxyPort = 9092;
const uint16_t kSystemCommandPort = 9093;

void IOWorkerThread(NDebug::IONode *in, NDebug::IONode *out)
{
    assert(in != nullptr);
    assert(out != nullptr);

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

void ConnectionWorkerThread(NDebug::TCPIONode *tcp, NDebug::MuxUSBIONode *usb)
{
    assert(usb);
    assert(tcp);

    std::thread deviceToHost(IOWorkerThread, usb, tcp);
    std::thread hostToDevice;

    while (true) {
        tcp->listenAndAccept();

        hostToDevice = std::thread(IOWorkerThread, tcp, usb);
        hostToDevice.join();
    }

    deviceToHost.join();
}

// Thread responsible for reading packets from the USB device and queueing them
// on the approrpriate MuxUSBIONode.
void USBMuxReaderThread(NDebug::USBIONode *usbSource,
                        const std::vector<NDebug::MuxUSBIONode *> &muxionodes)
{
    assert(muxionodes.size() == NDEBUG_SYS_CHANNEL_COUNT);

    while (true) {
        std::vector<uint8_t> buf;
        NDebug::IONodeResult result = usbSource->readBuf(&buf);

        if (result != NDebug::IONodeResult::Success) continue;


        if (buf.size() < sizeof(ndebug_system_packet_t)) continue;


        ndebug_system_packet_t *pkt =
            reinterpret_cast<ndebug_system_packet_t *>(&buf[0]);
        uint32_t channel = pkt->channel;
        if (channel >= NDEBUG_SYS_CHANNEL_COUNT) continue;

        if (pkt->ctrl.type == NDEBUG_CTRL_CMD_DATA) {
            buf.erase(buf.begin(),
                      buf.begin() + sizeof(ndebug_system_packet_t));
            muxionodes[channel]->queueBuf(buf);
        } else if (pkt->ctrl.type == NDEBUG_CTRL_CMD_FLOWCTRL) {
            muxionodes[channel]->signalBufAvail();
        }
    }
}

int main(int argc, char *argv[])
{
    NDebug::USBIONode usb(NDebug::Constants::kVendorId,
                          NDebug::Constants::kProductId,
                          NDEBUG_PROTOCOL_LK_SYSTEM);

    if (!usb.connect()) {
        std::cerr << "Could not connect to USB device." << std::endl;
        return -1;
    }

    std::cerr << "Connected to USB device." << std::endl;

    NDebug::TCPIONode consoleTCP(kConsoleProxyPort);
    if (!consoleTCP.open()) {
        std::cerr << "Could not open console TCP connection." << std::endl;
        return -1;
    }

    NDebug::TCPIONode commandTCP(kSystemCommandPort);
    if (!commandTCP.open()) {
        std::cerr << "Could not open command TCP connection." << std::endl;
        return -1;
    }

    NDebug::MuxUSBIONode consoleMuxUSBIONode(NDEBUG_SYS_CHANNEL_CONSOLE, &usb);
    NDebug::MuxUSBIONode commandMuxUSBIONode(NDEBUG_SYS_CHANNEL_COMMAND, &usb);

    std::vector<NDebug::MuxUSBIONode *> muxUSBIONodes {
        &consoleMuxUSBIONode,
        &commandMuxUSBIONode
    };

    std::thread muxReaderThread(USBMuxReaderThread, &usb, muxUSBIONodes);

    std::thread consoleConnectionWorkerThread(
        ConnectionWorkerThread, &consoleTCP, &consoleMuxUSBIONode);
    std::thread commandConnectionWorkerThread(
        ConnectionWorkerThread, &commandTCP, &commandMuxUSBIONode);

    muxReaderThread.join();
    consoleConnectionWorkerThread.join();
    commandConnectionWorkerThread.join();

    return 0;
}
