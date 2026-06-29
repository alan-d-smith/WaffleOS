#!/bin/sh
#
# Builds Bochs 2.7 from source and installs it under /usr/local.
#
# NOTE: This is an ALTERNATIVE to the distribution package. On Debian/Ubuntu/WSL
# the simplest route is:  sudo apt install bochs bochs-sdl vgabios
# The committed .bochsrc targets those distro paths (/usr/share/...). If you use
# this script instead, point .bochsrc at the source-build paths, e.g.:
#   romimage:    file=/usr/local/share/bochs/BIOS-bochs-latest
#   vgaromimage: file=/usr/local/share/bochs/VGABIOS-lgpl-latest

mkdir -p "toolchain"
cd toolchain || exit

wget -O bochs-2.7.tar.gz "https://sourceforge.net/projects/bochs/files/bochs/2.7/bochs-2.7.tar.gz/download"
tar -xzf bochs-2.7.tar.gz

cd bochs-2.7 || exit

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

make
sudo make install
