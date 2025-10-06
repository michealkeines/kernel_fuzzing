#include <stdint.h>

extern void uart_printf(const char* fmt, ...);

static volatile uint64_t jiffies;

/*
    we need to read from the current freq from special register (CNTFRQ_EL0)
    calculate the ticks from the freq
    write that to special register (CNTV_TVAL_EL0)
    enable timer with writing to special register (CNTV_CTL_EL0)


    CNTV_* register are for virtual timer
    for physical timer, we need to use CNTP_*

    in our case, we will use virtual timer
*/

static inline uint64_t get_current_freq(void)
{
    uint64_t current_freq = 0;
    asm volatile (
        "mrs %0, CNTFRQ_EL0":"=r"(current_freq)
    );

    return current_freq;
}

static inline void write_freq(uint64_t freq)
{
    // uart_printf("Freq going to be written\n");
    asm volatile (
        "msr CNTV_TVAL_EL0, %0"::"r"(freq)
    );
    // uart_printf("Freq written\n");
}

static inline void enable_timmer(void)
{
    asm volatile (
        "msr CNTV_CTL_EL0, %0"::"r"(1ull)
    );
}

void write_ticks(void)
{
    uint64_t current_freq = get_current_freq();
    // uart_printf("Current freq: %d\n", current_freq);
    current_freq = current_freq / 1000;
    write_freq(current_freq);
}

void timer_init(void)
{
    write_ticks();
    enable_timmer();
}

void timer_handler(void)
{
    jiffies++;

    if (jiffies == 1000000) {
        uart_printf("Timer tick\n");
        jiffies = 0;
    }
}