![Alt Text](./ics-os.gif)

## About

Modern real-world operating systems are too complex to be taught to undergraduates and other instructional operating systems are not complete and usable and do not work on real hardware. By providing students with a _not so complex_ working operating system to play with, they will be able to appreciate and understand deeper the concepts underlying an operating system.

Thus, this project aims to develop a simple yet operational instructional operating system for teaching undergraduate operating systems courses. ICS-OS is a fork of <a href='http://sourceforge.net/projects/dex-os'>DEX-OS</a> by Joseph Dayo.

ICS-OS remains a 32-bit protected-mode kernel. It boots on 64-bit (AMD64) PCs in legacy/compatibility mode via GRUB, including from a USB thumb drive that is then mounted as the root filesystem.

This tree (`ics-os-ex`) extends the instructional base with **in-OS TinyCC self-hosting**, a **POSIX-ish user libc**, **FAT32/IDE/USB I/O performance work**, and **ELF executable loading** suitable for compiling programs inside the OS.

## Downloads

Latest floppy image: <a href='https://github.com/srg-ics-uplb/ics-os/raw/master/ics-os/ics-os-floppy.img'>ics-os-floppy.img</a>

Test the floppy image in qemu.
```
$ qemu-system-i386 -fda ics-os-floppy.img
```

## Build Environment

The kernel builds on modern 64-bit Ubuntu (tested on **Ubuntu 24.04** with gcc 13) using `gcc-multilib`. Docker is optional.

Install host packages:

```
$ ./ics-os/scripts/install-deps.sh
```

### Native build (recommended)

```
$ cd ics-os
$ make clean
$ make
$ make usb
```

Boot the USB image in QEMU on a 64-bit CPU, with the thumb drive as the root filesystem:

```
$ make boot-usb-amd64
```

To emulate a real UHCI USB stick (the kernel's USB driver mounts it as `/icsos`):

```
$ make boot-usb-storage
```

Headless smoke tests (serial console, no VGA window):

```
$ make test-usb-amd64
$ make test-usb-storage
$ make test-exec          # load host-built hello.exe
$ make test-selfhost      # in-OS TinyCC compile + run
$ make test-iobench       # file I/O / block-cache microbenchmark
```

Write `ics-os-usb.img` to a physical thumb drive (BIOS/CSM firmware can boot it as a disk; the kernel also has a UHCI USB mass-storage driver):

```
$ sudo dd if=ics-os-usb.img of=/dev/sdX bs=4M status=progress conv=fsync
```

For a firmware-bootable hybrid image that works like `dd` of a live USB (BIOS and UEFI):

```
$ make livecd
$ sudo dd if=ics-os-livecd.iso of=/dev/sdX bs=4M status=progress conv=fsync
$ make boot-livecd-amd64
```

The FAT USB image is mounted as `/icsos` (the root filesystem). The live CD uses ISO9660 (`cdfs`) instead.

### Floppy and live CD (legacy)

```
$ sudo make floppy
$ make boot-floppy
```

```
$ make livecd
$ make boot-livecd
```

### Using Docker to build

ICS-OS is a 32-bit operating system and requires a 32-bit toolchain (`gcc -m32`). You need to install
[docker](https://docs.docker.com/engine/install/ubuntu/) and [docker-compose](https://docs.docker.com/compose/install/)
to build inside a container.

Run the following command to enter the build environment:

`$docker-compose run ics-os-build`

or if you are using the docker-compose plugin:

`$docker compose run ics-os-build`

You will be dropped to a shell where you can perform the build. The ics-os folder is mapped inside the container. Thus,
you can perform the edits outside the container(in another terminal) and the changes will be reflected inside the build environment.

```
#cd /home/ics-os
#make clean
#make
#make usb
#exit
```

See [Lab 01](https://github.com/srg-ics-uplb/ics-os/blob/master/labs/lab01/ICSOS_Lab01.pdf) for a more complete discussion of how
to set up the build environment.

## Current enhancements (ics-os-ex)

### In-OS TinyCC and self-hosting

- Vendored **TinyCC 0.9.27** (i386) under `ics-os/contrib/tcc/`, host-built as `apps/tcc.exe`.
- POSIX-oriented user headers in `ics-os/sdk/include/` plus `posix.c`, `setjmp.c`, and the existing DexSDK.
- Console commands:
  - `selfhost` — compile `min.c` / tiny `hello` with in-OS tcc and run them (`SELFHOST_TEST_PASS`).
  - `exectest` — smoke-test ELF loading of host-built `hello.exe`.
  - `cc` / `kbuild` / `tccboot` — compile user programs, rebuild the kernel image, or rebuild TinyCC inside the OS.
  - `iobench` — sequential file-read / block-cache benchmark (`IOBENCH_PASS`).
- Staging script `scripts/stage-selfhost.sh` copies compiler sources, SDK headers, and kernel sources onto the USB image for in-OS work.
- Larger ramdisk (`autoexec.bat`) for build scratch space under `/ramdisk`.

Typical in-OS flow after `make usb` / `make boot-usb-amd64`:

```
exectest
selfhost
iobench
tccboot          # long-running TinyCC self-rebuild (optional)
```

### I/O performance

Sequential reads of large files (e.g. the ~210 KB `tcc.exe`) are much faster than the previous per-cluster wait path:

| Pass | ~210 KB `tcc.exe` map |
|------|------------------------|
| Cold | ~98 ms |
| Warm (block cache) | &lt;1 ms |

Techniques used:

- **FAT extent coalescing** — contiguous cluster runs become multi-sector IDE/USB requests (up to 64 sectors).
- **Generic block cache** (`kernel/iomgr/blkcache.c`, 512 KiB) shared via the I/O manager.
- **BPB caching** — avoid re-reading the boot sector on every open.
- **Eager file map** — `vfs_mapfile()` / `user_execp` `[mmap]` path for loading executables; POSIX `mmap` with an fd fills through the same reader.
- **USB MSC** — multi-block SCSI READ10/WRITE10 instead of one sector per command.
- **FAT32 create/alloc fixes** — correct free-cluster scan and directory slot lookup so in-OS compilers can write outputs.

### Other kernel / userland notes

- ELF user processes with larger heap/stack; kernel identity map retained in user page directories for syscalls.
- Headless-friendly exception dumps (serial) without keyboard pause.
- LFN support enabled for long source filenames on FAT.

See also `ics-os/contrib/tcc/README.icsos` and `ics-os/base/icsos.hlp`.

## Development and Support
This project is used at the <a href='http://www.ics.uplb.edu.ph'>Institute of Computer Science</a>, <a href='http://www.uplb.edu.ph'>University of the Philippines Los Banos</a> for <a href='http://ics.uplb.edu.ph/courses/ugrad/cmsc/125'>CMSC 125</a>. It is maintained by the <a href='https://srg-ics-uplb.github.io'>Systems Research Group</a>.

Get started by reading the <a href="https://github.com/srg-ics-uplb/ics-os/wiki/Kernel-Developer's-Guide">Kernel Developer's Guide</a>.

Don't forget to check the <a href="http://github.com/srg-ics-uplb/ics-os/wiki">Wiki</a>.

You can ask questions by submitting an issue.

## Citation

If you find his resource useful in your research or teaching, please cite our [paper](https://jachermocilla.org/publications/hermocilla-pitj2009-ics-os.pdf).

---

J. A. C. Hermocilla. Ics-os: A kernel programming approach to teaching operating system concepts. Philippine Information Technology Journal, 2(2):25--30, 2009.

---

You can also use the following bibtex entry.

```
@article{hermocilla-ics-os-pitj2009,
  author = {Hermocilla, J. A. C.},
  title = {ICS-OS: A Kernel Programming Approach to Teaching Operating System Concepts},
  journal = {Philippine Information Technology Journal},
  volume = {2},
  number = {2},
  year = {2009},
  issn = {2012-0761},
  pages = {25--30},
  publisher = {Philippine Society of Information Technology Educators and Computing Society of the Philippines },
  address = {Philippines},
  pdf = {https://jachermocilla.org/publications/hermocilla-pitj2009-ics-os.pdf}
}
```
