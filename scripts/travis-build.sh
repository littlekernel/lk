#!/bin/sh

# Copyright (c) 2014 Eren Türkay <turkay.eren@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


# This is a script for travis-ci that builds lk for different
# architectures using different toolchains on travis-ci build machine.
#
# PROJECT and TOOLCHAIN are provided as environment variables. These
# variables are specified in .travis.yml file.

TOOLCHAIN_BASE_URL="http://newos.org/toolchains"
TOOLCHAIN_SUFFIX="tar.xz"

TOOLCHAIN_ADDRESS="$TOOLCHAIN_BASE_URL/$TOOLCHAIN.$TOOLCHAIN_SUFFIX"

mkdir -p archives
cd archives
echo "Downloading toolchain $TOOLCHAIN from $TOOLCHAIN_ADDRESS"
wget -v -N $TOOLCHAIN_ADDRESS || exit 1
cd ..

echo "Unpacking $TOOLCHAIN"
tar xf archives/$TOOLCHAIN.$TOOLCHAIN_SUFFIX || exit 1
export PATH=`pwd`/$TOOLCHAIN/bin:$PATH

echo "Starting build '$PROJECT' with '$TOOLCHAIN'\n"
make $PROJECT
