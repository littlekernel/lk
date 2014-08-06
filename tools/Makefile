
all: lkboot

lkboot: lkboot.c network.c ../app/lkboot/lkboot.h
	gcc -Wall -o lkboot lkboot.c network.c

clean::
	rm -f lkboot
