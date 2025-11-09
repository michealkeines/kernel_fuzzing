ENTRY(_start)
SECTIONS
{
    . = 0x0000000080000000;
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : {
        __bss_start = .;
        *(.bss*) *(COMMON) __bss_end = .;
    }
    _stack_top = 0x00000000800A0000;
}