#include "basic.h"

#include "../cmd_executor.h"
#include "../stdio.h"
#include "../../libs/string.h"
#include "../../drivers/display.h"
#include "../../timers/timer.h"
#include "../x86.h"
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
    register_command("shutdown", cmd_shutdown, "Shutdown the system");
    register_command("systime",  cmd_systime,  "Print the time since startup");
    register_command("hello",    cmd_hello,    "Print Hello, World!");
    register_command("waffle",   cmd_waffle,   "Display the WaffleOS splash screen");
    register_command("cowsay",   cmd_cowsay,   "Make a cow say something");
    register_command("enablegraphics", cmd_enablegraphics, "Enable graphics mode and launch GUI");
}
