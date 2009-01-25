# included from the main makefile to include a set of rules.mk to satisfy
# the current MODULE list. If as a byproduct of including the rules.mk
# more stuff shows up on the MODULE list, recurse

# sort and filter out any modules that have already been included
MODULES := $(sort $(MODULES))
MODULES := $(filter-out $(ALLMODULES),$(MODULES))

ifneq ($(MODULES),)

ALLMODULES += $(MODULES)
ALLMODULES := $(sort $(ALLMODULES))
INCMODULES := $(MODULES)
MODULES :=
$(info including $(INCMODULES))
include $(addsuffix /rules.mk,$(INCMODULES))

include make/module.mk

endif

