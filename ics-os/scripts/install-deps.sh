#!/bin/bash
# Install the dependencies for a modern Ubuntu host (tested on 24.04).
set -e
sudo apt-get update
sudo apt-get install -y \
    build-essential gcc-multilib libc6-dev-i386 nasm python3 \
    qemu-system-x86 qemu-utils \
    tcc git grub-pc-bin grub-efi-amd64-bin xorriso mtools fdisk ovmf
