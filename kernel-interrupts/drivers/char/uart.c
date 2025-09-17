/*
UART is just a common way to connect and send bit based on time difference (baudrate)

both parties will agree on a frame and baudrate

*/

#include <stdint.h>

#define UART    0x09000000UL  // Base Address
#define DR      (UART + 0x00) // Data register
#define FR      (UART + 0x18) // Status Flag

static inline void write_data(uint32_t value)
{
    *(volatile uint32_t*)DR = value;
}

static inline uint32_t read_status()
{
    return *(volatile uint32_t*)FR;
}

void uart_putc(char c)
{
    while (read_status() & (1<<5)) {} // wait for the status flag and the proceed to writing

    write_data((uint32_t)c);
}

void uart_puts(const char* s)
{
    while(*s) uart_putc(*s++);
}