LK
==

The LK embedded kernel

Building
========
The code is structured into `project`, `target`, `platform`, and `arch`. You
can build the kernel by invoking `make <project>` where `project` is one of the
projects in project/ directory. For example, to build for vexpress-a9 platform,
use `make vexpress-a9-test`

Using local.mk
--------------
If you are building for the same platform again and again, you can
define the project in `local.mk` file by putting;

`PROJECT := vexpress-a9-test`

The build system reads local.mk and runs accordingly. After putting the
project information, `make` can be invoked alone to build the specific
project.

Turning compilation output
--------------------------
You can turn the compiler output by putting the following in your
local.mk;

`NOECHO := `

How To Contribute
=================
LK does not impose strict rules on contribution. As long as you contribute, it
is welcomed. However, there are suggestions that you may consider while writing
patches. These suggestions make the life easier for code reviewers and
maintainers.

Use git rebase workflow
-----------------------
Fork the repository and create a branch for your changes. Frequently, fetch the
upstream changes and rebase your branch on top of it. This workflow makes
merging the code easier for maintainers.

Write meaningful commit messages
--------------------------------
For each change, write a meaningful commit message. Summary the change in the
top line and explain the change, _if needed_, afterwards in detail. You can
look at commit messages in
[commits](https://github.com/travisg/lk/commits/master) section for a
reference.

You may want to prefix your one-line message with [xx] or [xx][yy] where xx and
yy are the directories that you worked in. These prefixes are not must but may
make the commit meaningful. If you feel that prefixes will help, you can use
them.

Keep your branch history clean
------------------------------
Remember that the changes and your messages are going to be part of lk master
when merged. These changes will be shown just as you pushed them. So, squash
the small commits if needed _(i.e. fix typo/fix grammar)_, re-order the commits
if required, and force push them in your branch. Thus, at the end of the day,
your branch can be easily merged with a clean history.

Send pull request
-----------------
When sending a pull request, explain your changes in general. Write your inline
code comments in **files changed** section rather than in a specific commit.
Inline comments made to a specific commit can disappear in the conversation tab
when you force push to your branch to keep the history clean.

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
