# Getting Started Guide

## Quick Start

This guide assumes the following simple layout where your workspace (for example `mylk`) is a sibling of the cloned `lk` repository and the fetched toolchains live under a `toolchain` directory in the same parent folder:

~/lk-work/
    lk/                # cloned littlekernel repository
        toolchain/     # fetched toolchains (created by scripts/fetch-toolchains.py)
    mylk/              # example user workspace (created below)

1. Create a work directory and clone `lk`

```bash
mkdir -p ~/lk-work && cd ~/lk-work
git clone https://github.com/littlekernel/lk
```

2. Download an appropriate toolchain (this will create a `toolchain/` directory under `lk-work/lk/`)

```bash
# Fetches the latest riscv64-elf toolchain for your host.
cd lk
scripts/fetch-toolchains.py --prefix riscv64-elf
```

3. Add the toolchain `bin` directory to your PATH. Replace the example below with the actual toolchain folder you have downloaded.

```bash
export PATH=~/lk-work/lk/toolchain/riscv64-elf-15.1.0-Linux-x86_64/bin:$PATH
```

4. Find available projects

```bash
ls project/*
```

5. For example pick `qemu-virt-riscv64-test` and build the kernel

```bash
make qemu-virt-riscv64-test
```

6. Test the kernel with Qemu using the provided script

```bash
scripts/do-qemuriscv -6S
```

## Build A Hello App
Continue from Quick Start's steps above. Create your workspace as a sibling of the cloned `lk` repository and enter it (note the `cd ..` to move to the parent of `lk`):

```bash
cd ..   # make sure you're in the parent directory that contains the cloned `lk`
mkdir -p mylk/{project,app} && cd mylk
```

1. Configure your main makefile; set your toolchain path and point `LKROOT` to the cloned `lk` repository. Replace the PATH example with the actual toolchain path on your system.

```bash
cat << 'EOF' > makefile
export PATH := /path/to/your/toolchain/bin:$(PATH)  # e.g. ~/lk-work/toolchain/riscv64-elf-14.2.0-Linux-x86_64/bin
-include lk_inc.mk
LOCAL_DIR := .
LKMAKEROOT := .
LKROOT := ../lk
LKINC := $(LOCAL_DIR)
DEFAULT_PROJECT ?= myqr
BUILDROOT ?= $(LOCAL_DIR)

ifeq ($(filter $(LKROOT),$(LKINC)), )
LKINC := $(LKROOT) $(LKINC)
endif

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
_top:
    @$(MAKE) -C $(LKMAKEROOT) -rR -f $(LKROOT)/engine.mk $(addprefix -I,$(LKINC)) $(MAKECMDGOALS)
$(MAKECMDGOALS): _top
    @:
.PHONY: _top
EOF
```
2. Copy an existing project as your new project

```bash
cp ../lk/project/qemu-virt-riscv64-test.mk project/project1.mk
```

3. Create a new hello app under the `app` directory

```bash
mkdir app/hello
```
```c
cat << 'EOF' > app/hello/hello.c
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <app.h>
#include <platform.h>
#include <lk/console_cmd.h>

static int hello(int argc, const console_cmd_args *argv) {
    printf("hello world\n");
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("hello", "hello tests", &hello)
STATIC_COMMAND_END(hello);

APP_START(hello)
APP_END
EOF
```
```makefile
cat << 'EOF' > app/hello/rules.mk
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/hello.c

include make/module.mk
EOF
```

4. Add the following line to your project's makefile (`project/project1.mk`) so the project builds your app as well:

```makefile
MODULES += app/hello
```

5. Check your directory structure

```bash
# mylk
tree
.
├── app
│   └── hello
│       ├── hello.c
│       └── rules.mk
├── makefile
└── project
        └── project1.mk

# lk-work
tree ../ -d -L 1
../.
├── lk
└── mylk

```

6. Build your project

```bash
make project1
```

7. Run your kernel with Qemu

```bash
qemu-system-riscv64 -machine virt -cpu rv64 -m 48 -smp 1 -bios none -nographic -kernel build-project1/lk.elf
```

You will see output similar to:

```text
FDT: found memory arena, base 0x80000000 size 0x3000000
FDT: found 1 cpus

welcome to lk/MP

boot args 0x0 0x1020 0x0 0x0
INIT: cpu 0, calling hook 0x8001e3b8 (version) at level 0x3ffff, flags 0x1
version:
	arch:     riscv
	platform: qemu-virt-riscv
	target:   qemu-virt-riscv
	project:  project1
	buildid:  L96DH_LOCAL
initializing heap
calling constructors
initializing mp
initializing threads
initializing timers
initializing ports
creating bootstrap completion thread
top of bootstrap2()
INIT: cpu 0, calling hook 0x8001a848 (minip) at level 0x70000, flags 0x1
INIT: cpu 0, calling hook 0x8001b6c6 (pktbuf) at level 0x70000, flags 0x1
pktbuf: creating 256 pktbuf entries of size 1536 (total 393216)
INIT: cpu 0, calling hook 0x8001fe08 (virtio) at level 0x70000, flags 0x1
RISCV: percpu cpu num 0x0 hart id 0x0
RISCV: Machine mode
RISCV: mvendorid 0x0 marchid 0x0 mimpid 0x0 mhartid 0x0
RISCV: misa 0x800000000014112d
RISCV: Releasing 0 secondary harts from purgatory
initializing platform
PCIE: initializing pcie with ecam at 0x30000000 found in FDT
PCI: pci ecam functions installed
PCI: last pci bus is 255
PCI dump:
    bus 0
     dev 0000:00:00.0 vid:pid 1b36:0008 base:sub:intr 6:0:0
PCI dump post assign:
    bus 0
     dev 0000:00:00.0 vid:pid 1b36:0008 base:sub:intr 6:0:0
initializing target
initializing apps
starting app inetsrv
starting internet servers
starting app shell
entering main console loop
```

8. Test your hello application

```bash
] hello
hello world

```

