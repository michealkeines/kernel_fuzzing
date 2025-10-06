#include <stdint.h>

extern void uart_printf(const char*, ...);


long sys_write_int(uint64_t fd, const char *buf, uint64_t len)
{
    long ret;

    asm volatile(
        "mov x0, %1\n"    // arg0: fd
        "mov x1, %2\n"    // arg1: buf pointer
        "mov x2, %3\n"    // arg2: length
        "mov x8, #1\n"    // syscall number = 1 (sys_write)
        "svc #0\n"        // trigger SVC exception
        "mov %0, x0\n"    // capture return value
        : "=r"(ret)
        : "r"(fd), "r"(buf), "r"(len)
        : "x0", "x1", "x2", "x8", "memory"
    );

    return ret;
}



void user_loop1(void)
{
    uint64_t counter = 1;
    uart_printf("her in 1\n");
    while (1)
    {
        // uart_printf("her in 1 %d\n", counter);
        if (counter == 10000000000) {
            uart_printf("Calling SYSCALL from A\n");
            sys_write_int(1, "User Loop A\n", 10);
            counter = 0;
        }
        counter++;
    }
}


void user_loop2(void)
{
    uint64_t counter = 1;

    while (1)
    {
        // uart_printf("her in 2 %d\n", counter);
        if (counter == 10000000000) {
            uart_printf("Calling SYSCALL from B\n");
            sys_write_int(1, "User Loop B\n", 10);
            counter = 0;
        }
        counter++;
    }
}