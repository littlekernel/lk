LOCAL_DIR := $(GET_LOCAL_DIR)

C_STRING_OPS := \
	bcopy \
	bzero \
	memchr \
	memcmp \
	memcpy \
	memmove \
	memset \
	strcat \
	strchr \
	strcmp \
	strcoll \
	strcpy \
	strdup \
	strerror \
	strlcat \
	strlcpy \
	strlen \
	strncat \
	strncpy \
	strncmp \
	strnicmp \
	strnlen \
	strpbrk \
	strrchr \
	strspn \
	strstr \
	strtok \
	strxfrm

LIBC_STRING_C_DIR := $(LOCAL_DIR)

# include the arch specific string routines
#
# the makefile may filter out implemented versions from the C_STRING_OPS variable
include $(LOCAL_DIR)/arch/$(ARCH)/rules.mk

OBJS += \
	$(addprefix $(LIBC_STRING_C_DIR)/,$(addsuffix .o,$(C_STRING_OPS)))

