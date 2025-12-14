ENTRY(_start)
SECTIONS
{
    # think about memory region overflow, as we might overflow in future, if we grow past the current block, this happened in the text region, as we added more funcitons to it
    . = 0x0000000080000000;
    .text : { *(.text*); }
     . = ALIGN(4096);
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : {
        __bss_start = .;
        *(.bss*) *(COMMON) __bss_end = .;
        . += 0x7000;
      
    }  
    . = ALIGN(4096);
    __block_memory_start = .;
    . += 512 * 1024;
    __block_memory_end = .;
    _stack_top = 0x00000000800A0000;
}
