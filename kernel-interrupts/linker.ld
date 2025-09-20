ENTRY(_start)
SECTIONS
{
    . = 0x40000000;
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : {
        __bss_start = .;
        *(.bss*) *(COMMON) __bss_end = .;
    }
    _stack_top = 0x40010000;
}