#******************************************************************************
#
# Makefile - Rules for building the driver library.
#
# Copyright (c) 2005-2020 Texas Instruments Incorporated.  All rights reserved.
# Software License Agreement
# 
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
# 
#   Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# 
#   Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the  
#   distribution.
# 
#   Neither the name of Texas Instruments Incorporated nor the names of
#   its contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# This is part of revision 2.2.0.295 of the Tiva Peripheral Driver Library.
#
#******************************************************************************

#
# The base directory for TivaWare.
#
ROOT=..

#
# Include the common make definitions.
#
include ${ROOT}/makedefs

#
# Where to find header files that do not live in the source directory.
#
IPATH=..

#
# The default rule, which causes the driver library to be built.
#
all: ${COMPILER}
all: ${COMPILER}/libdriver.a

#
# The rule to clean out all the build products.
#
clean:
	@rm -rf ${COMPILER} ${wildcard *~}

#
# The rule to create the target directory.
#
${COMPILER}:
	@mkdir -p ${COMPILER}

#
# Rules for building the driver library.
#
${COMPILER}/libdriver.a: ${COMPILER}/adc.o
${COMPILER}/libdriver.a: ${COMPILER}/aes.o
${COMPILER}/libdriver.a: ${COMPILER}/can.o
${COMPILER}/libdriver.a: ${COMPILER}/comp.o
${COMPILER}/libdriver.a: ${COMPILER}/cpu.o
${COMPILER}/libdriver.a: ${COMPILER}/crc.o
${COMPILER}/libdriver.a: ${COMPILER}/des.o
${COMPILER}/libdriver.a: ${COMPILER}/eeprom.o
${COMPILER}/libdriver.a: ${COMPILER}/emac.o
${COMPILER}/libdriver.a: ${COMPILER}/epi.o
${COMPILER}/libdriver.a: ${COMPILER}/flash.o
${COMPILER}/libdriver.a: ${COMPILER}/fpu.o
${COMPILER}/libdriver.a: ${COMPILER}/gpio.o
${COMPILER}/libdriver.a: ${COMPILER}/hibernate.o
${COMPILER}/libdriver.a: ${COMPILER}/i2c.o
${COMPILER}/libdriver.a: ${COMPILER}/interrupt.o
${COMPILER}/libdriver.a: ${COMPILER}/lcd.o
${COMPILER}/libdriver.a: ${COMPILER}/mpu.o
${COMPILER}/libdriver.a: ${COMPILER}/onewire.o
${COMPILER}/libdriver.a: ${COMPILER}/pwm.o
${COMPILER}/libdriver.a: ${COMPILER}/qei.o
${COMPILER}/libdriver.a: ${COMPILER}/shamd5.o
${COMPILER}/libdriver.a: ${COMPILER}/ssi.o
${COMPILER}/libdriver.a: ${COMPILER}/sw_crc.o
${COMPILER}/libdriver.a: ${COMPILER}/sysctl.o
${COMPILER}/libdriver.a: ${COMPILER}/sysexc.o
${COMPILER}/libdriver.a: ${COMPILER}/systick.o
${COMPILER}/libdriver.a: ${COMPILER}/timer.o
${COMPILER}/libdriver.a: ${COMPILER}/uart.o
${COMPILER}/libdriver.a: ${COMPILER}/udma.o
${COMPILER}/libdriver.a: ${COMPILER}/usb.o
${COMPILER}/libdriver.a: ${COMPILER}/watchdog.o

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
