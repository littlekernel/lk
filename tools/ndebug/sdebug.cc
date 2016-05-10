#include "lib/ndebug/system/handlers/fs/shared_structs.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdint>
#include <fstream>

#include <iostream>
#include <string>
#include <vector>

using namespace std;

struct __attribute__((__packed__)) FSDebugHeader {
    uint32_t opcode;
    cmdhdlr_fs_header_t cmd_header;
};

int32_t readResult(const int sock) {
    uint8_t res[64];
    int rc = recv(sock, res, sizeof(res), 0);
    if (rc < static_cast<int>(sizeof(uint32_t))) {
        cerr << "error reading reply from device." << endl;
        return -1;
    }
    int32_t reply = *((int32_t *)res);
    if ((reply & 0xFFFFFF00) != 0x52455400) {
        cerr << "malformed reply from device" << endl;
        return -1;
    }

    reply = reply & 0xFF;
    return reply;
}

int handlePutfile(int sock, const string& localpath, const string& remotepath) {
    ifstream file(localpath, ios::binary | ios::ate);
    if (!file) {
        cerr << "could not open file " << localpath << " for reading." << endl;
        return -1;
    }

    streamsize size = file.tellg();
    file.seekg(0, ios::beg);

    vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        cerr << "could not read " << size << " bytes from " << localpath
             << endl;
        return -1;
    }

    // Send the Filesystem Debug Command to the device.
    FSDebugHeader header;
    header.opcode = CMDHDLR_FS_OPCODE;
    header.cmd_header.opcode = OPCODE_PUTFILE;
    header.cmd_header.path_len = remotepath.size();
    header.cmd_header.data_len = buffer.size();
    int rc = 
        send(sock, reinterpret_cast<uint8_t *>(&header), sizeof(header), 0);
    if (rc != sizeof(header)) {
        cerr << "error writing command header to socket" << endl;
        return -1;
    }

    // Await an ACK or ERR from the device.
    int32_t result = readResult(sock);
    if (result != 0) {
        cerr << "header ack from device failed with error = " << result << endl;
        return result;
    }

    // Send the Filename to the device.
    if (send(sock, remotepath.data(), remotepath.size(), 0) < 0) {
        cerr << "error writing filename '" << remotepath << "' to remote device"
             << endl;
        return -1;
    }

    // Await an ACK or ERR from the device.
    result = readResult(sock);
    if (result != 0) {
        cerr << "filename ack from device failed with error = " << result
             << endl;
        return result;
    }

    size_t total = 0;
    while (total < buffer.size()) {
        int written = 
            send(sock, buffer.data() + total, buffer.size() - total, 0);
        if (written < 0) {
            cout << "Error 6!" << endl;
            return -1;
        }
        total += written;
    }

    // Await an ACK or ERR from the device.
    result = readResult(sock);
    if (result != 0) {
        cerr << "data ack from device failed with error = " << result
             << endl;
        return result;
    }

    return 0;
}

int main(int argc , char *argv[])
{
    if (argc < 2) {
        cout << "Not enough args" << endl;
        return -1;
    }

    int sock;
    struct sockaddr_in server;
     
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock < 0) 
    	return -1;
     
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(9093);
 
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
        return -1;
    }

    string command(argv[1]);
    if (command == "putfile") {
        if (argc < 4) {
            cout << "Not enough args" << endl;
            return -1;
        }
        string localpath(argv[2]);
        string remotepath(argv[3]);
        handlePutfile(sock, localpath, remotepath);
    } else {
        cout << "Command not found." << endl;
    }

     
    close(sock);
    return 0;
}