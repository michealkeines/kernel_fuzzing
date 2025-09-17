ENTRY(_start)

SECTIONS
{
    . = 0x00000000;

    .text : ALIGN(0x1000) {
        KEEP(*(.text.boot))
        *(.text*)
    }

    .rodata : ALIGN(0x1000) {
        *(.rodata*)
    }

    .data : ALIGN(0x1000) {
        *(.data*)
    }

    .bss : ALIGN(0x1000) {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }

    _stack_top = 0x40010000;
}