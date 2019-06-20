#!/usr/bin/env python3

import argparse
import socket
import time

print("hello")

parser = argparse.ArgumentParser(description="Network Testing Tools");
parser.add_argument('-a', '--address', nargs=1, help='ip or dns address of host',
                    default='192.168.0.99')
parser.add_argument('-p', '--port', nargs=1, help='port on host',
                    default=[1234], type=int)

args = parser.parse_args()
print(args)

print("connecting to {}, port {}".format(args.address, args.port[0]))
s = socket.create_connection((args.address, args.port[0]))
print(s)

s.send(b"what what");

while True:
    time.sleep(100)
    data = s.recv(65536)
    if not data:
        break

s.close()

# vim: set ts=4 sw=4 expandtab:
