#include "cpu_sched.h"

#include <stddef.h>
#include "cpu_sched.h"
#include "../core/stdio.h"
#include "../core/memory.h"

static PCB* ready_queue_head = NULL;
static PCB* ready_queue_tail = NULL;
static uint32_t next_pid = 1;

void scheduler_init(void) {
    ready_queue_head = NULL;
    ready_queue_tail = NULL;
    next_pid = 1;
    printf("[SCHEDULER] Scheduler initialized.\r\n");
}

void add_process(void (*entry_point)(void)) {
    PCB* new_pcb = (PCB*)malloc(sizeof(PCB));
    if (!new_pcb) {
        printf("[SCHEDULER] Failed to allocate PCB.\r\n");
        return;
    }
    new_pcb->pid = next_pid++;
    new_pcb->state = PROCESS_READY;
    new_pcb->entry_point = entry_point;
    new_pcb->next = NULL;

    if (ready_queue_tail == NULL) {
        ready_queue_head = new_pcb;
        ready_queue_tail = new_pcb;
    } else {
        ready_queue_tail->next = new_pcb;
        ready_queue_tail = new_pcb;
    }

    printf("[SCHEDULER] Process %d added.\r\n", new_pcb->pid);
}

void schedule(void) {
    while (ready_queue_head != NULL) {
        PCB* current = ready_queue_head;
        current->state = PROCESS_RUNNING;
        printf("[SCHEDULER] Running process %d...\r\n", current->pid);

        current->entry_point();

        current->state = PROCESS_TERMINATED;
        ready_queue_head = current->next;
        free(current);
    }
    ready_queue_tail = NULL;
}
