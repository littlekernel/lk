
all: lkboot mkboot

LKBOOT_SRCS := lkboot.c liblkboot.c network.c
LKBOOT_DEPS := network.h liblkboot.h ../app/lkboot/lkboot.h
lkboot: $(LKBOOT_SRCS) $(LKBOOT_DEPS)
	gcc -Wall -o lkboot $(LKBOOT_SRCS)

MKBOOT_SRCS := mkimage.c bootimage.c ../lib/mincrypt/sha256.c
mkboot: $(MKBOOT_SRCS)
	gcc -Wall -g -o mkimage -I../lib/mincrypt/include $(MKBOOT_SRCS)

clean::
	rm -f lkboot mkimage
