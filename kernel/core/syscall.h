

#ifndef ___SYSCALL___
#define ___SYSCALL___

#include <stdint.h>

int64_t sys_write(uint64_t fd, char *buff, uint64_t size);

#endif // ___SYSCALL___