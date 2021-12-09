# A collection of handy macros used all over the build system.
# This can be included anywhere and must not have any side effects.

# Find the local dir of the make file
GET_LOCAL_DIR    = $(patsubst %/,%,$(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))))

# makes sure the target dir exists
MKDIR = if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi

# prepends the BUILD_DIR var to each item in the list
TOBUILDDIR = $(addprefix $(BUILDDIR)/,$(1))

# converts specified variable to boolean value
TOBOOL = $(if $(filter-out 0 false,$1),true,false)

COMMA := ,
E :=
SPACE := $E $E

# lower case and upper case translation
LC = $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))
UC = $(subst a,A,$(subst b,B,$(subst c,C,$(subst d,D,$(subst e,E,$(subst f,F,$(subst g,G,$(subst h,H,$(subst i,I,$(subst j,J,$(subst k,K,$(subst l,L,$(subst m,M,$(subst n,N,$(subst o,O,$(subst p,P,$(subst q,Q,$(subst r,R,$(subst s,S,$(subst t,T,$(subst u,U,$(subst v,V,$(subst w,W,$(subst x,X,$(subst y,Y,$(subst z,Z,$1))))))))))))))))))))))))))

# test if two files are different, replacing the first
# with the second if so
# args: $1 - temporary file to test
#       $2 - file to replace
define TESTANDREPLACEFILE
	if [ -f "$2" ]; then \
		if cmp -- "$1" "$2"; then \
			rm -f -- $1; \
		else \
			mv -- $1 $2; \
		fi \
	else \
		mv -- $1 $2; \
	fi
endef

# replace all characters or sequences of letters in defines to convert to a proper C style variable
MAKECVAR=$(subst C++,CPP,$(subst -,_,$(subst /,_,$(subst .,_,$1))))

# generate a header file at $1 with an expanded variable in $2
# $3 provides an (optional) raw footer to append to the end
# NOTE: the left side of the variable will be upper cased and some symbols replaced
# to be valid C names (see MAKECVAR above).
# The right side of the #define can be any valid C but cannot contain spaces, even
# inside a string.
define MAKECONFIGHEADER
	$(info generating $1) \
	$(MKDIR); \
	echo '#pragma once' > $1.tmp; \
	$(foreach var,$($(2)), \
		echo \#define \
		$(firstword $(subst =,$(SPACE),$(call MAKECVAR,$(call UC,$(var))))) \
		$(if $(findstring =,$(var)),$(subst $(firstword $(subst =,$(SPACE),$(var)))=,,$(var))) \
		>> $1.tmp;) \
	echo $3 >> $1.tmp; \
	$(call TESTANDREPLACEFILE,$1.tmp,$1)
endef

check_compiler_flag = $(shell $(CC) -c -xc /dev/null -o /dev/null $(1) 2>/dev/null && echo yes || echo no)
# Due to GCC's behaviour with regard to unknown warning flags this macro can
# only be used to detect warning-enable options (-Wfoo) but not for warning
# disable flags such as -Wno-foo.
# https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#:~:text=When%20an%20unrecognized%20warning%20option%20is%20requested
is_warning_flag_supported = $(strip \
    $(if $(findstring -Wno-,$(1)),$(error "Cannot use -Wno- flags here: $(1)"),) \
	$(if $(CC),,$(error "CC is not set, this macro cannot be used yet!")) \
	$(if $($(call MAKECVAR,$(1))), \
	    $(info Using cached result for $(1): $($(call MAKECVAR,$(1)))), \
	    $(eval $(call MAKECVAR,$(1)) := $(call check_compiler_flag,-Werror -fsyntax-only $(1))) \
	    $(info Checking if $(1) is supported: $($(call MAKECVAR,$(1)))) \
	 )$($(call MAKECVAR,$(1))))
