#!/usr/bin/env python

import sys, os, struct

if len(sys.argv) < 2:
    print "not enough args, usage:"
    print "%s <binfile>" % sys.argv[0]
    sys.exit(1)

f = open(sys.argv[1], "r+b")

a = struct.unpack('iiiiiii', f.read(7*4))

s = 0
for i in a:
    s += i
s = -s

f.seek(7*4)
f.write(struct.pack('i', s))

f.close()

