### Getting Started Guide

#### Quick Start

1- Create a work directory
```
mkdir -p lk-work && cd lk-work
```
2- Clone the repo
```
git clone https://github.com/littlekernel/lk
```
3- Download appropriate toolchain and extract it
```
wget https://newos.org/toolchains/riscv64-elf-12.1.0-Linux-x86_64.tar.xz

mkdir -p toolchain
tar xf riscv64-elf-12.1.0-Linux-x86_64.tar.xz
cd ..
```
4- Add toolchain to PATH
```
export PATH=$PWD/toolchain/riscv64-elf-12.1.0-Linux-x86_64/bin:$PATH
```
5- Change dir to lk to build and find available project
```
cd lk
ls project/*
```
6- E.g pick `qemu-virt-riscv64-test` and build kernel
```
make qemu-virt-riscv64-test
```
7- Test kernel with Qemu by using prepared script
```
scripts/do-qemuriscv -6
```

#### Build A Hello App
Continue from Quick Start's 3.step

4- Create your workspace and enter in it
```
mkdir -p mylk/{project,app} && cd mylk
```
5- Configure your main makefile; set your toolchain path and point up your lk main path correctly
```
cat << EOF > makefile
export PATH := /home/myuser/lk-work/toolchain/riscv64-elf-12.1.0-Linux-x86_64/bin:$(PATH)
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
6- Copy an exisiting project as your new project
```
cp ../lk/project/qemu-virt-riscv64-test.mk project/project1.mk
```
7- Create a new hello app under app directory
```
mkdir app/hello
```
```
cat << EOF > app/hello/hello.c
production:
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
```
cat << EOF > app/hello/rules.mk
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/hello.c

include make/module.mk
EOF
```
8- Add below line to project's makefile (project/project1.mk), your project will build also your app:
```
MODULES +=  app/hello
```

9- Check your directories structure
```
# mylk
tree 
.
├── app
│   └── hello
│       ├── hello.c
│       └── rules.mk
├── makefile
└── project
    └── project1.mk
```
```
# lk-work
tree ../ -d -L 1 
../.
├── lk
├── mylk
└── toolchain/riscv64-elf-12.1.0-Linux-x86_64

```
10- Build your project
```
make project1
```

11- Run your kernel with Qemu
```
qemu-system-riscv64 -machine virt -cpu rv64 -m 48 -smp 1 -bios none -nographic -kernel build-project1/lk.elf
```
You will see this output
```
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
] 
```
12- Test your hello application
```
] hello
hello world

```
