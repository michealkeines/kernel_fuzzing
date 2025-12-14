
#include "mmu.h"


#define PHYS_KERNEL_BASE   0x0000000080000000UL  // kernel load address
#define VIRT_KERNEL_BASE   0xffff000080000000UL  // kernel virtual base
#define PHYS_REGION_2_BASE 0x0000000009000000UL  // second block region
#define VIRT_REGION_2_BASE 0xffff000082400000UL  // virtual for UART

#define L0_BASE_PHYS   0x0000000080020000UL
#define L1_BASE_PHYS   0x0000000080040000UL
#define L2_BASE_PHYS   0x0000000080060000UL
#define L2_0_BASE_PHYS   0x0000000080080000UL
#define L2_1_BASE_PHYS   0x00000000800A0000UL

#define DESC_VALID         (1UL << 0)
#define DESC_TABLE         (1UL << 1)
#define DESC_AF            (1UL << 10)
#define DESC_BLOCK         (1UL << 0)

#define get_pa_identity(va) \
 0x0000FFFFFFFFFFFF & (va)
// 01000 => 00100 => 00010 => 00001

uint64_t allocate_mem(AllocateMem *mem, uint64_t size, BitIndex *res);

bool set_bitmap(AllocateMem *mem, uint64_t size, BitIndex *res);

bool map_pagetable_entry(uint64_t va, uint64_t pa, uint64_t total);


static inline uint64_t calc_index_l0(uint64_t va) { return (va >> 39) & 0x1FF; }
static inline uint64_t calc_index_l1(uint64_t va) { return (va >> 30) & 0x1FF; }
static inline uint64_t calc_index_l2(uint64_t va) { return (va >> 21) & 0x1FF; }
#define PTE_ATTR_NORMAL  (1ULL << 2)   // AttrIdx = 1
#define PTE_SH_INNER     (3ULL << 8)
#define PTE_AF           (1ULL << 10)
#define PTE_AP_RW        (0ULL << 6)
static inline uint64_t make_table_desc(uint64_t next_table_phys_base) {
    uint64_t desc = ((next_table_phys_base & ~0xFFFUL) >> 12) << 12;
    desc |= DESC_VALID | DESC_TABLE;
    return desc;
}

static inline uint64_t make_block_desc(uint64_t phys_base) {
    uint64_t desc = ((phys_base & ~0xFFFUL) >> 12) << 12;
    desc |= DESC_VALID | DESC_AF;  // valid + access flag
    // desc |= PTE_ATTR_NORMAL | PTE_SH_INNER | PTE_AF | PTE_AP_RW;
    return desc;
}


static inline void clear_table(uint64_t table_phys_base) {
    uint64_t *tbl = (uint64_t*)table_phys_base;
    for (int i = 0; i < 512; i++)
        tbl[i] = 0;
}

static inline void set_table_entry(uint64_t *table, uint64_t idx, uint64_t desc) {
    table[idx] = desc;
    //    // Ensure the descriptor write is visible
    //    __asm__ volatile("dsb ishst" ::: "memory");

    //    // Invalidate TLB entry for this VA (or all if simpler)
    //    __asm__ volatile("tlbi vmalle1" ::: "memory");
   
    //    // Ensure TLB invalidation completed
    //    __asm__ volatile("dsb ish" ::: "memory");
    //    __asm__ volatile("isb");
}


void mmu_init(void)
{
    /* clear all tables */
    clear_table(L0_BASE_PHYS);
    clear_table(L1_BASE_PHYS);
    clear_table(L2_BASE_PHYS);
    clear_table(L2_0_BASE_PHYS);
    clear_table(L2_1_BASE_PHYS);


    // i need create identity mapping, that is why i need add entry for both va and pa

    // 0xffff000080000 => entry porting 0x800000 
    // 0x80000000 => 0x8000000

    // as soon as we enable MMU, we will get a fault
    
    /* ---- L0: map kernel VA region ---- */
    uint64_t l0_idx = calc_index_l0(VIRT_KERNEL_BASE);
    uint64_t l0_desc = make_table_desc(L1_BASE_PHYS);
    set_table_entry((uint64_t*)L0_BASE_PHYS, l0_idx, l0_desc);

     l0_idx = calc_index_l0(PHYS_KERNEL_BASE);
     l0_desc = make_table_desc(L1_BASE_PHYS);
    set_table_entry((uint64_t*)L0_BASE_PHYS, l0_idx, l0_desc);

    /* ---- L1: map kernel VA → L2 ---- */
    uint64_t l1_idx = calc_index_l1(VIRT_KERNEL_BASE);
    uint64_t l1_desc = make_table_desc(L2_BASE_PHYS);
    set_table_entry((uint64_t*)L1_BASE_PHYS, l1_idx, l1_desc);

     l1_idx = calc_index_l1(PHYS_KERNEL_BASE);
     l1_desc = make_table_desc(L2_BASE_PHYS);
    set_table_entry((uint64_t*)L1_BASE_PHYS, l1_idx, l1_desc);

    /* ---- L2: block mapping 0x0000000080000000 → 0x80000000 ---- */
    uint64_t l2_idx = calc_index_l2(VIRT_KERNEL_BASE);
    uint64_t l2_desc = make_block_desc(PHYS_KERNEL_BASE);
    set_table_entry((uint64_t*)L2_BASE_PHYS, l2_idx, l2_desc);

     l2_idx = calc_index_l2(PHYS_KERNEL_BASE);
     l2_desc = make_block_desc(PHYS_KERNEL_BASE);
    set_table_entry((uint64_t*)L2_BASE_PHYS, l2_idx, l2_desc);

    /* ---- L2: second block (0x0000000082400000 → 0x90000000) ---- */
    l2_idx = calc_index_l2(VIRT_REGION_2_BASE);
    uint64_t l2_desc2 = make_block_desc(PHYS_REGION_2_BASE);
    set_table_entry((uint64_t*)L2_BASE_PHYS, l2_idx, l2_desc2);

    // l2_idx = calc_index_l2(0x80a00000);
    // l2_desc2 = make_block_desc(PHYS_REGION_2_BASE);
    // set_table_entry((uint64_t*)L2_BASE_PHYS, l2_idx, l2_desc2);

    uint64_t mair =
    (0x00ULL << 0) |     // AttrIdx 0: Device-nGnRnE
    (0xFFULL << 8);  

    uint64_t tcr = 0;

    tcr |= (16ULL << 0);    // T0SZ = 16 (48-bit)
    tcr |= (16ULL << 16);   // T1SZ = 16 (48-bit)
    
    tcr |= (0ULL << 14);    // TG0 = 4KB
    tcr |= (2ULL << 30);    // TG1 = 4KB
    
    tcr |= (3ULL << 12);    // SH0 = Inner Shareable
    tcr |= (3ULL << 28);    // SH1 = Inner Shareable
    
    tcr |= (1ULL << 10);    // ORGN0 = WB
    tcr |= (1ULL << 8);     // IRGN0 = WB
    tcr |= (1ULL << 26);    // ORGN1 = WB
    tcr |= (1ULL << 24);    // IRGN1 = WB
    
    tcr |= (5ULL << 32);    // IPS = 48-bit PA
    uint64_t ttbr1 = L0_BASE_PHYS;

    __asm__ volatile(
        "msr MAIR_EL1, %[mair]\n"
        "msr TCR_EL1,  %[tcr]\n"
        "msr TTBR1_EL1,%[ttbr1]\n"
        "msr TTBR0_EL1,%[ttbr1]\n" // TODO: fix this, user space should have access this page table entries
        "dsb ishst\n"
        "isb\n"
        "mrs x0, SCTLR_EL1\n"
        "orr x0, x0, #(1 << 0)\n"   // M (enable MMU)
        "orr x0, x0, #(1 << 2)\n"   // C (enable data cache)
        "orr x0, x0, #(1 << 12)\n"  // I (enable instruction cache)
        "msr SCTLR_EL1, x0\n"
        "dsb sy\n"
        "isb\n"
        :
        : [mair]"r"(mair), [tcr]"r"(tcr), [ttbr1]"r"(ttbr1)
        : "x0", "memory");
}



void *mem_block(uint64_t size)
{
    uint64_t MAX = 524288; 
    if (size >= MAX) return 0;

    return (void *) __block_memory_start;
}





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
#define PAGE_SHIFT 12
#define PAGE_SIZE  (1UL << PAGE_SHIFT)

#define PHYS_BASE_ADDR  0x800B0000UL // this is our physical pages will mapped
#define PHYS_BASE_PAGE  (PHYS_BASE_ADDR >> PAGE_SHIFT)

uint64_t bitindex_to_phys(BitIndex bi)
{
    uint64_t bitmap_page = bi.index * 8 + bi.bit;
    uint64_t phys_page   = PHYS_BASE_PAGE + bitmap_page;
    return phys_page << PAGE_SHIFT;
}


bool set_bitmap(AllocateMem *mem, uint64_t size, BitIndex *res)
{
	uint64_t t = 0;
    uart_printf("size in setbitmap: %d\n", size);

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
/*
we are returning the total number of pages that are allocated and also the BitIndex within the map

eg: lets saw we have 3 page

17, 18, 19

our res will be a array of BitIndex

BitIndex for 17
BitIndex for 18
BitIndex for 19 

to calculate the starting address of a page

1 => 0x0 to 0x1000 => 0 to 4096
2 => 0x1000 to 0x2000 => 4097 to 8192
17 =>  0x10000 to 0x11000 => 4096 * 17 = 69632


PA 0x11000 => VA 0xffff000000011000

update the page tables for the VA


we cant use 0x000000000000 to 0x3ffffffffffff something, because qemu use it for something, ram is mapped at 0x800000000

so we sshould keep our page table mapped from there too, it took a whole day to figure this issue, so next time

remember, physical page entries are starting at this location

we are already have l1 table used in mmu_init, we reuse that untill we use 1GP, next we will need new l1 table

*/

uint64_t kmalloc(AllocateMem *mem, uint64_t size)
{
    BitIndex res[64];
    uart_printf("res: %l\n", res);

    uint64_t total_allocated = allocate_mem(mem, size, res);
    uart_printf("total allocated: %l\n", total_allocated);
    BitIndex current = res[0];
    uint64_t index = current.index;
    uart_printf("current index: %d\n", index);

    uint64_t pa = bitindex_to_phys(current);
    // 0x1000 => 0xFFFF00000001000
    uint64_t va = (pa | 0xFFFF000000000000);
    uart_printf("for va: %l\n", va);

    // map pages table entry for that va
    bool allocated = map_pagetable_entry(va, pa, total_allocated);
    uart_printf("for allcoated done: %l\n", va);

    return va;
}


/*
    - update kernel page table, what is the registry = TTBR1_EL1
    - it can either have table discriptor or block discriptor

    eg: 48bit va
        0xFFFF000000001000 => 0x0000000000001000


*/
bool map_pagetable_entry(uint64_t va, uint64_t pa, uint64_t total) 
{
    if (total > 1)
    {
        uart_printf("you odnt have that much ram");
        return false;
    }
    // TODO: create separate L2 table for every index
    for (int64_t i = 0; i < total; ++i)
    {
        // As our physical pages start from 8000000 something, we already have table entyr for it, that covers 1GP of l1, if need more we can use L2_0 or L2_1 TODO: make this dynamic
        uint64_t current_l2 = (i == 0) ? L2_BASE_PHYS : L2_0_BASE_PHYS;
        // uint64_t current_l2 = L2_BASE_PHYS;
        // va 0xffff000000000000 => 0x000000000000000
        uint64_t current_va = va + (4096 * i);

        // TODO: currently pa is hardcoded, so for i = 1, we will same pa, that is gonna break things here
        // i need to add a L1 entry
        uint64_t l1_idx = calc_index_l1(current_va);
        uint64_t l1_desc = make_table_desc(current_l2);
        set_table_entry((uint64_t*)L1_BASE_PHYS, l1_idx, l1_desc);
        // va 0x0000000000000000 => 0x000000000000000
        l1_idx = calc_index_l1(pa);
        l1_desc = make_table_desc(current_l2);
        set_table_entry((uint64_t*)L1_BASE_PHYS, l1_idx, l1_desc);
        // i need to add a L2 entry
        uint64_t l2_idx = calc_index_l2(current_va);
        uint64_t l2_desc = make_block_desc(pa);
        set_table_entry((uint64_t*)current_l2, l2_idx, l2_desc);
        l2_idx = calc_index_l2(pa);
        l2_desc = make_block_desc(pa);
        set_table_entry((uint64_t*)current_l2, l2_idx, l2_desc);
    }


    return true;
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




// void print_bit_index(BitIndex *res, uint64_t size)
// {
// 	for (uint64_t i = 0; i < size; ++i)
// 	{
// 		printf("Index: %llu, Bit: %x\n", res[i].index, res[i].bit);
// 	}
// }

void free_pages(AllocateMem *mem, BitIndex *res, uint64_t size)
{
	for (uint64_t i = 0; i < size; ++i)
	{
		deallocate_mem(mem, res[i].index, res[i].bit);
	}
}



// void mmu_init_4k_80000(void)
// {
//     /* clear all tables */
//     clear_table(L0_BASE_PHYS);
//     clear_table(L1_BASE_PHYS);
//     clear_table(L2_BASE_PHYS);

//     /* ---- L0: map kernel VA region ---- */
//     uint64_t l0_idx = calc_index_l0(VIRT_KERNEL_BASE);
//     uint64_t l0_desc = make_table_desc(L1_BASE_PHYS);
//     set_table_entry((uint64_t*)L0_BASE_PHYS, l0_idx, l0_desc);

//     /* ---- L1: map kernel VA → L2 ---- */
//     uint64_t l1_idx = calc_index_l1(VIRT_KERNEL_BASE);
//     uint64_t l1_desc = make_table_desc(L2_BASE_PHYS);
//     set_table_entry((uint64_t*)L1_BASE_PHYS, l1_idx, l1_desc);

//     /* ---- L2: block mapping 0x0000000080000000 → 0x80000000 ---- */
//     uint64_t l2_idx = calc_index_l2(VIRT_KERNEL_BASE);
//     uint64_t l2_desc = make_block_desc(PHYS_KERNEL_BASE);
//     set_table_entry((uint64_t*)L2_BASE_PHYS, l2_idx, l2_desc);

//     /* ---- L2: second block (0x0000000082400000 → 0x90000000) ---- */
//     l2_idx = calc_index_l2(VIRT_REGION_2_BASE);
//     uint64_t l2_desc2 = make_block_desc(PHYS_REGION_2_BASE);
//     set_table_entry((uint64_t*)L2_BASE_PHYS, l2_idx, l2_desc2);

//     uint64_t mair = 0xFF;           // normal memory attributes
//     uint64_t tcr  = 0;

//     tcr |= (0ULL << 14);   // TG0 = 4 KB
//     tcr |= (16ULL);        // T0SZ = 64 - 48 = 16 bits
//     tcr |= (2ULL << 30);   // TG1 = 4 KB
//     tcr |= (16ULL << 16);  // T1SZ = 16 bits

//     uint64_t ttbr1 = L0_BASE_PHYS;

//     __asm__ volatile(
//         "msr MAIR_EL1, %[mair]\n"
//         "msr TCR_EL1,  %[tcr]\n"
//         "msr TTBR0_EL1,%[ttbr1]\n"
//         "dsb ishst\n"
//         "isb\n"
//         "mrs x0, SCTLR_EL1\n"
//         "orr x0, x0, #(1 << 0)\n"   // M (enable MMU)
//         "orr x0, x0, #(1 << 2)\n"   // C (enable data cache)
//         "orr x0, x0, #(1 << 12)\n"  // I (enable instruction cache)
//         "msr SCTLR_EL1, x0\n"
//         "dsb sy\n"
//         "isb\n"
//         :
//         : [mair]"r"(mair), [tcr]"r"(tcr), [ttbr1]"r"(ttbr1)
//         : "x0", "memory");
// }


// void mmu_init_16kb_v2(void)
// {
//     const uint64_t L0_BASE = 0x70004000ULL;
//     const uint64_t L1_BASE = 0x70008000ULL;
//     const uint64_t L2_BASE = 0x7000C000ULL;

//     uint64_t *l0 = (uint64_t*)L0_BASE;
//     uint64_t *l1 = (uint64_t*)L1_BASE;
//     uint64_t *l2 = (uint64_t*)L2_BASE;

//     // Each table = 2048 entries (16 KB)
//     for (int i = 0; i < 2048; i++) {
//         l0[i] = 0;
//         l1[i] = 0;
//         l2[i] = 0;
//     }

//     uint64_t l0_index = (0x80000000UL >> 47);

//     l0[l0_index] = (L1_BASE & ~0x3FFFUL) | 0x3UL; // table descriptor

//     uint64_t table_entry2 = (L2_BASE & ~0x3FFFUL) | 0x3UL;

//     uint64_t l1_index = ((0x80000000UL >> 36) & 0x7FFUL);

//     l1[l1_index] = table_entry2; // table descriptor

//     uint64_t block_entry = (0x80000000UL & ~0x3FFFUL)
//                         | (1ULL << 10)   // AF=1
//                         | 0x1;        // block descriptor

//     uint64_t l2_index1 = ((0x80000000UL >> 25) & 0x7FFUL);
//     l2[l2_index1] = block_entry;

//     block_entry = (0x09000000UL & ~0x3FFFUL)
//                         | (1ULL << 10)   // AF=1
//                         | 0x1;        // block descriptor
    
//     uint64_t l2_index2 = ((0x82400000UL >> 25) & 0x7FFUL);
//     l2[l2_index2] = block_entry;
//     uint64_t mair = 0xFF;

//     uint64_t tcr = 0;

//     // TTBR0_EL1 region (user space)
//     tcr |= (2ULL << 14);   // TG0 = 16 KB granule (bits [15:14] = 10b)
//     tcr |= (16ULL);        // T0SZ = 64 - 48 = 16 bits (bits [5:0])
    
//     // TTBR1_EL1 region (kernel space)
//     tcr |= (1ULL << 30);   // TG1 = 16 KB granule (bits [31:30] = 01b)
//     tcr |= (16ULL << 16);  // T1SZ = 16 (bits [21:16])
    

//     uint64_t ttbr = L0_BASE;

//     __asm__ volatile(
//         "msr MAIR_EL1, %[mair]\n"
//         "msr TCR_EL1,  %[tcr]\n"
//         "msr TTBR0_EL1,%[ttbr]\n"
//         "dsb ishst\n"
//         "isb\n"
//         "mrs x0, SCTLR_EL1\n"
//         "orr x0, x0, #(1 << 0)\n"
//         "orr x0, x0, #(1 << 2)\n"
//         "orr x0, x0, #(1 << 12)\n"
//         "msr SCTLR_EL1, x0\n"
//         "dsb sy\n"
//         "isb\n"
//         :
//         : [mair]"r"(mair), [tcr]"r"(tcr), [ttbr]"r"(ttbr)
//         : "x0", "memory");
// }


// void mmu_init_16kb(void)
// {
//     const uint64_t L0_BASE = 0x80004000ULL;
//     const uint64_t L1_BASE = 0x80008000ULL;
//     const uint64_t L2_BASE = 0x8000C000ULL;

//     uint64_t *l0 = (uint64_t*)L0_BASE;
//     uint64_t *l1 = (uint64_t*)L1_BASE;
//     uint64_t *l2 = (uint64_t*)L2_BASE;

//     // Each table = 2048 entries (16 KB)
//     for (int i = 0; i < 2048; i++) {
//         l0[i] = 0;
//         l1[i] = 0;
//         l2[i] = 0;
//     }

//     l0[0] = (L1_BASE & ~0x3FFFUL) | 0x3UL; // table descriptor

//     l1[2] = (L2_BASE & ~0x3FFFUL) | 0x3UL; // table descriptor

//     uint64_t block_entry = (0x80000000UL & ~0x3FFFUL)
//                          | (1ULL << 10)   // AF=1
//                          | 0x1;        // block descriptor
//     l2[0] = block_entry;

//     block_entry = (0x09000000ULL & ~0x3FFFUL)
//                          | (1ULL << 10)   // AF=1
//                          | 0x1;        // block descriptor
//     l2[18] = block_entry;

//     uint64_t mair = 0xFF;

//     uint64_t tcr = 0;
//     tcr |= (2ULL << 14);   // TG0 = 16 KB granule (bits 
//     tcr |= (16ULL);        // T0SZ = 64 − 48 = 16 (48-bit VA)

//     uint64_t ttbr = L0_BASE;

//     __asm__ volatile(
//         "msr MAIR_EL1, %[mair]\n"
//         "msr TCR_EL1,  %[tcr]\n"
//         "msr TTBR0_EL1,%[ttbr]\n"
//         "dsb ishst\n"
//         "isb\n"
//         "mrs x0, SCTLR_EL1\n"
//         "orr x0, x0, #(1 << 0)\n"
//         "orr x0, x0, #(1 << 2)\n"
//         "orr x0, x0, #(1 << 12)\n"
//         "msr SCTLR_EL1, x0\n"
//         "dsb sy\n"
//         "isb\n"
//         :
//         : [mair]"r"(mair), [tcr]"r"(tcr), [ttbr]"r"(ttbr)
//         : "x0", "memory");
// }




// void mmu_init_4kb(void)
// {
//     const uint64_t L0_BASE = 0x80002000UL;
//     const uint64_t L1_BASE = 0x80003000UL;
//     const uint64_t L2_BASE = 0x80004000UL;

//     for (int i = 0; i < 512; i++) {
//         ((uint64_t*)L0_BASE)[i] = 0;
//         ((uint64_t*)L1_BASE)[i] = 0;
//         ((uint64_t*)L2_BASE)[i] = 0;
//     }

//     uint64_t table_entry = 0x0000000080003; // base address of next table, leaving out last bits

//     table_entry = table_entry << 12; // shift left everything to start from 12 bit
//     table_entry = table_entry | 0x3; // set as valid table descriptor
//     // rest of the bits are 0 by default

//     // caculate index for L0
//     uint16_t l0_index = (0x80000000UL >> 39);

//     ((uint64_t*) L0_BASE)[l0_index] = table_entry;

//     uint64_t table_entry2 = 0x0000000080004; // base address of next table, leaving out last bits

//     table_entry2 = table_entry2 << 12; // shift left everything to start from 12 bit
//     table_entry2 = table_entry2 | 0x3; // set as valid table descriptor
//     // rest of the bits are 0 by default

//     // caculate index for L1
//     uint16_t l1_index = (0x80000000UL >> 30 & 0x1FF);

//     ((uint64_t*) L1_BASE)[l1_index] = table_entry2; // 3rd entry will be 0x80000000

//     uint64_t block_entry = 0x0000000080000; // Physicla map base address

//     block_entry = block_entry << 12; // shift left everything to start from 12 bit
//     block_entry = block_entry | (1UL << 10); // set AF bit
//     block_entry = block_entry | 0x1; // set as valid block descriptor

//     // caculate index for L1
//     uint16_t l2_index = (0x80000000UL >> 21 & 0x1FF);

//     ((uint64_t*)L2_BASE)[l2_index] = block_entry;

//     uint64_t block_entry2 = 0x000000009000; // Physicla map base address
//     block_entry2 = block_entry2 << 12; // shift left everything to start from 12 bit
//     block_entry2 = block_entry2 | (1UL << 10); // set AF bit
//     block_entry2 = block_entry2 | 0x1; // set as valid block descriptor

//     l2_index = (0x82400000UL >> 21 & 0x1FF);

//     ((uint64_t*) L2_BASE)[l2_index] = block_entry2;

//     uint64_t tcr = 0;

//     tcr |= 16; // we want 48 bit va, so 64 - 48 = 16, 16 should be place in T0SZ, TG0 needs to be 4k, so that 00, which is already 00 in our case
//     uint64_t ttbr = L0_BASE;


//     uint64_t mair = (0xFF << 0); // set everythign as normal and accessible

//     __asm__ volatile(
//         "msr MAIR_EL1, %[mair]\n"
//         "msr TCR_EL1,  %[tcr]\n"
//         "msr TTBR0_EL1,%[ttbr]\n"
//         "dsb ishst\n"
//         "isb\n"
//         "mrs x0, SCTLR_EL1\n"
//         "orr x0, x0, #(1 << 0)\n"
//         "orr x0, x0, #(1 << 2)\n"
//         "orr x0, x0, #(1 << 12)\n"
//         "msr SCTLR_EL1, x0\n"
//         "dsb sy\n"
//         "isb\n"
//         :
//         : [mair]"r"(mair), [tcr]"r"(tcr), [ttbr]"r"(ttbr)
//         : "x0", "memory");
// }

// void mmu_init_old(void)
// {
//     const uint64_t L1_BASE = 0x80002000UL;
//     const uint64_t L2_BASE = 0x80003000UL;

//     for (int i = 0; i < 512; i++) {
//         ((uint64_t*)L1_BASE)[i] = 0;
//         ((uint64_t*)L2_BASE)[i] = 0;
//     }

//     ((uint64_t*)L1_BASE)[2] = (L2_BASE | 0b11); // table descriptor

//     // Map RAM (0x80000000)
//     ((uint64_t*)L2_BASE)[0] =
//         (0x80000000UL & 0xFFFFFFE00000UL) |
//         (1 << 0) | (0 << 2) | (1 << 10) | (3 << 8);

//     // Map UART (as normal memory) at VA 0x82400000 → PA 0x09000000
//     ((uint64_t*)L2_BASE)[0x12] =
//     (0x09000000UL & 0xFFFFFFE00000UL) |
//     (1 << 0) | (0 << 2) | (1 << 10) | (0 << 8);

//     uint64_t mair = (0xFF << 0) | (0x04 << 8); // Normal + Device
//     uint64_t tcr  = 0x0000000000003520UL;
//     uint64_t ttbr = L1_BASE;

//     __asm__ volatile(
//         "msr MAIR_EL1, %[mair]\n"
//         "msr TCR_EL1,  %[tcr]\n"
//         "msr TTBR0_EL1,%[ttbr]\n"
//         "dsb ishst\n"
//         "isb\n"
//         "mrs x0, SCTLR_EL1\n"
//         "orr x0, x0, #(1 << 0)\n"
//         "orr x0, x0, #(1 << 2)\n"
//         "orr x0, x0, #(1 << 12)\n"
//         "msr SCTLR_EL1, x0\n"
//         "dsb sy\n"
//         "isb\n"
//         :
//         : [mair]"r"(mair), [tcr]"r"(tcr), [ttbr]"r"(ttbr)
//         : "x0", "memory");
// }
