LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

PLATFORM := nuclei-hbird
WITH_LINKER_GC ?= 1

MEMSIZE ?= 0x10000     # 64KB
GLOBAL_DEFINES += TARGET_HAS_DEBUG_LED=1

# target code will set the master frequency to 16Mhz
GLOBAL_DEFINES += SOC_FREQ=32000000 DOWNLOAD_MODE=DOWNLOAD_MODE_ILM

MODULE_SRCS := $(LOCAL_DIR)/target.c

# set some global defines based on capability
GLOBAL_DEFINES += CONSOLE_ENABLE_HISTORY=0
GLOBAL_DEFINES += PLATFORM_HAS_DYNAMIC_TIMER=1
GLOBAL_DEFINES += ARCH_RISCV_CLINT_BASE=0x02000000
GLOBAL_DEFINES += ARCH_RISCV_MTIME_RATE=32768

OPENOCD_CFG := $(LOCAL_DIR)/openocd_hbird.cfg

OPENOCD_ARGS += -f $(OPENOCD_CFG)

GDB_UPLOAD_ARGS ?= --batch
GDB_UPLOAD_CMDS += -ex "monitor halt"
GDB_UPLOAD_CMDS += -ex "monitor flash protect 0 0 last off"
GDB_UPLOAD_CMDS += -ex "load"
GDB_UPLOAD_CMDS += -ex "monitor resume"
GDB_UPLOAD_CMDS += -ex "quit" 

include make/module.mk

