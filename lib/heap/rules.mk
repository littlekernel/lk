LOCAL_DIR := $(GET_LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/heap_wrapper.c

# pick a heap implementation
ifndef LK_HEAP_IMPLEMENTATION
LK_HEAP_IMPLEMENTATION=miniheap
endif
ifeq ($(LK_HEAP_IMPLEMENTATION),miniheap)
MODULE_DEPS := $(LOCAL_DIR)/miniheap
endif
ifeq ($(LK_HEAP_IMPLEMENTATION),dlmalloc)
MODULE_DEPS := $(LOCAL_DIR)/dlmalloc
endif

GLOBAL_DEFINES += LK_HEAP_IMPLEMENTATION=$(LK_HEAP_IMPLEMENTATION)

include make/module.mk
