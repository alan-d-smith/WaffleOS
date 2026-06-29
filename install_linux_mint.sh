#!/bin/sh
# Shell commands from: https://wiki.osdev.org/GCC_Cross-Compiler

# Install dependencies --------------------------------
sudo apt update
sudo apt install -y build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo libisl-dev libsdl2-dev nasm

# Create the toolchain directory in $HOME and move into it
mkdir -p "$HOME/toolchain"
cd "$HOME/toolchain" || exit

BINUTILS_VER=2.37
GCC_VER=11.2.0

# Set variables
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# Download and extract files
BINUTILS_TAR="binutils-${BINUTILS_VER}.tar.xz"
GCC_TAR="gcc-${GCC_VER}.tar.xz"

wget "https://ftp.gnu.org/gnu/binutils/${BINUTILS_TAR}"
wget "https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VER}/${GCC_TAR}"

tar -xf ${BINUTILS_TAR}
tar -xf ${GCC_TAR}

# BINUTILS ----------------------------------------
mkdir -p "$HOME/src"
cd "$HOME/src" || exit

mkdir -p build-binutils
cd build-binutils || exit
"$HOME/toolchain/binutils-${BINUTILS_VER}/configure" --target="$TARGET" --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
make install

# GCC ---------------------------------------------
cd "$HOME/src" || exit

# The $PREFIX/bin dir _must_ be in the PATH. We did that above.
which -- "$TARGET-as" || echo "$TARGET-as is not in the PATH"

mkdir -p build-gcc
cd build-gcc || exit

"$HOME/toolchain/gcc-${GCC_VER}/configure" --target="$TARGET" --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers --disable-hosted-libstdcxx
make all-gcc
make all-target-libgcc
# make all-target-libstdc++-v3
make install-gcc
make install-target-libgcc
# make install-target-libstdc++-v3

# Persist the toolchain on PATH for future shells so `make` works without setting PATH each time.
PROFILE="$HOME/.bashrc"
PATH_LINE="export PATH=\"\$HOME/opt/cross/bin:\$PATH\""
if ! grep -qsF "$PATH_LINE" "$PROFILE"; then
    printf '\n# i686-elf cross-compiler (WaffleOS)\n%s\n' "$PATH_LINE" >> "$PROFILE"
    echo "Added \$HOME/opt/cross/bin to PATH in $PROFILE"
fi

echo
echo "i686-elf cross-compiler installed to $PREFIX."
echo "Run 'source $PROFILE' or open a new terminal so i686-elf-gcc is on your PATH"
