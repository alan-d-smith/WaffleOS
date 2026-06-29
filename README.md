# WaffleOS

WaffleOS is a small, Unix-like hobby operating system for 32-bit x86, written in NASM and C.
It is a learning project: the aim is to demystify how operating systems work by building one from
the bootloader up, not to be a production system.

<img src="https://github.com/user-attachments/assets/0553e678-e8b8-4853-ad1e-6e97e162c5fd" alt="WaffleOS logo" height="200"/> <img src="https://github.com/user-attachments/assets/be56b9a4-c1fe-4710-b60d-4dc30a516874" alt="WaffleOS CLI" height="200"/> <img src="https://github.com/user-attachments/assets/cf378052-a6c2-4748-a15d-c7168cb37a6b" alt="WaffleOS GUI" height="200"/>

![Hack Club arcade 2024 finalist](https://img.shields.io/badge/Hack%20Club%20arcade%202024-showcase%20finalist-gold?logo=https%3A%2F%2Fassets.hackclub.com%2Ficon-rounded.png&logoSize=auto&labelColor=orange)
![License: GPL](https://img.shields.io/badge/License-GPL-blue.svg)
[![CodeFactor](https://www.codefactor.io/repository/github/alan-d-smith/waffleos/badge)](https://www.codefactor.io/repository/github/alan-d-smith/waffleos)
![GitHub Issues or Pull Requests](https://img.shields.io/github/issues/alan-d-smith/WaffleOS)
![GitHub commit activity](https://img.shields.io/github/commit-activity/t/alan-d-smith/WaffleOS)
![GitHub contributors](https://img.shields.io/github/contributors/alan-d-smith/WaffleOS)
![GitHub Repo stars](https://img.shields.io/github/stars/alan-d-smith/WaffleOS)
![GitHub forks](https://img.shields.io/github/forks/alan-d-smith/WaffleOS)

> **Status:** early and experimental. Expect rough edges, and interfaces that change between commits.

---

## Table of contents

1. [Features](#features)
2. [Building and running](#building-and-running)
3. [Commands](#commands)
4. [Roadmap](#roadmap)
5. [Acknowledgements](#acknowledgements)
6. [License](#license)

---

## Features

Implemented:

- **Bootloader** — a two-stage boot that switches the CPU into 32-bit protected mode and loads the C kernel.
- **Interrupts** — Global Descriptor Table (GDT) and Interrupt Descriptor Table (IDT) with interrupt service routines.
- **Memory management** — a heap allocator providing `malloc()`, `calloc()`, and `free()`, plus paging.
- **Drivers** — ATA PIO disk, PS/2 keyboard and mouse, VGA text mode, and a VBE graphics mode.
- **Programmable Interval Timer (PIT)** — for timing and precise delays.
- **Shell** — a command-line interface, with an optional graphical mode (window manager and a few demo apps).

Work in progress:

- **FAT12** filesystem support.
- **HTTP client** — currently RFC 3986-compliant URI parsing.

---

## Building and running

WaffleOS is built and run on Linux. On Windows, the Windows Subsystem for Linux (WSL) works well —
run `wsl --install`, follow the prompts, then restart.

### Run a prebuilt image (QEMU)

To just try it out, download a `main_floppy.img` from the releases page (or use the one in `build/`) and run:

```sh
sudo apt install qemu-system-i386
qemu-system-i386 -drive format=raw,file=path/to/main_floppy.img
```

### Build from source

musl is included as a git submodule, so clone recursively (or initialise the submodule after cloning):

```sh
git clone --recursive https://github.com/alan-d-smith/WaffleOS.git
# or, if already cloned:
git submodule update --init
```

Build dependencies:

- `nasm`
- an `i686-elf` cross-compiler (`i686-elf-gcc` and binutils) — see the
  [OSDev cross-compiler guide](https://wiki.osdev.org/GCC_Cross-Compiler).
  The `install_linux_mint.sh` script in this repository automates building the toolchain.
- `make`
- `mtools` and `dosfstools` — provide `mkfs.fat`, `mcopy`, and `mmd`, used to assemble the FAT12 floppy image.
- `qemu-system-i386` to run the result, or `bochs`.

musl's libm is built automatically from the `external/musl` submodule as part of the build; there is
no separate install step for it.

Then:

```sh
make          # build build/main_floppy.img
make qemu     # run the image in QEMU
make bochs    # run the image in Bochs
make clean    # remove build artifacts
```

### Bochs

<details>
<summary>Recommended <code>./configure</code> flags for building Bochs on Linux Mint</summary>

See also `install_bochs_linux_mint.sh`.

```sh
./configure --enable-smp \
            --enable-cpu-level=6 \
            --enable-all-optimizations \
            --enable-x86-64 \
            --enable-pci \
            --enable-vbe \
            --enable-vmx \
            --enable-debugger \
            --enable-disasm \
            --enable-debugger-gui \
            --enable-logging \
            --enable-fpu \
            --enable-3dnow \
            --enable-sb16=dummy \
            --enable-cdrom \
            --enable-x86-debugger \
            --enable-iodebug \
            --disable-plugins \
            --disable-docbook \
            --with-term --with-sdl2
```

</details>

### Real x86 hardware

Booting on physical hardware is not documented yet.

---

## Commands

The command set is still evolving and subject to change. Run `help` in the shell for the current list.

| Command | Description |
| --- | --- |
| `help` | List available commands |
| `clear` | Clear the screen |
| `systime` | Print the time since startup |
| `enablegraphics` | Switch to graphics mode and launch the GUI |
| `printroot` | Print the root directory entries |
| `shutdown` | Shut down the system (planned; will use ACPI) |
| `crashme` | Deliberately fault the system (for testing) |

Filesystem commands exist as stubs and do not yet do anything meaningful:
`ls`, `cd`, `mkdir`, `touch`.

### Fun

| Command | Description |
| --- | --- |
| `hello` | Print "Hello, World!" |
| `waffle` | Print the WaffleOS splash, with an ASCII waffle |
| `cowsay <text>` | Print a cow saying `<text>` (defaults to "WaffleOS!") |

---

## Roadmap

Planned, in rough order of interest:

- Flesh out the filesystem commands (`ls`, `cd`, `mkdir`, `mv`, `touch`, `rm`, `cat`) on top of FAT12.
- A graphics toggle command to enable/disable VBE graphics at runtime.
- Implement `shutdown` via ACPI.
- Continue the HTTP client beyond URI parsing.

---

## Acknowledgements

WaffleOS would not have been possible without these resources:

- **Nanobyte OS** by Tiberiu C. — <https://github.com/nanobyte-dev/nanobyte_os>
- **OSDev Wiki** — <https://wiki.osdev.org/Main_Page>
- **musl libc** (used for libm) — <https://musl.libc.org/>

```bibtex
@misc{nanobyte_os,
  author       = {Tiberiu C.},
  title        = {Nanobyte OS},
  year         = {2024},
  url          = {https://github.com/nanobyte-dev/nanobyte_os},
  note         = {Accessed: 2025-03-01}
}
@misc{osdev,
  author       = {{OSDev Community}},
  title        = {OSDev Wiki},
  year         = {2025},
  url          = {https://wiki.osdev.org/Main_Page},
  note         = {Accessed: 2025-03-01}
}
@misc{musl,
  author       = {Rich Felker and contributors},
  title        = {musl libc},
  year         = {2025},
  url          = {https://musl.libc.org/},
  note         = {Version 1.2.6, MIT licensed (Copyright © 2005-2020 Rich Felker, et al.). Used for libm. Accessed: 2026-06-29}
}
```

---

## License

WaffleOS is licensed under the [GNU General Public License](LICENSE).

It also links [musl libc](https://musl.libc.org/)'s math library, included as a git submodule under
`external/musl`, which is distributed under the MIT license (Copyright © 2005-2020 Rich Felker, et al.).
The MIT license text and full contributor list are in the submodule's `COPYRIGHT` file.
Much of the math code used (`src/math`) derives from third-party works under permissive terms,
including Copyright © 1993, 2004 Sun Microsystems; © 2003-2011 David Schultz; © 2003-2009 Steven G. Kargl;
© 2003-2009 Bruce D. Evans; © 2008 Stephen L. Moshier; and © 2017-2018 Arm Limited, as labelled in the
individual source files.
