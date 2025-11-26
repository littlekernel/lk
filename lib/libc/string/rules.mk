LOCAL_DIR := $(GET_LOCAL_DIR)

C_STRING_OPS := \
	bcopy \
	bzero \
	memchr \
	memcmp \
	memcpy \
	memmove \
	memset \
	strcasecmp \
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
# x86 or x86-64 or anothers .....? T_T
ifeq ($(SUBARCH), x86-64)
	-include $(LOCAL_DIR)/arch/$(SUBARCH)/rules.mk
$(info string SUBARCH = $(SUBARCH))
else
	-include $(LOCAL_DIR)/arch/$(ARCH)/rules.mk
$(info strings ARCH = $(ARCH))
endif

MODULE_SRCS += \
	$(addprefix $(LIBC_STRING_C_DIR)/,$(addsuffix .c,$(C_STRING_OPS)))

