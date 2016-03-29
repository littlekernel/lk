
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

clean::
	rm -f lkboot mkimage
