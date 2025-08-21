# GDB debug support for GBL in LK

## Overview

This document describes how to use GDB to debug GBL and using the UEFI
debug support protocol.

## Build GBL

First, we need to build the GBL with the following steps.

### Download GBL

 * GBL_MANIFEST_URL_DEFAULT="https://android.googlesource.com/kernel/manifest/"
 * GBL_MANIFEST_BRANCH_DEFAULT="uefi-gbl-mainline"
 * mkdir -p gbl
 * cd gbl
 * repo init -u ${GBL_MANIFEST_URL} -b ${GBL_MANIFEST_BRANCH}
 * repo sync

### Build GBL with Debug Target

 * REPO_ROOT=$(readlink -f .)
 * BAZEL_TARGET="@gbl//efi:aarch64_debug_dev"
 * BAZEL_OUT_BASE="${REPO_ROOT}/out_aarch64_debug_dev"
 * # Build GBL
 * "${REPO_ROOT}/tools/bazel" "--output_base=${BAZEL_OUT_BASE}" build "${BAZEL_TARGET}" --sandbox_debug --verbose_failures --symlink_prefix=/
 * # Copy GBL out of the directory
 * BAZEL_OUT_BIN=$("${REPO_ROOT}/tools/bazel" cquery "${BAZEL_TARGET}" --output files 2>/dev/null)
 * BAZEL_OUT_BIN=$(readlink -f "${BAZEL_OUT_BASE}/execroot/_main/${BAZEL_OUT_BIN}")
 * GBL_DBG_BIN="${BAZEL_OUT_BASE}/gbl.efi"
 * cp "${BAZEL_OUT_BIN}" "${GBL_DBG_BIN}"
 * # pack debug symbols
 * DWO_OUT="${BAZEL_OUT_BASE}/dwo"
 * mkdir -p "${DWO_OUT}"
 * find "${BAZEL_OUT_BASE}" -name *.dwo -not -path "${DWO_OUT}*" -exec mv -f {} "${DWO_OUT}"/ \; 2>/dev/null || true
 * llvm-dwp -o "${GBL_DBG_BIN}.dwp" $(ls "${DWO_OUT}"/*.dwo)

## Run LK with QEmu and load GBL.

### Run qemu

 * APP1=/path/to/gbl.efi
 * qemu-system-aarch64 -machine virt \
     -m 1024M \
     -cpu cortex-a57 \
     -nographic \
     -kernel build-qemu-virt-arm64-test/lk.elf \
     -drive if=none,file=${APP1},id=blk1,format=raw \
     -gdb tcp::13333

### Run GBL

 In the LK prompt. Use the following commands.

 * uefi_set_var gbl_debug 1
 * uefi_load virtio0

 And we will see the following message.

```
bio_read returns 4096, took 5 msecs (819200 bytes/sec)
PE header machine type: aa64
Valid UEFI application found.
Failed to allocate physical memory size 7073792 at specified address 0x140000000 fallback to regular allocation
Relocating image from 0x140000000 to 0x53600000
Entry function located at 0x53604db0
platform_setup_system_table is called
Calling kernel with stack [0x54800000, 0x548fffff]
locate_handle_buffer(0x2, (0x3152bca5 0xeade 0x433d 0x441f29dc1cc02e86), search_key=0)
Required protocol not found: EfiRngProtocol
SECURITY WARNING: Failed to generate stack canary, using static default: Unsupported
open_protocol(LOADED_IMAGE_PROTOCOL_GUID, handle=0x53600000, agent_handle=0x53600000, controller_handle=0, attr=0x1)
Image base: 0x53600000
Please run load_gbl_debug_bin.py or set $x0=0 from gdb to continue.
```

## Use GDB to debug.

### Use GDB to load debug symbols of GBL manually.

Since GBL prints its own address. We can load the debug symbols based on
that address.

The above shows the Image base address is 0x53600000

We can use the command "readpe gbl.efi". Look at the "Virtual Address" field.
And we can get the following offsets.

 * .text: 0x1000
 * .data: 0x392000
 * .rdata: 0x315000

So the actual address is by adding the base address (0x53600000)
to each of the address above. Then it becomes:

 * .text: 0x53601000
 * .data: 0x53992000
 * .rdata: 0x53915000

Then we can run "gdb-multiarch". In the GDB prompt we use the following
commands.

 * target remote :13333
 * add-symbol-file /path/to/gbl.efi 0x53601000 -s .data 0x53992000 -s .rdata 0x53915000

And use "bt full" we can see the following output.

```
#0  0x0000000053604d7c in app::wait_gdb (entry=0x548ffe30)
    at external/gbl/efi/app/main.rs:145
        buf = [49]
        image_base = 1398800384
#1  0x0000000053604e80 in app::efi_main (image_handle=0x53600000, 
    systab_ptr=0x5340f000) at external/gbl/efi/app/main.rs:184
        canary = 2812601071695653757
        entry = efi::EfiEntry {image_handle: 0x53600000, systab_ptr: 0x5340f000}
#2  0xffff00000012dcfc in ?? ()
No symbol table info available.
#3  0xffff00000012ab10 in ?? ()
No symbol table info available.
Backtrace stopped: previous frame inner to this frame (corrupt stack?)
```

Then we can use "set $x0=0" in GDB to let the GBL continue to run.

### Use UEFI Debug support protocol.

By loading python extensions we made, we can make the above things easier.

In GDB, we load the python extension.

 * target remote :13333
 * source gdb_efi_apps.py

And we will have several "efi" commands.
 * efi showvendor    # show the vendor of the bootloader.
 * efi table         # show the configuration tables of the UEFI
 * efi test list     # list of the current loading UEFI apps
 * efi loadsymbols   # load debug symbols

```
(gdb) efi showvendor
Firmware Vendor: Little Kernel
Firmware Version: 0
(gdb) efi table
 1CE1E5BC-7CEB-42F2-81E5-8AADF180F57B: VendorTable = 0x53411000
 B1B621D5-F19C-41A5-830B-D9152C69AAE0: VendorTable = 0x53412000
          gEfiDebugImageInfoTableGuid: VendorTable = 0xffff0000001767a8
(gdb) efi test list
\virtio0: addr: 0x53600000, size: 0x6bf000, path:None
(gdb) efi loadsymbols -m virtio0 /path/to/gbl.efi /path/to/gbl.efi
LoadedImage address: 0x53600000, size: 0x6bf000
PE text offset: 0x1000
PE data offset: 0x392000
PE rdata offset: 0x315000
(gdb) bt full
#0  0x0000000053604d7c in app::wait_gdb (entry=0x548ffe30)
    at external/gbl/efi/app/main.rs:145
        buf = [49]
        image_base = 1398800384
#1  0x0000000053604e80 in app::efi_main (image_handle=0x53600000, 
    systab_ptr=0x5340f000) at external/gbl/efi/app/main.rs:184
        canary = 2812601071695653757
        entry = efi::EfiEntry {image_handle: 0x53600000, systab_ptr: 0x5340f000}
#2  0xffff00000012dcfc in ?? ()
No symbol table info available.
#3  0xffff00000012ab10 in ?? ()
No symbol table info available.
Backtrace stopped: previous frame inner to this frame (corrupt stack?)
```

The GDB python extension is currently put here.

 * https://git.codelinaro.org/paul_liu/test-e5e925b80fd7/-/blob/clo/main/verify_gbl_on_qemu/gdb_efi_apps.py?ref_type=heads

It needs EDK2's python extension scritps. And we are currently trying
to upstream this extension to EDK2.

 * https://github.com/tianocore/edk2/pull/11285
