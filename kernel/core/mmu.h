#ifndef __MMU__
#define __MMU__

#include <stdbool.h>
#include <stdint.h>
extern void uart_printf(const char* fmt, ...);
#define USER_PROCESS_1_TABLE_START 0x800F0000UL
#define USER_PROCESS_2_TABLE_START (USER_PROCESS_1_TABLE_START + 0x00060000UL)



extern uint8_t __block_memory_start[];
extern uint8_t __block_memory_end[];

extern uint8_t __block_l2_memory_start[];

typedef struct allocate_mem {
	uint8_t *buffer;
} AllocateMem;

typedef struct bit_index
{
	uint64_t index;
	uint8_t bit;
	bool valid;
} BitIndex; 

uint64_t kmalloc(uint64_t size);
void add_virtio_page(uint64_t reg);

void init_bitmap(void *bitmap);

uint64_t convert_to_physical(uint64_t add);

uint64_t kernal_to_user_space(uint64_t virtual_kernel_address);
#endif // __MMU__