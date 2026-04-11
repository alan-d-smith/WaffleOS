#include "file_cmds.h"
#include "../cmd_executor.h"
#include "../stdio.h"
#include "../../libs/string.h"
#include "../../drivers/display.h"
#include "../../timers/timer.h"
#include "../x86.h"
#include "../../libs/gui.h"
#include "../../libs/apps/terminal.h"
#include "../../drivers/fat.h"

// Command function: printroot
// This function iterates over the root directory entries in the FAT filesystem
// and prints the names of valid files/directories.
void cmd_printroot(int argc, char **argv) {
    // Check if the FAT filesystem is initialized (g_Data should be non-NULL)
    if (!g_Data) {
        console_printf("FAT: Filesystem not initialized\r\n");
        return;
    }

    // Get the root directory entries.
    FAT_DirectoryEntry *entries = (FAT_DirectoryEntry*)g_Data->RootDirectory.Buffer;
    int entryCount = g_Data->BS.BootSector.DirEntryCount;

    console_printf("Root Directory:\r\n");

    for (int i = 0; i < entryCount; i++) {
        // If the first byte is 0x00, then no further entries exist.
        if (entries[i].Name[0] == 0x00)
            break;
        // If the entry is deleted (first byte 0xE5), skip it.
        if (entries[i].Name[0] == 0xE5)
            continue;

        char name[12];
        memcpy(name, entries[i].Name, 11);
        name[11] = '\0';
        console_printf("%s\r\n", name);
    }
}

void cmd_mkdir(int argc, char **argv) {
    if (argc < 2) {
        console_printf("Usage: mkdir <directory>\r\n");
        return;
    }

    console_printf("mkdir not yet functional!\r\n");

    // FAT_MakeDirectory(argv[1]);
}

void cmd_touch(int argc, char **argv) {
    if (argc < 2) {
        console_printf("Usage: touch <filename>\r\n");
        return;
    }
    console_printf("touch command not yet implemented\r\n");
}

void cmd_ls(int argc, char **argv) {
    console_printf("ls command not yet implemented\r\n");
}

void cmd_cd(int argc, char **argv) {
    if (argc < 2) {
        console_printf("Usage: cd <directory>\r\n");
        return;
    }
    console_printf("cd command not yet implemented\r\n");
}

// Register file-related commands.
void register_file_commands(void) {
    register_command("printroot", cmd_printroot, "Print the root directory entries");
    register_command("mkdir",    cmd_mkdir,    "Make a directory (stub)");
    register_command("touch",    cmd_touch,    "Create a new file (stub)");
    register_command("ls",       cmd_ls,       "List directory contents (stub)");
    register_command("cd",       cmd_cd,       "Change directory (stub)");
}
