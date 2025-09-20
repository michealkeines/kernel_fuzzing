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

static inline void set_cntv(void)
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


void timer_irq_handler(void)
{
    jiffies++; // if we are here, we need to increament our internal counter, because we got an interrupt request

    // now we can log it, lets log it after every 1000 jiffies
    if ((jiffies % 1000ul) == 0)
    {
        uart_puts("Done one Tick, we are in EL1\n");
    }

    set_cntv(); // set it back again, so it will interrupt us back in 1ms

}


// EL1 irq handler
void el1_irq(void)
{
    // uart_puts("EL1: irq handler (started))\n");
    unsigned int iar = gic_ack(); // we get the current interrupt
    unsigned int id = iar & 0x3FF;

    if (id == 27) timer_irq_handler(); // if it is timmer, we call the handler
    gic_eoi(iar); // we say that we are done now
}