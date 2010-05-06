LOCAL_DIR := $(GET_LOCAL_DIR)

MODULES += \
	lib/fs \
	lib/bcache \
	lib/bio

OBJS += \
	$(LOCAL_DIR)/ext2.o \
	$(LOCAL_DIR)/dir.o \
	$(LOCAL_DIR)/io.o \
	$(LOCAL_DIR)/file.o
