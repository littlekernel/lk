# routines and rules to print some helpful stuff


#$(warning MAKECMDGOALS = $(MAKECMDGOALS))

# print some help and exit
ifeq ($(firstword $(MAKECMDGOALS)),help)
do-nothing=1

.PHONY: help
help:
	@echo "LK build system quick help"
	@echo "Individual projects are built into a build-<project> directory"
	@echo "Output binary is located at build-<project>/lk.bin"
	@echo "Environment or command line variables controlling build:"
	@echo "PROJECT = <project name>"
	@echo "TOOLCHAIN_PREFIX = <absolute path to toolchain or relative path with prefix>"
	@echo ""
	@echo "Special make targets:"
	@echo "make help: This help"
	@echo "make list: List of buildable projects"
	@echo "make clean: cleans build of current project"
	@echo "make spotless: removes all build directories"
	@echo "make <project>: try to build project named <project>"
	@echo ""
	@echo "Examples:"
	@echo "PROJECT=testproject make"
	@echo "PROJECT=testproject make clean"
	@echo "make testproject"
	@echo "make testproject clean"
	@echo ""
	@echo "output will be in build-testproject/"

endif

# list projects
ifeq ($(firstword $(MAKECMDGOALS)),list)
do-nothing=1

# get a list of all the .mk files in the top level project directories
PROJECTS:=$(basename $(strip $(foreach d,$(LKINC),$(wildcard $(d)/project/*.mk))))
PROJECTS:=$(shell basename -a $(PROJECTS))

.PHONY: list
list:
	@echo 'List of all buildable projects: (look in project/ directory)'; \
	for p in $(PROJECTS); do \
		echo $$p; \
	done

endif

# vim: set syntax=make noexpandtab
