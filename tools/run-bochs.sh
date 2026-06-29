#!/bin/sh
# Launch Bochs with auto-detected BIOS, VGA BIOS and display library so that
# `make bochs` works across environments without editing .bochsrc:
#   - distribution packages on Debian/Ubuntu/WSL (SeaBIOS + vgabios package),
#   - distribution packages on Fedora/Arch (BIOS-bochs-latest in /usr/share),
#   - a Bochs built from source under /usr/local.
#
# .bochsrc holds the machine-independent settings; this script supplies the
# paths that differ per system, passing them as command-line overrides (Bochs
# lets later command-line options override the rcfile).
#
# Override the display library with:  BOCHS_DISPLAY=x make bochs
set -eu

# Always run from the repository root (this script lives in tools/).
cd "$(CDPATH= cd "$(dirname "$0")/.." && pwd)"

die() { echo "run-bochs: $*" >&2; exit 1; }

command -v bochs >/dev/null 2>&1 || \
    die "bochs not found. Debian/Ubuntu/WSL: sudo apt install bochs bochs-sdl vgabios"
[ -f build/main_floppy.img ] || die "build/main_floppy.img not found - run 'make' first."

# Prefer the share/ tree that belongs to the bochs binary in PATH.
bochs_bin=$(command -v bochs)
prefix=${bochs_bin%/bin/bochs}
[ "$prefix" = "$bochs_bin" ] && prefix=/usr   # fallback if not .../bin/bochs

# Return the first readable file from the arguments (-r follows symlinks, so a
# dangling distro symlink such as VGABIOS-lgpl-latest is correctly skipped).
first_readable() {
    for f in "$@"; do
        if [ -r "$f" ]; then printf '%s\n' "$f"; return 0; fi
    done
    return 1
}

bios=$(first_readable \
    "$prefix/share/bochs/BIOS-bochs-latest" \
    "$prefix/share/seabios/bios-256k.bin" \
    /usr/share/bochs/BIOS-bochs-latest \
    /usr/local/share/bochs/BIOS-bochs-latest \
    /usr/share/seabios/bios-256k.bin \
    /usr/share/seabios/bios.bin) \
    || die "no system BIOS found. Debian/Ubuntu/WSL: sudo apt install bochs (pulls in seabios)."

vgabios=$(first_readable \
    "$prefix/share/bochs/VGABIOS-lgpl-latest" \
    /usr/share/bochs/VGABIOS-lgpl-latest \
    /usr/local/share/bochs/VGABIOS-lgpl-latest \
    /usr/share/vgabios/vgabios.bin \
    /usr/share/seabios/vgabios-stdvga.bin) \
    || die "no VGA BIOS found. Debian/Ubuntu/WSL: sudo apt install vgabios"

# Choose a display library that is actually available.
is_wsl() { grep -qiE 'microsoft|wsl' /proc/version 2>/dev/null; }
have_gui() {
    for d in /usr/lib/*/bochs/plugins /usr/lib/bochs/plugins /usr/local/lib/bochs/plugins; do
        [ -e "$d/libbx_$1_gui.so" ] && return 0
    done
    return 1
}

if [ "${BOCHS_DISPLAY:-}" ]; then
    display=$BOCHS_DISPLAY
elif have_gui sdl2; then
    display=sdl2
elif have_gui x; then
    display=x
else
    display=sdl2   # static (--disable-plugins) source build: assume --with-sdl2
fi

# The X11 GUI crashes under WSLg (glibc buffer overflow); steer away from it.
if [ "$display" = x ] && is_wsl; then
    if have_gui sdl2; then
        echo "run-bochs: the X11 GUI crashes under WSL; using sdl2 instead" >&2
        display=sdl2
    else
        die "the Bochs X11 GUI crashes under WSL; use SDL instead: sudo apt install bochs-sdl"
    fi
fi

echo "run-bochs: bios=$bios"
echo "run-bochs: vgabios=$vgabios"
echo "run-bochs: display=$display"

exec bochs -q -f .bochsrc \
    "romimage: file=$bios" \
    "vgaromimage: file=$vgabios" \
    "display_library: $display" \
    "$@"
