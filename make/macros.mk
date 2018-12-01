# Find the local dir of the make file
GET_LOCAL_DIR    = $(patsubst %/,%,$(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))))

# makes sure the target dir exists
MKDIR = if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi

# prepends the BUILD_DIR var to each item in the list
TOBUILDDIR = $(addprefix $(BUILDDIR)/,$(1))

# converts specified variable to boolean value
TOBOOL = $(if $(filter-out 0 false,$1),true,false)

COMMA := ,
SPACE :=
SPACE +=

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
