# KERNEL MAKEFILE
# source: https://github.com/nanobyte-dev/nanobyte_os/blob/videos/part7/src/kernel/Makefile
TARGET_ASMFLAGS += -f elf
TARGET_CFLAGS += -ffreestanding -nostdlib -mno-red-zone -I../kernel -Icore -I$(BUILD_DIR)/musl-sysroot/include -Wall -Wextra
TARGET_LIBS += $(BUILD_DIR)/libm.a -lgcc
TARGET_LINKFLAGS += -T linker.ld -nostdlib

SOURCES_C   := $(patsubst ./%,%,$(shell find . -type f -name '*.c'))
SOURCES_ASM := $(patsubst ./%,%,$(shell find . -type f -name '*.asm'))
HEADERS_ASM := $(patsubst ./%,%,$(shell find . -type f -name '*.inc'))
SOURCES_GAS := $(patsubst ./%,%,$(shell find . -type f -name '*.s'))

OBJECTS_C   := $(patsubst %.c,$(BUILD_DIR)/kernel/c/%.obj,$(SOURCES_C))
OBJECTS_ASM := $(patsubst %.asm,$(BUILD_DIR)/kernel/asm/%.obj,$(SOURCES_ASM))
OBJECTS_GAS := $(patsubst %.s,$(BUILD_DIR)/kernel/asm/%.obj,$(SOURCES_GAS))

.PHONY: all kernel clean always

all: kernel

kernel: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: $(OBJECTS_ASM) $(OBJECTS_S) $(OBJECTS_C)
	@$(TARGET_LD) $(TARGET_LINKFLAGS) -Wl,-Map=$(BUILD_DIR)/kernel.map -o $@ $^ $(TARGET_LIBS)
	@echo "--> Created:  kernel.bin"

$(BUILD_DIR)/kernel/c/%.obj: %.c
	@mkdir -p $(@D)
	@$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/asm/%.obj: %.asm $(HEADERS_ASM)
	@mkdir -p $(@D)
	@$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/asm/%.obj: %.s $(HEADERS_ASM)
	@mkdir -p $(@D)
	@$(TARGET_CC) $(TARGET_ASMFLAGS) -m32 -c -o $@ $<
	@echo "--> Compiled (GAS): " $<

clean:
	rm -rf $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/kernel/
