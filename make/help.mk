# routines and rules to print some helpful stuff


#$(warning MAKECMDGOALS = $(MAKECMDGOALS))

# print some help and exit
ifeq ($(firstword $(MAKECMDGOALS)),help)
do-nothing=1

.PHONY: help
help:
	@echo "LK build system quick help\n" \
	"Individual projects are built into a build-<project> directory\n" \
	"Output binary is located at build-<project>/lk.bin\n" \
	"Environment or command line variables controlling build:\n" \
	"PROJECT = <project name>\n" \
	"TOOLCHAIN_PREFIX = <absolute path to toolchain or relative path with prefix>\n" \
	"\n" \
	"Special make targets:\n" \
	"make help: This help\n" \
	"make list: List of buildable projects\n" \
	"make clean: cleans build of current project\n" \
	"make spotless: removes all build directories\n" \
	"make <project>: try to build project named <project>\n" \
	"\n" \
	"Examples:\n" \
	"PROJECT=testproject make\n" \
	"PROJECT=testproject make clean\n" \
	"make testproject\n" \
	"make testproject clean\n" \
	"\n" \
	"output will be in build-testproject/\n" \

endif

# list projects
ifeq ($(firstword $(MAKECMDGOALS)),list)
do-nothing=1

# get a list of all the .mk files in the top level project directories
PROJECTS:=$(basename $(strip $(foreach d,$(LKINC),$(wildcard $(d)/project/*.mk))))
PROJECTS:=$(shell basename $(PROJECTS))

.PHONY: list
list:
	@echo 'List of all buildable projects: (look in project/ directory)'; \
	for p in $(PROJECTS); do \
		echo $$p; \
	done

endif

# vim: set syntax=make noexpandtab
