.set MAGIC, 0xE85250D6
.section .multiboot, "a"
.align 8
mbt_header:
    .long MAGIC
    .long 0
    .long 48
    .long -(MAGIC + 0 + 48)
    .short 5
    .short 0
    .long 20
    .long 1024
    .long 768
    .long 32
    .long 0
    .short 0
    .short 0
    .long 8

.section .bss
.align 16
stack_bottom:
    .skip 65536
stack_top:

.section .text
.global _start
.type _start, @function
_start:
    cli
    mov $stack_top, %esp
    xor %ebp, %ebp
    push %ebx
    call kmain
1:  hlt
    jmp 1b
