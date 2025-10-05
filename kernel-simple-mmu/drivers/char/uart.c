/*
UART is just a common way to connect and send bit based on time difference (baudrate)

both parties will agree on a frame and baudrate

*/

#include <stdint.h>
#include <stdarg.h>

#define UART    0x09000000UL  // Base Address
#define DR      (UART + 0x00) // Data register
#define FR      (UART + 0x18) // Status Flag
#define IBRD    (UART + 0x24)
#define FBRD    (UART + 0x28)
#define LCRH    (UART + 0x2C)
#define CR      (UART + 0x30)
#define IMSC    (UART + 0x38)
#define ICR     (UART + 0x44)

static inline void mmio_w32(uint64_t addr, uint32_t v){ *(volatile uint32_t*)addr = v; }
static inline uint32_t mmio_r32(uint64_t addr){ return *(volatile uint32_t*)addr; }

void uart_init(void)
{
    // 1) Disable UART while configuring
    mmio_w32(CR, 0);

    // 2) Clear pending interrupts
    mmio_w32(ICR, 0x7FF);

    // 3) Baud ~115200 with 24 MHz refclk (QEMU virt default)
    //    Divisor = 24_000_000 / (16 * 115200) = 13.0208 → IBRD=13, FBRD≈2
    mmio_w32(IBRD, 13);
    mmio_w32(FBRD, 2);

    // 4) 8N1, FIFO enabled: WLEN=0b11 (bits 6:5), FEN=1 (bit 4)
    mmio_w32(LCRH, (3u << 5) | (1u << 4));

    // 5) Mask all UART interrupts (polling mode for now)
    mmio_w32(IMSC, 0);

    // 6) Enable UART, TX, RX: UARTEN(bit0), TXE(bit8), RXE(bit9)
    mmio_w32(CR, (1u << 0) | (1u << 8) | (1u << 9));

    (void)mmio_r32(FR);
}


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

void int2str(int d)
{
    // handle zero explicitly
    if (d == 0) {
        uart_putc('0');
        return;
    }

    // handle negative numbers
    if (d < 0) {
        uart_putc('-');
        d = -d;
    }

    char buf[2048];   // enough for 32-bit integers
    int i = 0;

    // extract digits (in reverse order)
    while (d > 0 && i < sizeof(buf)) {
        int digit = d % 10;
        buf[i++] = '0' + digit;   // convert to ASCII
        d = d / 10;
    }

    // print digits in correct order
    while (--i >= 0) {
        uart_putc(buf[i]);
    }
}

int strulen(const char *s) {
    const char *p = s;
    while (*p) p++;
    return p - s;
}

void uart_printf(const char* fmt, ...)
{
    va_list fmt_args;
    int i = 0;
    va_start(fmt_args, fmt);
    // uart_puts(fmt);
    int full = strulen(fmt);
    while (i < full)
    {
        char current = fmt[i];
        // uart_putc(current);
        if (current != '%')
        {
            uart_putc(current);
        }
        else if (current == '%')
        {
            // get past %
            i++;
            char format_specifier = fmt[i];
            int c;
            int d;
            const char *s;
            switch (format_specifier)
            {
            case 'c':
                c = va_arg(fmt_args, int);
                uart_putc(c);
                break;
            case 'd':
                d = va_arg(fmt_args, int);
                int2str(d);
                break;
            case 's':
                s = va_arg(fmt_args, char *);
                uart_puts(s);
                break;
            default:
                s = va_arg(fmt_args, char *);
                uart_puts(s);
                break;
            }

        } else {
            uart_putc(current);
        }

        i++;
    }

    va_end(fmt_args);
}


// Tests

// uart_printf("kernal Starting\n");
// uart_printf("Count %d\n", 345);
// uart_printf("full %s\n", "sdfsd");
// uart_printf("char %c\n", 'a');