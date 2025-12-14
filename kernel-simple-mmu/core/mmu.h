#ifndef __MMU__
#define __MMU__

#include <stdbool.h>
#include <stdint.h>
extern void uart_printf(const char* fmt, ...);


extern uint8_t __block_memory_start[];
extern uint8_t __block_memory_end[];

typedef struct allocate_mem {
	uint8_t *buffer;
} AllocateMem;

typedef struct bit_index
{
	uint64_t index;
	uint8_t bit;
	bool valid;
} BitIndex; 

#endif // __MMU__