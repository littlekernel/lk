#!/usr/bin/env python
# vim: set expandtab ts=4 sw=4 tw=100:

import sys
from optparse import OptionParser

parser = OptionParser()
parser.add_option("-b", "--before", dest="before", action="append",
                  help="text to put before, may be specified more than once")
parser.add_option("-a", "--after", dest="after", action="append",
                  help="text to put after, may be specified more than once")
(options, args) = parser.parse_args()

if options.before and len(options.before) > 0:
    for b in options.before:
        print b

offset = 0
f = bytearray(sys.stdin.read())
for c in f:
    if offset != 0 and offset % 16 == 0:
        print ""
    print "%#04x," % c,
    offset = offset + 1
print ""

if options.after and len(options.after) > 0:
    for a in options.after:
        print a

