#include <stdint.h>
#include "mmu.h"
extern uint64_t get_total_ram(void);
extern void uart_init(void);
extern void uart_printf(const char* fmt, ...);
extern void *mem_block(uint64_t size);
extern void *kmalloc(uint64_t size);



#include <stdint.h>

uint8_t *init_allocation_map(uint64_t total_physical_ram)
{

    // uart_printf("test: %l\n", (uint64_t)__block_memory_start);
    uart_printf("Total RAM: %l\n", total_physical_ram);
    uint64_t bitmap_size = (total_physical_ram / 4096) / 8;
    uart_printf("Total BITMAP Size: %l\n", bitmap_size);
    uint8_t *bitmap = (uint8_t*)mem_block(bitmap_size);

    if (bitmap == 0) uart_printf("Allocation failed: %d\n", bitmap_size);
    return bitmap;
}

struct Allocated
{
    uint8_t *bitmap;
    uint64_t index;
};

void kmain(uint64_t total_ram)
{
    // uint64_t *val= (uint64_t *)0x80A00000;
    uart_init();

    uart_printf("h");
    uart_printf("Kernel Starting\n");

    uart_printf("Kernel Inited\n");
    // total_ram = 4294967296;
    // uint8_t *bitmap = init_allocation_map(total_ram);

	// AllocateMem main = {0};

	// main.buffer = bitmap;

	// BitIndex res[64];

	// void *test = kmalloc(300);

	// const char *name = (char *)test;

	// name = "sdfsd";

	// uart_printf(name);

	
	// uint64_t allocated1 = allocate_mem(&main, 19900, res);
	// print_bit_index(res, allocated1);

	// free_pages(&main, res, allocated1);
	// allocated1 = allocate_mem(&main, 19900, res);
	// print_bit_index(res, allocated1);
	// return 0;
	// allocated1 = allocate_mem(&main, 200, res);
	// print_bit_index(res, allocated1);
	// allocated1 = allocate_mem(&main, 200, res);
	// print_bit_index(res, allocated1);
	// allocated1 = allocate_mem(&main, 200, res);
	// print_bit_index(res, allocated1);
	// allocated1 = allocate_mem(&main, 200, res);
	// print_bit_index(res, allocated1);
	// allocated1 = allocate_mem(&main, 200, res);
	// print_bit_index(res, allocated1);
	// allocated1 = allocate_mem(&main, 200, res);
	// print_bit_index(res, allocated1);
	// allocated1 = allocate_mem(&main, 200, res);
	// print_bit_index(res, allocated1);
	// allocated1 = allocate_mem(&main, 200, res);
	// print_bit_index(res, allocated1);
	// allocated1 = allocate_mem(&main, 200, res);
	// print_bit_index(res, allocated1);




    


    while (1) {int i = 1;}
}