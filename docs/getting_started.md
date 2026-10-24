# Getting Started Guide

## Quick Start

1. Clone `lk`

```bash
mkdir -p ~/lk-work && cd ~/lk-work
git clone https://github.com/littlekernel/lk
cd lk
```

2. Download a toolchain. `scripts/fetch-toolchains.py` installs the latest prebuilt one for
   your host into `lk/toolchain/`. With no `--prefix` it fetches every architecture LK
   builds for; `--list` shows what would be fetched.

```bash
scripts/fetch-toolchains.py --prefix riscv64-elf
```

3. Add the toolchain's `bin` directory to your PATH

```bash
export PATH=$PWD/$(ls -d toolchain/riscv64-elf-*/bin | tail -1):$PATH
```

4. List the buildable projects

```bash
make list
```

5. Pick one, for example `qemu-virt-riscv64-test`, and build it. Output goes to
   `build-qemu-virt-riscv64-test/`.

```bash
make qemu-virt-riscv64-test

# Or build using Clang / Clang+LLD
make qemu-virt-riscv64-test TOOLCHAIN=clang
make qemu-virt-riscv64-test TOOLCHAIN=clang LD=ld.lld

# Or build using Clang with link-time optimization (thin or full)
make qemu-virt-riscv64-test TOOLCHAIN=clang LTO_MODE=thin
```

`TOOLCHAIN=clang` uses the `clang` on the PATH and the LLVM tools installed beside it, or
failing that the newest `/usr/lib/llvm-*/bin`. Clang is a cross compiler already, so no
target-specific toolchain is needed. Point `CLANG_BINDIR` at another LLVM install to use
that instead:

```bash
make qemu-virt-riscv64-test TOOLCHAIN=clang LD=ld.lld CLANG_BINDIR=/opt/llvm/bin
```

6. Run it under QEMU. The `do-qemu*` scripts rebuild the project first, then boot it;
   `-h` lists their options.

```bash
scripts/do-qemuriscv -6
```

Type `help` at the `]` prompt for the list of commands. `Ctrl-A x` exits QEMU.

## Build A Hello App

LK can build modules and projects that live outside its own tree. The outer directory gets
a copy of LK's top level `makefile` and an `lk_inc.mk` that says where everything is; `lk`
itself sits inside it, as a clone, a git submodule or a symlink.

```text
~/lk-work/
    lk/                    # the clone from the Quick Start
    mylk/
        lk -> ../lk
        makefile           # copied from lk/makefile
        lk_inc.mk          # copied from lk/lk_inc.mk.example
        project/project1.mk
        app/hello/hello.c
        app/hello/rules.mk
```

1. Create the workspace

```bash
cd ~/lk-work
mkdir -p mylk/project mylk/app/hello && cd mylk
ln -s ../lk lk
cp lk/makefile .
cp lk/lk_inc.mk.example lk_inc.mk
```

2. Edit `lk_inc.mk`. Set `LKINC` to `.`, so make searches this directory for modules and
   projects alongside `lk`, and set `DEFAULT_PROJECT`:

```makefile
LKMAKEROOT := .
LKINC := .
LKROOT := lk
BUILDROOT ?= .
DEFAULT_PROJECT ?= project1
```

3. Start a project from an existing one

```bash
cp lk/project/qemu-virt-riscv64-test.mk project/project1.mk
echo 'MODULES += app/hello' >> project/project1.mk
```

4. Write the app: one source file with a console command, and its `rules.mk`

```bash
cat << 'EOF' > app/hello/hello.c
#include <stdio.h>
#include <app.h>
#include <lk/console_cmd.h>

static int hello(int argc, const console_cmd_args *argv) {
    printf("hello world\n");
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("hello", "print a greeting", &hello)
STATIC_COMMAND_END(hello);

APP_START(hello)
APP_END
EOF

cat << 'EOF' > app/hello/rules.mk
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/hello.c

MODULE_DEPS += lib/console

include make/module.mk
EOF
```

5. Build and run it. The PATH from the Quick Start still applies. The scripts build and run
   whatever project `-p` names, from the current directory.

```bash
make project1
lk/scripts/do-qemuriscv -6 -p project1
```

The boot log ends at the shell prompt:

```text
welcome to lk/MP

boot args 0x0 0x87e00000 0x0 0x0
INIT: cpu 0, calling hook 0x8002d848 (version) at level 0x3fff, flags 0x1
version:
	arch:     riscv
	platform: qemu-virt-riscv
	target:   qemu-virt-riscv
	project:  project1
	buildid:  PAO6O_LOCAL
initializing heap
...
initializing platform
...
initializing target
initializing apps
...
starting app shell
entering main console loop
]
```

6. Try the new command

```text
] hello
hello world
```
