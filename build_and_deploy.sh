#!/bin/bash
set -e

echo "[*] Building Kernel Module (AArch64 GKI 6.1)..."
cd /home/dev/projectMINT/stealth_lkm/stealthlkm_drv/kernel
make clean
make

echo "[*] Building C++ Daemon..."
cd ../daemon
make clean
make

echo "[*] Pushing binaries to the Android device..."
adb push ../kernel/stealthlkm.ko /data/local/tmp/
adb push stealth_daemon /data/local/tmp/
adb push engine_tester /data/local/tmp/ 2>/dev/null || true

echo "[*] Unloading old module (if any) and loading new stealthlkm.ko via su..."
adb shell su -c "rmmod stealthlkm 2>/dev/null || true"
adb shell su -c "insmod /data/local/tmp/stealthlkm.ko"

echo "[*] Target device Kernel Log (dmesg):"
adb shell su -c "dmesg | tail -n 15"

echo "[SUCCESS] Stealth-LKM is actively running!"
