#!/usr/bin/env python

# generates Zynq bootrom header from input payout

import sys, os, array

if len(sys.argv) < 3:
    print "not enough args, usage:"
    print "%s <binfile> <outfile>" % sys.argv[0]
    sys.exit(1)

fin = open(sys.argv[1], "r+b")
finsize = os.stat(sys.argv[1]).st_size
fout = open(sys.argv[2], "w+b")

header = array.array('I')

# start generating header
# from section 6.3.2 of Zynq-700 AP SoC Technical Reference Manual (v1.7)

# vector table (8 words)
for _ in range(0, 8):
    header.append(0)

# (0x20) width detection
header.append(0xaa995566)

# (0x24) identification 'XLNX'
header.append(0x584c4e58)

# (0x28) encryption status (not encrypted)
header.append(0)

# (0x2c) user defined
header.append(0)

# (0x30) source offset
header.append(0x8c0)

# (0x34) length of image
header.append(finsize)

# (0x38) reserved
header.append(0)

# (0x3c) start of execution (0)
header.append(0)

# (0x40) total image length (same as length of image for non secure)
header.append(finsize)

# (0x44) reserved
header.append(0)

# (0x48) header checksum
sum = 0
for i in header:
    sum += i
sum = ~sum
header.append(sum & 0xffffffff)

# user defined
for _ in range(0x4c, 0xa0, 4):
    header.append(0)

# register init pairs (all ffs to cause it to skip)
for _ in range(0xa0, 0x8a0, 4):
    header.append(0xffffffff)

# reserved
for _ in range(0x8a0, 0x8c0, 4):
    header.append(0)

fout.write(header)

# copy the input into the output
while True:
    buf = fin.read(1024)
    if not buf:
        break
    fout.write(buf)

fin.close()
fout.close()
