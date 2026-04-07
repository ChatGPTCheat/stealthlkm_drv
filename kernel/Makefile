# SPDX-License-Identifier: GPL-2.0
#
# Makefile for stealth_analyzer LKM (AArch64 / Android GKI 6.1)
#
# The target kernel was built with clang + LLD (Android NDK toolchain).
# We must use the same toolchain to avoid ABI / BTF / modversions mismatches.
#
# Usage:
#   make          — cross-compile the module
#   make clean    — remove build artifacts
#

obj-m := stealthlkm.o
stealthlkm-objs := stealth_analyzer.o stealth_simd_core.o stealth_spe.o

# Force the compiler to enable NEON/SIMD intrinsics for the core logic
CFLAGS_stealth_simd_core.o := -march=armv8-a+simd -O3 -fno-tree-vectorize -isystem /opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/lib/clang/21/include
CFLAGS_REMOVE_stealth_simd_core.o := -mgeneral-regs-only

# Path to Android GKI 6.1 kernel source tree
KDIR := /home/dev/projectMINT/android-kernels/6.1/

# Android NDK LLVM toolchain
LLVM_BIN := /opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/

ARCH := arm64

MAKE_ARGS := \
	ARCH=$(ARCH) \
	LLVM=1 \
	LLVM_IAS=1 \
	CC=$(LLVM_BIN)clang \
	LD=$(LLVM_BIN)ld.lld \
	AR=$(LLVM_BIN)llvm-ar \
	NM=$(LLVM_BIN)llvm-nm \
	OBJCOPY=$(LLVM_BIN)llvm-objcopy \
	OBJDUMP=$(LLVM_BIN)llvm-objdump \
	STRIP=$(LLVM_BIN)llvm-strip

all: modules

modules:
	$(MAKE) $(MAKE_ARGS) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) $(MAKE_ARGS) -C $(KDIR) M=$(PWD) clean
