#include "syscall.h"


// int64_t sys_write1(uint64_t fd, char *buff, uint64_t size)
// {
//     int64_t result;
//     asm volatile (
//         "mov x8, #64 \t\n"
//         "svc #0"
//         : "=r" (result)
//         : "r" (fd), "r" (buff), "r" (size)
//         : "x8", "memory"
//     );
//     return result;
// }