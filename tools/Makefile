
all: lkboot

lkboot: lkboot.c liblkboot.c network.c network.h liblkboot.h ../app/lkboot/lkboot.h
	gcc -Wall -o lkboot lkboot.c liblkboot.c network.c

clean::
	rm -f lkboot
