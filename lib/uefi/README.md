## Build

```
make qemu-virt-arm64-test
```

Note, this may fail if your system does not have `aarch64-elf-gcc` installed. To address, download from [here](https://newos.org/toolchains/aarch64-elf-14.2.0-Linux-x86_64.tar.xz), unzip, and add the extracted dir to PATH.

## Run

```
qemu-system-aarch64 -cpu max -m 512 -smp 1 -machine virt,highmem=off \
	-kernel build-qemu-virt-arm64-test/lk.elf \
	-net none -nographic \
	-drive if=none,file=lib/uefi/helloworld_aa64.efi,id=blk,format=raw \
	-device virtio-blk-device,drive=blk
```


Once you see the main console prompt, enter `uefi_load virtio0` to load the hello world UEFI application.

```
starting app shell
entering main console loop
] uefi_load virtio0
bio_read returns 4096, took 1 msecs (4096000 bytes/sec)
PE header machine type: aa64
Valid UEFI application found.
Entry function located at 0xffff000780067380
Hello World!
```

## Loader regression tests

The loader regressions are part of LK's `ut all` suite in test builds:

```sh
make qemu-virt-arm64-test TOOLCHAIN=clang LD=ld.lld IGNORE_LOCAL_MK=1
qemu-system-aarch64 -cpu max -m 512 -smp 1 -machine virt,highmem=off \
    -kernel build-qemu-virt-arm64-test/lk.elf -net none -nographic \
    -monitor none -append 'lk.autorun=ut+all;poweroff'
```

The `uefi_loader_tests` case embeds the existing Hello World PE and generates
valid layout variants and malformed headers, section ranges, and entry points
in memory. Each fixture is loaded twice through a private read-only BIO device.
After one valid-load warmup for the kernel heap, tests assert the loader's return
status, unchanged PMM free-page count, and BIO reference count directly; they do
not match diagnostic strings. Valid fixtures execute the EFI payload through
the real loader, including unrounded image/raw-data sizes and empty section
placeholders. Malformed layout cases must fail after only the initial header
read, rather than accidentally failing later while loading image data. Truncated fixtures exercise
the read-error cleanup paths. No external disk or Python fixture runner is needed.

Use `ut uefi_loader_tests` in the LK console to run just these tests. The virtio
disk command above remains useful as a hardware-path smoke test. Unlike the old
per-fixture QEMU runner, an unexpected guest crash stops the current suite.

This is bounds/format regression coverage, not image authentication or a sandbox
for executing untrusted UEFI applications. The loader still requires the PE
headers and section table to fit in its initial 4 KiB read.

For compatibility with existing EFI producers, the loader accepts SizeOfImage
and SizeOfRawData that are not padded to their declared alignment, and ignores
zero-address, zero-length placeholder sections. Image allocation is still rounded
to native pages; all nonempty sections remain bounded by SizeOfImage and must not
overlap headers or previous sections.
