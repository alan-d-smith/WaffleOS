#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <float.h>
#include "stdio.h"
#include "memory.h"
#include "cpu_sched.h"
#include "cmd_executor.h"

#include "kernel.h"
#include "../drivers/display.h"
#include "../drivers/floppy.h"
#include "../drivers/fat.h"
#include "../libs/string.h"
#include "../libs/math.h"
#include "../libs/gui.h"
#include "x86.h"

extern uint8_t __bss_start;
extern uint8_t __end;

void __attribute__((section(".entry"))) start(uint16_t boot_drive) {
    memset((void*)(&__bss_start), (int)0, (size_t)((&__end) - (&__bss_start)));

    clrscr();
    command_executor_init();
    print_splash();
    init_hal();
    init_memory();
    init_pit();
    init_disk();
    scheduler_init();
    init_floppy();
    // init_filesystem(); // Currently non-functional
    init_keyboard();
    init_mouse();
    //init_fpu(); // Throws "Unhandled interrupt 39!"

    enable_interrupts();

    printf("[KERNEL] Kernel initialisation complete\r\n");

    //execute_command("enablegraphics");
    while(true) {
        printf("root $ ");
        char* input = read_line();
        execute_command(input);
    }

    end: // should be unreachable
        for (;;);
}
