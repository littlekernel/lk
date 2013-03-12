# the above include may override LKROOT and LKINC to allow external
# directories to be included in the build
-include lk_inc.mk

LKROOT ?= .
LKINC ?=

LKINC := $(LKROOT) $(LKINC)

# vaneer makefile that calls into the engine with lk as the build root
# if we're the top level invocation, call ourselves with additional args
$(MAKECMDGOALS) _top:
	LKROOT=$(LKROOT) LKINC="$(LKINC)" $(MAKE) -rR -f $(LKROOT)/engine.mk $(addprefix -I,$(LKINC)) $(MAKECMDGOALS)

.PHONY: _top
