#include <stdio.h>
#include <stdint.h>

// Helper function to print 64-bit unsigned integer as binary
void print_binary(uint64_t value) {
    for (int i = 63; i >= 0; i--) {
        printf("%llu", (value >> i) & 1);
        if (i % 8 == 0) printf(" "); // spacing every 8 bits for readability
    }
    printf("\n");
}

uint64_t flip_bit(uint64_t value, unsigned int index) {
    if (index >= 64) {
        return value;
    }
    return value | ((uint64_t)1 << index);
}

uint64_t flip_bit_range(uint64_t value, unsigned int start, unsigned int end) {
    if (start > end || end >= 64) {
        return value;
    }

    for (unsigned int i = start; i <= end; i++) {
        value = flip_bit(value, i);
    }
    return value;
}


int main() {
uint64_t tcr = 0;

// T0SZ = 16 (bits [5:0])
tcr |= (uint64_t)16;   // just assign directly since 16 fits in lower bits

// IRGN0 = 0b01 → bit 8
tcr = flip_bit(tcr, 8);

// ORGN0 = 0b01 → bit 10
tcr = flip_bit(tcr, 10);

// SH0 = 0b11 → bits 12–13
tcr = flip_bit_range(tcr, 12, 13);

// TG0 = 0b00 → nothing

// IPS = 0b101 → bits 32 and 34
tcr = flip_bit(tcr, 32);
tcr = flip_bit(tcr, 34);

// AS = 0 → nothing

// TBI0 = bit 38
tcr = flip_bit(tcr, 38);

// HA = bit 39
tcr = flip_bit(tcr, 39);

// HD = bit 40
tcr = flip_bit(tcr, 40);
printf("TCR_EL1 = 0x%016llx\n", tcr);
    return 0;
}

