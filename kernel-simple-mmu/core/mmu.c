#include <stdint.h>


#define PHYS_KERNEL_BASE   0x0000000080000000UL  // kernel load address
#define VIRT_KERNEL_BASE   0xffff000080000000UL  // kernel virtual base
#define PHYS_REGION_2_BASE 0x0000000009000000UL  // second block region
#define VIRT_REGION_2_BASE 0xffff000082400000UL  // virtual for UART

#define L0_BASE_PHYS   0x0000000080002000UL
#define L1_BASE_PHYS   0x0000000080004000UL
#define L2_BASE_PHYS   0x0000000080008000UL

#define DESC_VALID         (1UL << 0)
#define DESC_TABLE         (1UL << 1)
#define DESC_AF            (1UL << 10)
#define DESC_BLOCK         (1UL << 0)

static inline uint64_t calc_index_l0(uint64_t va) { return (va >> 39) & 0x1FF; }
static inline uint64_t calc_index_l1(uint64_t va) { return (va >> 30) & 0x1FF; }
static inline uint64_t calc_index_l2(uint64_t va) { return (va >> 21) & 0x1FF; }

static inline uint64_t make_table_desc(uint64_t next_table_phys_base) {
    uint64_t desc = ((next_table_phys_base & ~0xFFFUL) >> 12) << 12;
    desc |= DESC_VALID | DESC_TABLE;
    return desc;
}

static inline uint64_t make_block_desc(uint64_t phys_base) {
    uint64_t desc = ((phys_base & ~0xFFFUL) >> 12) << 12;
    desc |= DESC_VALID | DESC_AF;  // valid + access flag
    return desc;
}

static inline void clear_table(uint64_t table_phys_base) {
    uint64_t *tbl = (uint64_t*)table_phys_base;
    for (int i = 0; i < 512; i++)
        tbl[i] = 0;
}

static inline void set_table_entry(uint64_t *table, uint64_t idx, uint64_t desc) {
    table[idx] = desc;
}


void mmu_init(void)
{
    /* clear all tables */
    clear_table(L0_BASE_PHYS);
    clear_table(L1_BASE_PHYS);
    clear_table(L2_BASE_PHYS);

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

    uint64_t mair = 0xFF;           // normal memory attributes
    uint64_t tcr  = 0;

    tcr |= (0ULL << 14);   // TG0 = 4 KB
    tcr |= (16ULL);        // T0SZ = 64 - 48 = 16 bits
    tcr |= (2ULL << 30);   // TG1 = 4 KB
    tcr |= (16ULL << 16);  // T1SZ = 16 bits

    uint64_t ttbr1 = L0_BASE_PHYS;

    __asm__ volatile(
        "msr MAIR_EL1, %[mair]\n"
        "msr TCR_EL1,  %[tcr]\n"
        "msr TTBR1_EL1,%[ttbr1]\n"
        "msr TTBR0_EL1,%[ttbr1]\n"
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
