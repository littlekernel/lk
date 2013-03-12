# Find the local dir of the make file
GET_LOCAL_DIR    = $(patsubst %/,%,$(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))))

# makes sure the target dir exists
MKDIR = if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi

# prepends the BUILD_DIR var to each item in the list
TOBUILDDIR = $(addprefix $(BUILDDIR)/,$(1))

COMMA := ,
SPACE :=
SPACE +=

# generate a header file at $1 with an expanded variable in $2
define MAKECONFIGHEADER
	@$(MKDIR)
	@echo generating $1
	@rm -f $1.tmp; \
	LDEF=`echo $1 | tr '/\\.-' '_'`; \
	echo \#ifndef __$${LDEF}_H > $1.tmp; \
	echo \#define __$${LDEF}_H >> $1.tmp; \
	for d in `echo $($2) | tr '[:lower:]' '[:upper:]'`; do \
		echo "#define $$d" | sed "s/=/\ /g;s/-/_/g;s/\//_/g;s/\./_/g;s/\//_/g" >> $1.tmp; \
	done; \
	echo \#endif >> $1.tmp; \
	if [ -f "$1" ]; then \
		if cmp "$1.tmp" "$1"; then \
			rm -f $1.tmp; \
		else \
			mv $1.tmp $1; \
		fi \
	else \
		mv $1.tmp $1; \
	fi
endef
