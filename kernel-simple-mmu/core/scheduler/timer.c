#include <stdint.h>

extern void uart_printf(const char* fmt, ...);
extern void uart_puts(const char*);
extern void gic_eoi(unsigned int);
extern unsigned int gic_ack(void);

volatile unsigned long jiffies; // our internal counter
static unsigned long cntfrq; // current frequency

// this will use to set the frequency,
// eg: interrupt me every 1ms
static inline void msr_cntv_tval(uint64_t v)
{
    asm volatile(
        "msr CNTV_TVAL_EL0, %0"::"r"(v)
    );
}

// Enable Timer interrupts, it has its own arch specific register
void enable_cntv(void)
{
    asm volatile(
        "msr CNTV_CTL_EL0, %0"::"r"(1ull)
    );
}

void disable_cntv(void)
{
    asm volatile(
        "msr CNTV_CTL_EL0, %0"::"r"(0ull)
    );
}

// Read the actual frequency of the CPU,
// we use this to set our internal interrupt timing
static inline void read_cntfrq(void)
{
    asm volatile(
        "mrs %0, CNTFRQ_EL0":"=r"(cntfrq)
    );
}

void set_cntv(void)
{
    read_cntfrq();
    uart_printf("read back: %l\n", cntfrq);
    uint64_t our_freq = cntfrq / 10; // we want the interrupt every 1ms
    uart_printf("setting back: %l\n", our_freq);
    msr_cntv_tval(our_freq);
}

void timer_init_1khz(void)
{
    read_cntfrq(); // read curren freq
    set_cntv();
    enable_cntv(); // enable timer interrupts
}

