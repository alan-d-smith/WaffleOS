#include "basic.h"

#include "../cmd_executor.h"
#include "../stdio.h"
#include "../../libs/string.h"
#include "../../drivers/display.h"
#include "../../timers/timer.h"
#include "../x86.h"
#include "../idt.h"
#include "../log.h"
#include "../../libs/gui.h"
#include "../../libs/apps/terminal.h"

void cowsay(char* message) {    // TODO: change once string concatenation is improved (requires memory allocation)
    size_t len = strlen(message);
    size_t width = len + 2; // add padding

    console_printf(" ");
    for (size_t i = 0; i < width; i++) {
        console_printf("_");
    }
    console_printf("\n");

    console_printf("< %s >\n", message);

    console_printf(" ");
    for (size_t i = 0; i < width; i++) {
        console_printf("-");
    }
    console_printf("\n");

    console_printf("        \\   ^__^\r\n         \\  (oo)\\_______\r\n            (__)\\       )\\/\\\r\n                ||----w |\r\n                ||     ||\r\n\r\n", message);;
}

void print_splash(void) {
    console_printf("\n\t  \xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB\r\n");  // ╔══════════╗
    console_printf("\t  \xBA WaffleOS \xBA\r\n");                                  // ║ WaffleOS ║
    console_printf("\t  \xC8\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC\n\r\n");  // ╚══════════╝
}

void cmd_crashme(int argc, char **argv) {
    crash_me();
}

void cmd_clear(int argc, char **argv) {
    if (g_text_mode) {
        clrscr();
    } else {
        clear_terminal_buffer();
    }
}

void cmd_help(int argc, char **argv) {
    print_command_list();
}

void cmd_colors(int argc, char **argv) {
    static const char *names[16] = {
        "black", "blue", "green", "cyan", "red", "magenta", "brown", "light gray",
        "dark gray", "light blue", "light green", "light cyan", "light red",
        "light magenta", "yellow", "white"
    };
    console_printf("Colour palette:\r\n");
    for (int i = 0; i < 16; i++) {
        console_reset_color();
        console_printf("  %d ", i);
        console_set_color((uint8_t)i);
        console_printf("%s\r\n", names[i]);
    }
    console_reset_color();
}

// Block until a key is pressed
static void palette_wait_key(void) {
    disable_irq(1);
    while (x86_inb(0x64) & 0x01) (void)x86_inb(0x60);  // drain pending bytes
    for (;;) {
        if (x86_inb(0x64) & 0x01) {
            uint8_t sc = x86_inb(0x60);
            if (!(sc & 0x80)) break;
        }
    }
    while (x86_inb(0x64) & 0x01) (void)x86_inb(0x60);  // drain the break code
    enable_irq(1);
}

// Show the full 256-colour DAC palette.
// Layout:
//      0-15    standard colours
//      16-231  6x6x6 RGB cube
//      232-255 grayscale ramp
void cmd_palette(int argc, char **argv) {
    (void) argc;
    (void) argv;

    enable_graphics_mode(MODE_320x200x1);
    set_drawing_target(NULL);
    g_clrscr(0);

    enum
    {
        X = 16,
        W = 288,
        GAP = 6,

        STD_Y = 8,
        STD_COUNT = 16,
        STD_SW = W / STD_COUNT,
        STD_SH = 16,

        CUBE_Y = STD_Y + STD_SH + GAP,
        CUBE_LEVELS = 6,
        CUBE_COLS = CUBE_LEVELS * CUBE_LEVELS,
        CUBE_SW = W / CUBE_COLS,
        CUBE_SH = 18,

        GRAY_Y = CUBE_Y + CUBE_LEVELS * CUBE_SH + GAP,
        GRAY_COUNT = 24,
        GRAY_SW = W / GRAY_COUNT,
        GRAY_SH = 22,
    };

    // Standard colours: 0-15
    for (int i = 0; i < STD_COUNT; i++) {
        draw_rect(X + i * STD_SW,
                  STD_Y,
                  STD_SW,
                  STD_SH,
                  (uint32_t)i);
    }

    // 6x6x6 RGB cube: 16-231
    for (int g = 0; g < CUBE_LEVELS; g++) {
        int y = CUBE_Y + g * CUBE_SH;

        for (int r = 0; r < CUBE_LEVELS; r++) {
            int col_base = r * CUBE_LEVELS;
            int colour_base = 16 + 36 * r + 6 * g;

            for (int b = 0; b < CUBE_LEVELS; b++) {
                int x = X + (col_base + b) * CUBE_SW;
                uint32_t colour = colour_base + b;

                draw_rect(x, y, CUBE_SW, CUBE_SH, colour);
            }
        }
    }

    // Grayscale: 232-255
    for (int i = 0; i < GRAY_COUNT; i++) {
        draw_rect(
            X + i * GRAY_SW,
            GRAY_Y,
            GRAY_SW,
            GRAY_SH,
            (uint32_t)(232 + i)
        );
    }

    palette_wait_key();
    enable_text_mode();
}

void cmd_shutdown(int argc, char **argv) {
    console_printf("Shutting down...\r\n");
}

void cmd_systime(int argc, char **argv) {
    console_printf("Time since startup: ");
    print_systime();
    console_printf("\r\n");
}

void cmd_hello(int argc, char **argv) {
    console_printf("Hello, World!\r\n");
}

void cmd_waffle(int argc, char **argv) {
    print_splash();
    console_printf(" _________________\r\n");
    console_printf("/ ._____________. \\\r\n");
    console_printf("| |_|_|_|_|_|_|_| |\r\n");
    console_printf("| |_|_|_|_|_|_|_| |\r\n");
    console_printf("| |_|_|_|_|_|_|_| |\r\n");
    console_printf("| |_|_|_|_|_|_|_| |\r\n");
    console_printf("| |_|_|_|_|_|_|_| |\r\n");
    console_printf("| |_|_|_|_|_|_|_| |\r\n");
    console_printf("\\_________________/\r\n\r\n");
}

void cmd_cowsay(int argc, char **argv) {
    char *message = "WaffleOS!";
    if (argc > 1) {
        message = argv[1];
    }
    cowsay(message);
}

// "enablegraphics" command: Enables graphics mode, clears the screen,
// initializes the GUI, and enters an infinite loop calling render_gui()
void cmd_enablegraphics(int argc, char **argv) {
    if (!g_text_mode) {
        log_error("VGA", "Already in graphics mode.\r\n");
        return;
    }

    char mode = '0';  // default mode '0'
    if (argc > 1) {
        mode = argv[1][0];
    }
    // Convert mode character to int
    enable_graphics_mode(mode - '0');
    g_clear_screen();
    init_gui();

    while(true) {
        render_gui();
    }
}

// Register the basic commands with the command executor
void register_basics(void) {
    register_command("crashme",  cmd_crashme,  "Crash the system");
    register_command("clear",    cmd_clear,    "Clear the screen");
    register_command("help",     cmd_help,     "Display help message");
    register_command("colors",   cmd_colors,   "Show the 16-colour palette");
    register_command("palette",  cmd_palette,  "Show the 256-colour palette");
    register_command("shutdown", cmd_shutdown, "Shutdown the system");
    register_command("systime",  cmd_systime,  "Print the time since startup");
    register_command("hello",    cmd_hello,    "Print Hello, World!");
    register_command("waffle",   cmd_waffle,   "Display the WaffleOS splash screen");
    register_command("cowsay",   cmd_cowsay,   "Make a cow say something");
    register_command("enablegraphics", cmd_enablegraphics, "Enable graphics mode and launch GUI");
}
