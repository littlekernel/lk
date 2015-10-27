LOCAL_DIR := $(GET_LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/heap_wrapper.c \
	$(LOCAL_DIR)/page_alloc.c

ifeq ($(WITH_CPP_SUPPORT),true)
MODULE_SRCS += \
	$(LOCAL_DIR)/new.cpp
endif

# pick a heap implementation
ifndef LK_HEAP_IMPLEMENTATION
LK_HEAP_IMPLEMENTATION=miniheap
endif
ifeq ($(LK_HEAP_IMPLEMENTATION),miniheap)
MODULE_DEPS := lib/heap/miniheap
endif
ifeq ($(LK_HEAP_IMPLEMENTATION),dlmalloc)
MODULE_DEPS := lib/heap/dlmalloc
endif
ifeq ($(LK_HEAP_IMPLEMENTATION),cmpctmalloc)
MODULE_DEPS := lib/heap/cmpctmalloc
endif

GLOBAL_DEFINES += LK_HEAP_IMPLEMENTATION=$(LK_HEAP_IMPLEMENTATION)

include make/module.mk
