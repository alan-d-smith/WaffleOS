# libm build for WaffleOS
#
# Builds musl's libm (real-valued <math.h> functions + fenv) as a freestanding
# static library for the i686-elf kernel, and generates a curated include
# "sysroot" that exposes ONLY <math.h> (plus the headers it pulls in) so musl's
# other libc headers cannot shadow the kernel's freestanding/own headers.
#
# musl libc 1.2.6 - https://musl.libc.org  (vendored under ../musl)
include ../../config.mk

ARCH := i386
MUSL := ../musl

# Generated headers / outputs live under $(BUILD_DIR); the vendored musl tree
# stays a pristine upstream extract.
ALLTYPES    := $(BUILD_DIR)/musl-gen/include/bits/alltypes.h
OUT_LIB     := $(BUILD_DIR)/libm.a
SYSROOT_INC := $(BUILD_DIR)/musl-sysroot/include

MUSL_CFLAGS := -std=c99 -D_XOPEN_SOURCE=700 -ffreestanding -nostdlib -O2 \
               -fno-builtin -fno-stack-protector \
               -fno-tree-loop-distribute-patterns -fexcess-precision=standard
MUSL_INC := -I$(MUSL)/arch/$(ARCH) -I$(MUSL)/arch/generic \
            -I$(MUSL)/src/include -I$(MUSL)/src/internal \
            -I$(BUILD_DIR)/musl-gen/include -I$(MUSL)/include

# Source selection: arch-specific implementations override the generic C ones
# by basename, exactly as musl's own Makefile does.
math_base := $(wildcard $(MUSL)/src/math/*.c)
fenv_base := $(wildcard $(MUSL)/src/fenv/*.c)
arch_srcs := $(wildcard $(MUSL)/src/math/$(ARCH)/*.c) $(wildcard $(MUSL)/src/math/$(ARCH)/*.s) \
             $(wildcard $(MUSL)/src/fenv/$(ARCH)/*.c) $(wildcard $(MUSL)/src/fenv/$(ARCH)/*.s)

arch_bn   := $(basename $(notdir $(arch_srcs)))
base_srcs := $(foreach f,$(math_base) $(fenv_base),\
               $(if $(filter $(basename $(notdir $(f))),$(arch_bn)),,$(f)))

all_srcs := $(base_srcs) $(arch_srcs)
# Object paths mirror the source tree under $(BUILD_DIR)/libm/.
objs := $(foreach s,$(all_srcs),$(BUILD_DIR)/libm/$(patsubst $(MUSL)/%,%,$(basename $(s))).o)

.PHONY: all sysroot clean
all: $(OUT_LIB) sysroot

$(ALLTYPES):
	@mkdir -p $(@D)
	@sed -f $(MUSL)/tools/mkalltypes.sed \
	    $(MUSL)/arch/$(ARCH)/bits/alltypes.h.in \
	    $(MUSL)/include/alltypes.h.in > $@

$(BUILD_DIR)/libm/%.o: $(MUSL)/%.c $(ALLTYPES)
	@mkdir -p $(@D)
	@$(TARGET_CC) $(MUSL_CFLAGS) $(MUSL_INC) -c -o $@ $<

$(BUILD_DIR)/libm/%.o: $(MUSL)/%.s $(ALLTYPES)
	@mkdir -p $(@D)
	@$(TARGET_CC) $(MUSL_INC) -c -o $@ $<

$(OUT_LIB): $(objs)
	@rm -f $@
	@$(TARGET)-ar rcs $@ $(objs)
	@echo "--> Created libm: $@ ($(words $(objs)) objects)"

sysroot: $(ALLTYPES)
	@mkdir -p $(SYSROOT_INC)/bits
	@cp $(MUSL)/include/math.h     $(SYSROOT_INC)/math.h
	@cp $(MUSL)/include/features.h $(SYSROOT_INC)/features.h
	@cp $(ALLTYPES)                $(SYSROOT_INC)/bits/alltypes.h
	@echo "--> Created libm sysroot: $(SYSROOT_INC)"

clean:
	@rm -rf $(BUILD_DIR)/libm $(OUT_LIB) $(BUILD_DIR)/musl-gen $(BUILD_DIR)/musl-sysroot
