#ifndef ___SPINLOCK___
#define ___SPINLOCK___

#include <stdint.h>

typedef struct spinlock {
    volatile uint32_t lock; // 0 => not locked, 1 => it is locked
} SpinLock;

int32_t spin_lock(SpinLock *lock);
int32_t spin_unlock(SpinLock *lock);

int32_t spin_lock_with_mask(SpinLock *lock);
int32_t spin_unlock_with_mask(SpinLock *lock);



/*

a spin lock

- case were we just lock, get into the critical section, finish the work and unlock, no interrupts or we didnt get scheduled off

- case were we just lock, get into the critical section, and did some work, now we get scheduled off during the critical section, then we other process just keep waitin untill it cpu time slice is over and come abck here and finish the critical section

- case were we just lock, same as second case, but an irq tries to use the lock? will it wait forever, as there is not cpu schediled off for irq? so we need to make sure our lock will be used by any other irq

- case were we sepcifical tethe 


the test case should be a two use process trying to call block_write operations



LDAXR sets the monitor: When LDAXR w1, [x0] executes, it loads the value from the address in x0 and tags that physical address (or more precisely, the Exclusives Reservation Granule (ERG), typically a cache line) in the processor's exclusive monitor as being under exclusive access. 
Monitor is cleared on modification: If any other processor (or core) performs a store operation to any address within that same ERG, the exclusive monitor's tag is cleared. 
STXR checks the monitor: When STXR w3, w2, [x0] executes, it only performs the store if the exclusive monitor's tag for that address is still set. It returns 0 in w3 on success (tag was set) and 1 on failure (tag was cleared). 


This hardware-level coordination ensures that the state of the exclusive monitor is checked and updated atomically, making it impossible for two cores to successfully perform a STXR on the same location simultaneously.

*/

#endif // ___SPINLOCK___