LK
==

The LK embedded kernel

Building
========
The code is structed into `project`, `target`, `platform`, and `arch`. You can
build the kernel by invoking `make <project>` where `project` is one
of the projects in project/ directory. For example, to build for
realview-pb platform, use `make realview-pb-test`

Using local.mk
--------------
If you are building for the same platform again and again, you can
define the project in `local.mk` file by putting;

`PROJECT := realview-pb-test`

The build system reads local.mk and runs accordingly. After putting the
project information, `make` can be invoked alone to build the specific
project.

Turning compilation output
--------------------------
You can turn the compiler output by putting the following in your
local.mk;

`NOECHO := `

How Build System Works
======================
The build system is modular, written from scratch, and able to handle
dependencies. Each module can set its own files to compile along with its
compiler flags. The core of the build system is `engine.mk`

Starting at `project/<project>/rules.mk`, it defines where target is.
``target/<target>/rules.mk`` defines platform. Platform defines the
*architecture*, *memory base*, *memory size*, *linker to use*, *its own sources*
and *dependencies*. In this stage, we will start seeing `MODULE_DEPS`. The build
system recurses and solves its dependencies on another module. The trick is
that each module sets its own files to compiler, its own *CFLAGS*, etc via
`MODULE_*` vars, then includes `build/module.mk`. ``module.mk``generates all the
build rules for that module, then clears the vars.

The order of the build and CFLAGS can be seen by setting `NOECHO`, which may help
to understand the build system.
