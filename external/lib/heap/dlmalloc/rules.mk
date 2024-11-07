LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# MSPACE=1 enables mspace_malloc and other mspace_* routines.
# They allow users to use preallocated memory for heap allocations
# It's common for VM applications to preallocate backing memory for
# the guest, then free the entire backing memory at once after guest
# exits. This ensures no memory leak even if guest doesn't free its
# memory properly. Or hypervisor may wish that the guest memory
# are all contigous, etc.
MODULE_DEFINES=MSPACES=1

MODULE_SRCS += \
	$(LOCAL_DIR)/dlmalloc.c

include make/module.mk
