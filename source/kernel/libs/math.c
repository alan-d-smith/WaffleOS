// Using information from: https://wiki.osdev.org/FPU
#include "math.h"
#include <stdio.h>
#include "log.h"

void dummy_fpu(float value) {
    volatile float dummy = 1.01f - value;
    (void)dummy;
}

void init_fpu() {
    uint16_t control_word = get_fpu_control_word();
    control_word &= ~(0x3 << 8); // Clear PC bits (bits 8 and 9)
    control_word |= (0x2 << 8); // double precision

    control_word &= ~(0x3 << 10); // Clear RC bits (bits 10 and 11)
    control_word |= (0x3 << 10);  // Set RC to 11 (round toward zero)

    control_word |= 0x3F;        // Mask all exceptions
    set_fpu_control_word(control_word);

    // Dummy calculation (for some reason floating point operations just wont work if I dont do this))
    dummy_fpu(0.01f);

    log_ok("FPU", "FPU enabled");
}

// Clamp a value to a range (inclusive). i.e. for any value ∈ ℤ, clamp(value, min, max) ∈ [min, max]
int clamp(int value, int min, int max) {
    return (value < min) ? min : ((value > max) ? max : value);
}

static inline void set_fpu_control_word(uint16_t control_word) {
    __asm__ __volatile__("fldcw %0;"::"m"(control_word));
}

static inline uint16_t get_fpu_control_word() {
    uint16_t control_word;
    __asm__ __volatile__("fnstcw %0" : "=m" (control_word));
    return control_word;
}