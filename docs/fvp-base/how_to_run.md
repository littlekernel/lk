# Build and run Fixed Virtual Platform (FVP)

This guide introduces how to build and run little kernel on FVP Base platform
for Linux users.

Free-of-charge AEM FVPs can be downloaded from Arm Architecture Models on Arm
Developer without a license. They are available for Linux hosts only.
 
For details read [Introductions FVP
docs](https://developer.arm.com/documentation/100966/latest/Introduction-to-FVPs/Types-of-FVP)

1- Download FVP Base

[FVP_Base_RevC-2xAEMvA_11.xx_x_Linux64](https://developer.arm.com/Tools%20and%20Software/Fixed%20Virtual%20Platforms/Arm%20Architecture%20FVPs)

2- Clone the repo

```shell
git clone https://github.com/littlekernel/lk
cd lk
```

3- Download appropriate toolchain

```shell
# Fetches the latest aarch64-elf toolchain for your host.
scripts/fetch-toolchains.py --prefix aarch64-elf
``` 

4- Add toolchain to PATH

```shell
export PATH=$PWD/toolchain/aarch64-elf-14.2.0-Linux-x86_64/bin:$PATH
```

5- Build kernel

```shell
make fvp-base-test
```

6- Clone Trusted Firmware-A (TF-A)

```shell
cd ..
git clone https://github.com/ARM-software/arm-trusted-firmware.git 
cd arm-trsuted-firmware
```

7- Build Trusted Firmware-A (TF-A)

```shell
make \
   BL33=../lk/build-fvp-base-test/lk.bin \
   DEBUG=1 \
   FVP_USE_GIC_DRIVER=FVP_GICV2 \
   LOG_LEVEL=40 \
   MEASURED_BOOT=0 \
   CROSS_COMPILE=aarch64-linux-gnu- \
   PLAT=fvp all fip
```

For details of build options you can found at [TF-A build options](https://trustedfirmware-a.readthedocs.io/en/latest/getting_started/build-options.html)

8- Run FVP Base

```shell
/path/to/FVP_Base_RevC-2xAEMvA_11.xx_xx_Linux64/Base_RevC_AEMvA_pkg/models/Linux64_GCC-9.3/FVP_Base_RevC-2xAEMvA \
   -C bp.ve_sysregs.exit_on_shutdown=1 \
   -C cache_state_modelled=0 \
   -C pctl.startup=0.0.0.0 \
   -C gicv3.gicv2-only=1 \
   -C bp.pl011_uart0.out_file=log.txt \
   -C bp.secure_memory=1 \
   -C cluster0.NUM_CORES=1 \
   -C cluster0.has_arm_v8-4=1 \
   -C cluster1.NUM_CORES=0 \
   -C bp.secureflashloader.fname=path/to/arm-trusted-firmware/build/fvp/debug/bl1.bin \
   -C bp.flashloader0.fname=path/to/arm-trusted-firmware/build/fvp/debug/fip.bin
```
