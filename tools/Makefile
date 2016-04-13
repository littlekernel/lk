
all: lkboot mkimage

LKBOOT_SRCS := lkboot.c liblkboot.c network.c
LKBOOT_DEPS := network.h liblkboot.h ../app/lkboot/lkboot_protocol.h
LKBOOT_INCS :=
lkboot: $(LKBOOT_SRCS) $(LKBOOT_DEPS)
	gcc -Wall -o $@ $(LKBOOT_INCS) $(LKBOOT_SRCS)

MKIMAGE_DEPS := bootimage.h ../lib/bootimage/include/lib/bootimage_struct.h
MKIMAGE_SRCS := mkimage.c bootimage.c ../external/lib/mincrypt/sha256.c
MKIMAGE_INCS := -I../external/lib/mincrypt/include -I../lib/bootimage/include
mkimage: $(MKIMAGE_SRCS) $(MKIMAGE_DEPS)
	gcc -Wall -g -o $@ $(MKIMAGE_INCS) $(MKIMAGE_SRCS)

NDEBUG_DEPS :=
NDEBUG_SRCS := ndebug/ndebug.cc ndebug/usbionode.cc ndebug/tcpionode.cc
NDEBUG_INCS := -I./ndebug/include -I../lib/ndebug/include
ndbg: $(NDEBUG_SRCS) $(NDEBUG_DEPS)
	g++ -std=c++11 -Wall -g -o $@ $(NDEBUG_INCS) $(NDEBUG_SRCS) -L/usr/lib/x86_64-linux-gnu -lusb-1.0

clean::
	rm -f lkboot mkimage
