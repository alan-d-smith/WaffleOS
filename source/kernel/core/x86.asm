; Kernel x86 helper functions
; x86_inb and x86_outb source: https://github.com/nanobyte-dev/nanobyte_os/blob/videos/part7/src/kernel/x86.asm

; Read a byte from a port
global x86_inb
x86_inb:
    [bits 32]
    mov dx, [esp + 4]   ; Load port number from stack
    xor eax, eax
    in al, dx           ; Input byte from port into AL
    ret

; Write a byte to a port
global x86_outb
x86_outb:
    [bits 32]
    mov dx, [esp + 4]   ; Load port number from stack
    mov al, [esp + 8]   ; Load value to output from stack
    out dx, al          ; Output the value to the port
    ret

; Read a word from a port
global x86_inw
x86_inw:
    [bits 32]
    xor eax, eax         ; Clear entire EAX register
    mov dx, [esp + 4]    ; Load port number from stack
    in ax, dx           ; Input word from port into AX (upper 16 bits remain 0)
    ret

; Write a word to a port
global x86_outw
x86_outw:
    [bits 32]
    mov dx, [esp + 4]   ; Load port number from stack
    mov ax, [esp + 8]   ; Load word value to output from stack
    out dx, ax          ; Output the word value to the port
    ret

; i686_Panic and crash_me are from: https://github.com/nanobyte-dev/nanobyte_os/blob/videos/part9/src/kernel/arch/i686/io.asm
global i686_Panic
i686_Panic:
    cli
    hlt

global crash_me
crash_me:
    ; div by 0
    ; mov ecx, 0x1337
    ; mov eax, 0
    ; div eax
    int 0x80
    ret

; Source of information used for paging: https://wiki.osdev.org/Paging#32-bit_Paging_(Protected_Mode)

; Load the current value of CR3 into EAX and return it.
global x86_load_cr3
x86_load_cr3:
    [bits 32]
    mov eax, cr3       ; Move CR3 value into EAX
    ret

; Store a new value into CR3.
; The new CR3 value is passed on the stack (first parameter).
global x86_store_cr3
x86_store_cr3:
    [bits 32]
    mov eax, [esp + 4] ; Get the new CR3 value from the stack
    mov cr3, eax       ; Load it into CR3
    ret

; Load the current value of CR0 into EAX and return it.
global x86_load_cr0
x86_load_cr0:
    [bits 32]
    mov eax, cr0
    ret

; Store a new value into CR0.
; The new CR0 value is passed on the stack (first parameter).
global x86_store_cr0
x86_store_cr0:
    [bits 32]
    mov eax, [esp + 4]
    mov cr0, eax
    ret
