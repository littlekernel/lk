LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# An rofs image linked into the binary and registered as a memory block device at boot, for
# a file system that comes with the kernel:
#   ROFS_EMBED_IMAGE  the image, built by scripts/mkrofs.py (required)
#   ROFS_EMBED_NAME   the block device's name (default rofs0)
#   ROFS_EMBED_ALIGN  where the image starts in the binary, at least the image's own file
#                     alignment, so that a file's address keeps it (default 4096)
ifeq ($(ROFS_EMBED_IMAGE),)
$(error lib/fs/rofs/embed: set ROFS_EMBED_IMAGE to the image to embed)
endif
ROFS_EMBED_NAME ?= rofs0
ROFS_EMBED_ALIGN ?= 4096

MODULE_DEPS += lib/bio
MODULE_DEPS += lib/fs/rofs

MODULE_DEFINES += ROFS_EMBED_IMAGE=\"$(abspath $(ROFS_EMBED_IMAGE))\"
MODULE_DEFINES += ROFS_EMBED_NAME=\"$(ROFS_EMBED_NAME)\"
MODULE_DEFINES += ROFS_EMBED_ALIGN=$(ROFS_EMBED_ALIGN)

MODULE_SRCS += $(LOCAL_DIR)/embed.c

# .incbin is invisible to the compiler's dependency output
$(call TOBUILDDIR,$(LOCAL_DIR)/embed.c.o): $(abspath $(ROFS_EMBED_IMAGE))

include make/module.mk
