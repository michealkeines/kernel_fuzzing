#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

static uint8_t *buff[16384];

/*
 * TODO: just after set a bit to used, we also have to mape the virtual memory to the physical memory 
 *
 * VA => PA mapping	
 * */

typedef struct allocate_mem {
	uint8_t *buffer;
} AllocateMem;

typedef struct bit_index
{
	uint64_t index;
	uint8_t bit;
	bool valid;
} BitIndex; 


bool check_free(AllocateMem *mem, uint64_t index, uint8_t bit)
{
	uint8_t temp = mem->buffer[index];
	uint8_t v = 1;
	v = v << bit; // here we will set the bit to one
	if (temp & v) return false; // here we will OR it
	return true;
}

bool deallocate_mem(AllocateMem *mem, uint64_t index, uint8_t bit)
{
	if (check_free(mem, index, bit)) return true;

	uint8_t temp = mem->buffer[index];
	uint8_t v = 0;

	v = v << bit; // here we will set the bit to zero
	mem->buffer[index] = v;


	return check_free(mem, index, bit);
}

BitIndex find_free(AllocateMem *mem)
{
	uint64_t index = 0;
	uint8_t bit = 0;
//	printf("Starting the loop \n");
	for (;index <= 16385;)
	{
		//printf("sd\n");
		BitIndex i = { 
			.index = index,
			.bit = bit,
			.valid = true
		};
	//	printf("found Index: %llu, %x\n", index, bit);
		if(check_free(mem, index, bit)) return i;

		bit += 1;
		if (bit == 8) 
		{
			bit = 0;
			index += 1;
		}	
	}
//	printf("Ending the loop \n");
	
	BitIndex r = { .index = 0, .bit = 0, .valid = false };
	return r;
}


bool set_bitmap(AllocateMem *mem, uint64_t size, BitIndex *res)
{
	uint64_t t = 0;
	for (uint64_t i = 0; i < size; ++i)
	{
		//printf("set_bitmap: %llu\n", i);	
		BitIndex val = find_free(mem);
		if (val.valid) {
		//printf("set_bitmap valid: %llu\n", i);	
			uint8_t temp = mem->buffer[val.index];
			uint8_t v = 1;
			v = v << val.bit; // here we will set the bit to one
			mem->buffer[val.index] = temp | v; // here we will OR it
			t += 1;
	//	printf("Result Index: %llu, %x\n", val.index, val.bit);
			res[i] = val;
		}
	}

	return (t == size ? true : false);
}

uint64_t allocate_mem(AllocateMem *mem, uint64_t size, BitIndex *res)
{
	if (size < 4096)
	{
		if (set_bitmap(mem, 1, res)) return 1;

		return 0;
	
	}
	uint64_t val = size / 4096;
	uint64_t rem = size % 4096;
	
	if (val > 0 && rem > 0) val += 1;
	
	if (set_bitmap(mem, val, res)) return val;

	return 0;
}

void print_bit_index(BitIndex *res, uint64_t size)
{
	for (uint64_t i = 0; i < size; ++i)
	{
		printf("Index: %llu, Bit: %x\n", res[i].index, res[i].bit);
	}
}

void free_pages(AllocateMem *mem, BitIndex *res, uint64_t size)
{
	for (uint64_t i = 0; i < size; ++i)
	{
		deallocate_mem(mem, res[i].index, res[i].bit);
	}
}

int main()
{
	AllocateMem main = {0};

	main.buffer = (uint8_t *)buff;

	BitIndex res[64];

	uint64_t allocated1 = allocate_mem(&main, 19900, res);
	print_bit_index(res, allocated1);

	//free_pages(&main, res, allocated1);
	///allocated1 = allocate_mem(&main, 19900, res);
	//print_bit_index(res, allocated1);
	//return 0;
	allocated1 = allocate_mem(&main, 200, res);
	print_bit_index(res, allocated1);
	allocated1 = allocate_mem(&main, 200, res);
	print_bit_index(res, allocated1);
	allocated1 = allocate_mem(&main, 200, res);
	print_bit_index(res, allocated1);
	allocated1 = allocate_mem(&main, 200, res);
	print_bit_index(res, allocated1);
	allocated1 = allocate_mem(&main, 200, res);
	print_bit_index(res, allocated1);
	allocated1 = allocate_mem(&main, 200, res);
	print_bit_index(res, allocated1);
	allocated1 = allocate_mem(&main, 200, res);
	print_bit_index(res, allocated1);
	allocated1 = allocate_mem(&main, 200, res);
	print_bit_index(res, allocated1);
	allocated1 = allocate_mem(&main, 200, res);
	print_bit_index(res, allocated1);



	return 0;
}
