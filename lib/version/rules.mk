LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/version.c

# if no one else has defined it by now, build us a default buildid
# based on the current time.
# suffix it with _LOCAL if OFFICIAL_BUILD is unset
ifeq ($(strip $(BUILDID)),)
ifneq ($(OFFICIAL_BUILD),)
BUILDID := "$(shell $(LOCAL_DIR)/buildid.sh)"
else
BUILDID := "$(shell $(LOCAL_DIR)/buildid.sh)_LOCAL"
endif
endif

# Generate a buildid.h file, lazy evaulated BUILDID_DEFINE at the end
# of the first make pass. This lets modules that haven't been
# included yet set BUILDID.
BUILDID_DEFINE="BUILDID=\"$(BUILDID)\""
BUILDID_H := $(BUILDDIR)/buildid.h
$(BUILDID_H): .PHONY
	@$(call MAKECONFIGHEADER,$@,BUILDID_DEFINE)

GENERATED += $(BUILDID_H)

# make sure the module properly depends on and can find buildid.h
MODULE_SRCDEPS := $(BUILDID_H)

include make/module.mk
