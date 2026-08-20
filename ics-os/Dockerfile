############################################################
# ICS-OS Development (Ubuntu 24.04 host image)
############################################################
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN dpkg --add-architecture i386 \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        gcc-multilib \
        libc6-dev-i386 \
        nasm \
        qemu-system-x86 \
        qemu-utils \
        tcc \
        git \
        sudo \
        python3 \
        mtools \
        fdisk \
        xorriso \
        grub-pc-bin \
        grub-efi-amd64-bin \
        ovmf \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /home/ics-os
WORKDIR /home/ics-os
