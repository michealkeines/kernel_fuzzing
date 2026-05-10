#include "spinlock.h"


/*

we have the compare and swap logic using memory tagging instrucctions

and we are able to sync it

*/

int32_t __attribute__((noinline)) spin_lock(SpinLock *lock)
{
    int32_t temp = 0;
    int32_t result = 0;

    asm volatile (
        "1: \n\t"
        "LDAXR %w0, [%3] \n\t"
        "CBNZ  %w0, 1b \n\t"
        "MOV   %w0, #1 \n\t"
        "STLXR %w1, %w0, [%3]\n\t"
        "CBNZ  %w1, 1b \n\t"
        : "=&r" (temp), "=&r" (result)
        : "0" (temp), "r" (&lock->lock)
        : "memory"
    );

    // printf("locked\n");

    return result;
}


int32_t __attribute__((noinline)) spin_unlock(SpinLock *lock)
{
    int32_t temp = 0;
    int32_t result = 0;

    asm volatile (
        "1: \n\t"
        "LDAXR %w0, [%3] \n\t"
        // "CBNZ  %w1, 1b \n\t"
        "MOV   %w0, #0 \n\t"
        "STLXR %w1, %w0, [%3]\n\t"
        "CBNZ  %w1, 1b \n\t"
        : "=&r" (temp), "=&r" (result)
        : "0" (temp), "r" (&lock->lock)
        : "memory"
    );

    // printf("unlocked\n");
    return result;
}

static inline void mask_interrupts()
{ 
    // msr daifset, #0xf, copied from start.s
	__asm__ volatile("msr daifset, #0xf");
}
static inline void unmask_interrupts()
{ 
    // copied it form kmain
	__asm__ volatile("msr DAIFCLr, #2");
}

int32_t spin_lock_with_mask(SpinLock *lock)
{
    mask_interrupts();
    return spin_lock(lock);
}
int32_t spin_unlock_with_mask(SpinLock *lock)
{
    int32_t result = spin_unlock(lock);
    unmask_interrupts();
    return result;
}
// SpinLock l = {0};

// volatile int counter = 0;

// void *run(void * arg)
// {
//     for (int i = 0; i < 2500; i++)
//     {
//         spin_lock(&l);
//         counter += 1;
//         spin_unlock(&l);
//     }
//     return 0;
// }

// int main(void)

// {

//     pthread_t p1, p2;
//     printf("coutner: %d\n", counter);

//     pthread_create(&p1, NULL, run, NULL);
//     pthread_create(&p2, NULL, run, NULL);

//     pthread_join(p1, NULL);
//     pthread_join(p2, NULL);




//     printf("coutner: %d\n", counter);
//     return 0;
// }
