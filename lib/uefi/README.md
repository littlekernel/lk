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