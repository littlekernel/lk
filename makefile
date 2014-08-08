# the above include may override LKROOT and LKINC to allow external
# directories to be included in the build
-include lk_inc.mk

LKMAKEROOT ?= .
LKROOT ?= .
LKINC ?=
TOOLCHAIN_PREFIX ?=

LKINC := $(LKROOT) $(LKINC)

# vaneer makefile that calls into the engine with lk as the build root
# if we're the top level invocation, call ourselves with additional args
$(MAKECMDGOALS) _top:
	TOOLCHAIN_PREFIX=$(TOOLCHAIN_PREFIX) LKROOT=$(LKROOT) LKINC="$(LKINC)" $(MAKE) -C $(LKMAKEROOT) -rR -f $(LKROOT)/engine.mk $(addprefix -I,$(LKINC)) $(MAKECMDGOALS)

.PHONY: _top
