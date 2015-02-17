TARGET := vexpress-a9

MODULES += \
	app/tests \
	app/stringtests \
	app/shell \
	lib/aes \
	lib/aes/test \
	lib/bytes \
	lib/cksum \
	lib/debugcommands \
	lib/evlog \
	lib/libm

WITH_LINKER_GC := 0

GLOBAL_DEFINES += WITH_KERNEL_EVLOG=1

