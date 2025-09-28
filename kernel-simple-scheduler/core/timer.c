#include <stdint.h>

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
static inline void enable_cntv(void)
{
    asm volatile(
        "msr CNTV_CTL_EL0, %0"::"r"(1ull)
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
    uint64_t our_freq = cntfrq / 1000; // we want the interrupt every 1ms
    msr_cntv_tval(our_freq);
}

void timer_init_1khz(void)
{
    read_cntfrq(); // read curren freq
    set_cntv();
    enable_cntv(); // enable timer interrupts
}

