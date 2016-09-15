# the above include may override LKROOT and LKINC to allow external
# directories to be included in the build
-include lk_inc.mk

LKMAKEROOT ?= .
LKROOT ?= .
LKINC ?=
BUILDROOT ?= .
DEFAULT_PROJECT ?=
TOOLCHAIN_PREFIX ?=

# check if LKROOT is already a part of LKINC list and add it only if it is not
ifeq ($(filter $(LKROOT),$(LKINC)), )
LKINC := $(LKROOT) $(LKINC)
endif

# add the external path to LKINC
ifneq ($(LKROOT),.)
LKINC += $(LKROOT)/external
else
LKINC += external
endif

export LKMAKEROOT
export LKROOT
export LKINC
export BUILDROOT
export DEFAULT_PROJECT
export TOOLCHAIN_PREFIX

# veneer makefile that calls into the engine with lk as the build root
# if we're the top level invocation, call ourselves with additional args
_top:
	@$(MAKE) -C $(LKMAKEROOT) -rR -f $(LKROOT)/engine.mk $(addprefix -I,$(LKINC)) $(MAKECMDGOALS)

# If any arguments were provided, create a recipe for them that depends
# on the _top rule (thus calling it), but otherwise do nothing.
# "@:" (vs empty rule ";") prevents extra "'foo' is up to date." messages from
# being emitted.
$(MAKECMDGOALS): _top
	@:

.PHONY: _top
