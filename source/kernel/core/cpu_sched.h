#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Process states
typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_TERMINATED
} process_state_t;

// Process Control Block
typedef struct PCB {
    uint32_t pid;                   // Unique PID.
    process_state_t state;          // Current state of the process.
    void (*entry_point)(void);      // Function pointer to the process code.
    struct PCB* next;               // Pointer to the next PCB in the ready queue.
    // TODO: add registers, etc.
} PCB;

// Scheduler interface.
void scheduler_init(void);
void add_process(void (*entry_point)(void));
void schedule(void);
