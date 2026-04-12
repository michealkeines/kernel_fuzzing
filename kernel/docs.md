# Kernel Learning Notes

What I built, what I assumed to make it work, how real kernels do it differently, and what I need to learn to close each gap.

---

## 1. Boot & Startup

### What I learned

- The CPU starts executing at `_start` in physical address space. Before the MMU is on, every address is physical.
- I need to disable interrupts immediately (`msr daifset, #0xf`) because there are no handlers installed yet. Any interrupt would crash.
- `SPSel = 1` makes SP refer to SP_EL1, giving the kernel its own stack separate from user mode.
- BSS must be zeroed because the C language guarantees globals start at zero, but the loader doesn't guarantee memory is clean.
- After enabling the MMU, I can't keep running at the old physical address -- I need to jump to the virtual address. That's the `virt_entry` trampoline.

### What I assumed

**"QEMU drops me at EL1 with a flat physical address space."**
This is true for QEMU's `-device loader`, but real hardware boots at EL3 (secure firmware). The firmware initializes security state, drops to EL2 (hypervisor), which then drops to EL1 (OS). I skip EL3 and EL2 entirely.

**"I can just hardcode the stack at 0x801F0000."**
I picked an address that I knew was past my kernel image and page tables. A real kernel computes the stack location from linker symbols (`__kernel_end + STACK_SIZE`), so it adapts as the kernel grows.

**"Identity mapping during boot is enough."**
I map both `0x80000000` and `0xFFFF000080000000` to the same physical address. This lets the code keep running at the physical address after MMU enable, until the jump to virtual. Real kernels do the same trick (Linux calls it the "identity map trampoline"), but they tear down the identity mapping afterward. I leave it around forever.

### What I need to learn to improve this

- **EL2/EL3 bring-up**: How ARM Trusted Firmware (ATF/TF-A) works, what PSCI is, how EL2 hands off to EL1. Understanding this means understanding why the kernel can't just assume "I'm at EL1" on real hardware.
- **Multi-core boot**: My kernel assumes one CPU. On real hardware, secondary cores are parked and need to be woken up via PSCI `CPU_ON`. The kernel needs per-CPU stacks and a spin-lock barrier before secondary cores can proceed.
- **Tearing down the identity map**: After jumping to virtual addresses, the physical-address mapping in TTBR0/TTBR1 should be removed. Leaving it means kernel code is accidentally accessible at physical addresses too, which is a security hole.

---

## 2. MMU & Page Tables

### What I learned

- AArch64 uses a 4-level page table (L0 -> L1 -> L2 -> L3) with 4KB granule and 48-bit virtual addresses.
- TTBR1_EL1 handles the upper half (`0xFFFF...`) for kernel, TTBR0_EL1 handles the lower half (`0x0000...`) for user space. This split is automatic based on bit 55 of the VA.
- Each level covers a different address range: L0 entry covers 512GB, L1 covers 1GB, L2 covers 2MB, L3 covers 4KB.
- I calculate table indices by shifting and masking: `(va >> 39) & 0x1FF` for L0, `(va >> 30) & 0x1FF` for L1, etc. Each index picks one of 512 entries in that level's table.
- Page table entries have control bits: Valid, Table/Block, Access Flag, AP (access permissions), AttrIdx (memory type), PXN/UXN (execute permissions).
- `DSB` and `ISB` barriers are needed after writing system registers -- the CPU pipelines these and without barriers the new values might not take effect before the next instruction.
- I need an identity mapping (physical == virtual) that survives the moment I flip the MMU on, because the instruction *after* the `msr SCTLR_EL1` is fetched using the new translation.

### What I assumed

**"Page tables can live at hardcoded physical addresses."**
I placed tables at fixed addresses: L0 at 0x80020000, L1 at 0x80040000, L2 at 0x80060000, etc. I picked these by counting bytes past the kernel image and hoping nothing overlaps. This works because my kernel is small and I know the layout. A real kernel uses a page allocator to get free physical frames for page tables, so they can be placed anywhere and any number of them can exist.

**"I only need a few L2 block mappings."**
I map the kernel, UART, GIC, DTB, and stack as 2MB blocks at L2. That's it. If I need to map something new (a new device, more RAM), I add another hardcoded L2 entry. A real kernel has a general-purpose `ioremap()` function that takes any physical address + size and creates the mapping dynamically.

**"User page tables can be at fixed addresses (0x800F0000, 0x80150000)."**
I hardcoded per-process page table bases and compute sub-table locations as fixed offsets (`l1 = l0 + 0x10000`). This means I can only ever have 2 processes, and each process only gets one L3 table (512 entries = 2MB of mappable user space). A real kernel allocates page table frames from the page allocator and chains them dynamically, allowing unlimited processes with unlimited address space.

**"Virtual == Physical for user space (identity mapping)."**
When I map user pages, the virtual address in user space equals the physical address. I do `kernal_to_user_space()` which just strips the upper `0xFFFF` bits. So user VA `0x0C001000` maps to physical `0x0C001000`. A real kernel gives each process its own virtual address layout starting from a fixed base (like `0x400000` for code, growing-up heap, growing-down stack), and the physical backing can be anywhere. Two processes can both have virtual address `0x400000` mapping to completely different physical frames.

**"AttrIdx = 0 for everything is fine."**
All my page descriptors leave bits [4:2] as zero, which selects MAIR Attr0 = `0x00` (Device-nGnRnE memory). This means every mapped page -- kernel code, user code, stacks, heap -- is treated as device memory: no caching, no speculation, strict ordering. It works because QEMU emulates memory access correctly regardless of caching attributes, but on real hardware this would be extremely slow (every load/store goes to main memory) and some CPUs won't even fetch instructions from device memory. Normal memory should use AttrIdx=1 (`0xFF`, cacheable).

**"I don't need TLB invalidation after changing page tables."**
The TLB invalidation in `set_table_entry` is commented out. The CPU caches page table walks in the TLB. If I change a page table entry without invalidating, the CPU might still use the old cached translation. This works in my current setup because I set up all mappings before they're first used (so no stale entries exist), and context switches write TTBR0 which implicitly flushes some TLB state. But for dynamic mapping changes (page faults, munmap, fork), TLB invalidation is mandatory.

### What I need to learn to improve this

- **Page frame allocator**: How to track which physical frames are free. The common approach is a "buddy allocator" -- it groups pages in power-of-2 sizes and can merge adjacent free blocks. Linux uses this (`alloc_pages()`). Understanding this unlocks dynamic page table allocation.
- **MAIR and memory types**: Which MAIR index to use for what. Normal cacheable for RAM (code, data, stacks), Device-nGnRnE for MMIO registers (UART, GIC), possibly normal non-cacheable for DMA buffers (virtio). The descriptor's AttrIdx field selects which MAIR slot applies.
- **TLB management**: When to invalidate (`TLBI`), which scope to use (`vmalle1` = all EL1 entries, `vale1` = by VA, `aside1` = by ASID). Understanding ASIDs (Address Space IDs) -- they let the TLB hold entries from multiple processes without flushing on every context switch.
- **Kernel virtual address allocator**: How `vmalloc` / `ioremap` work. The kernel needs a way to pick unused virtual ranges and create mappings for them. This is separate from the physical frame allocator.
- **Copy-on-write**: For `fork()`, instead of copying all pages, mark them read-only in both parent and child. When either writes, the permission fault triggers a copy of just that one page. Requires a working page fault handler.

---

## 3. Memory Allocator (kmalloc)

### What I learned

- Physical memory is divided into 4KB pages. A bitmap can track which pages are allocated: 1 bit per page.
- To allocate N bytes, I round up to pages, find N free bits in the bitmap, mark them, convert the bit position to a physical address, create page table entries, and return the virtual address.
- The physical pages don't have to be at address 0 -- QEMU maps RAM starting at 0x80000000, so I had to offset my physical base accordingly. (This took a whole day to figure out -- physical 0x00000000 is device MMIO, not RAM.)

### What I assumed

**"One bitmap, linear scan, first-fit."**
`find_free()` scans from bit 0 every time, checking each bit one by one. For 4GB RAM that's potentially scanning 1M bits per allocation. This is O(n) per allocation. Real kernels use the buddy allocator which is O(log n): it maintains free lists grouped by block size (1 page, 2 pages, 4 pages... up to 2^MAX_ORDER pages). Allocation picks from the right size list, splitting larger blocks if needed.

**"kmalloc and page allocation are the same thing."**
My `kmalloc` always allocates full pages. A 16-byte struct gets a 4KB page. Real kernels have two layers: the page allocator for full pages, and a slab allocator (SLAB/SLUB) on top for small objects. SLUB divides a page into fixed-size slots (e.g., 32-byte, 64-byte, 128-byte) and hands out slots from the right slab. This avoids wasting ~4000 bytes per small allocation.

**"Allocating always-contiguous pages is fine."**
My bitmap allocates contiguous bit positions, meaning contiguous physical pages. This gets harder as memory fragments. The buddy allocator solves this by always allocating in power-of-2 aligned blocks and merging buddies on free. For non-contiguous cases, the kernel maps scattered physical pages into contiguous virtual addresses (that's what `vmalloc` does).

**"A stack-allocated BitIndex[64] is enough."**
`kmalloc` puts `BitIndex res[64]` on the kernel stack. This limits allocations to 64 pages (256KB) and could overflow the 4KB kernel stack for larger requests. A real allocator doesn't track individual bit positions this way -- the buddy system's free list structure inherently knows where free blocks are.

**"The bitmap buffer can come from a linker-defined block."**
`mem_block()` just returns `__block_memory_start` with no management. The linker reserves 512KB for this. Once the bitmap is placed there, remaining space is wasted. A real kernel bootstraps: first it uses a simple early allocator (like "memblock" in Linux) to get the page allocator going, then the page allocator manages everything.

### What I need to learn to improve this

- **Buddy allocator**: How power-of-2 free lists work, how splitting and merging maintains low fragmentation. Read Linux's `mm/page_alloc.c` or write a standalone buddy allocator.
- **Slab allocator (SLUB)**: How to sub-divide pages into object caches for `kmalloc(32)`, `kmalloc(64)`, etc. Key concept: a "slab" is a page (or group of pages) pre-divided into fixed-size slots, with a freelist threading through the free slots.
- **Early boot allocator**: How Linux's `memblock` works -- a simple array of (base, size) regions that serves allocations before the buddy allocator is ready. The bitmap approach I have is similar in spirit but cruder.

---

## 4. Scheduler & Context Switching

### What I learned

- A "context" is the full CPU register state of a process: x0-x30, SP_EL0 (user stack), ELR_EL1 (return address), SPSR_EL1 (processor state), and TTBR0_EL1 (page table pointer).
- Context switching means: save current registers to memory, load next task's registers from memory, ERET. The CPU doesn't know anything changed -- it just executes from wherever ELR_EL1 points with whatever SP says.
- Round-robin: each task gets N timer ticks. When the slice runs out, push current onto the back of the ready queue, pop the front, load its context.
- TTBR0_EL1 is part of the context. Switching it swaps the entire user address space. This is how two processes with the same virtual addresses (e.g., both have code at 0x1000) point to different physical memory.
- The timer interrupt fires, we save context on the kernel stack, call the C scheduler, get back a (possibly different) context pointer, restore from it, and ERET. The elegance is that save and restore are the same code path regardless of whether we actually switched.

### What I assumed

**"Only preempt from EL0."**
`irq_dispatcher` checks `SPSR_EL1[3:2]` to see if the interrupt came from EL0. If from EL1, skip scheduling. This means the kernel itself is non-preemptible. If a syscall takes a long time, the other process just waits. Real kernels (Linux with `PREEMPT=y`) can preempt even kernel code -- they track a "preemption count" and check it at specific safe points.

**"Two hardcoded tasks, compiled into the kernel."**
`user_process1` and `user_process2` are C functions in `kmain.c`. Their code lives in the kernel image. I strip the upper bits to make a "user" address and map that physical page into the process's TTBR0 table. A real kernel loads user binaries from the filesystem -- an ELF loader reads the binary, maps its segments into a fresh address space, and sets ELR_EL1 to the ELF entry point.

**"Fixed-size ready queue as a linked list through the Task struct."**
Each Task has a `next` pointer forming the ready queue. This is fine for 2 tasks but doesn't scale. Linux's CFS (Completely Fair Scheduler) uses a red-black tree keyed by "virtual runtime" -- tasks that have used less CPU time are picked first. This provides fairness without fixed time slices.

**"No blocked/sleeping state."**
My tasks are always READY or RUNNING. There's no way for a task to say "wake me up when this I/O completes." A real kernel has wait queues: a process blocks on a waitqueue, the interrupt handler for the I/O device wakes it by moving it back to the run queue. This is critical for I/O -- without it, processes must busy-wait.

**"Each task's context lives at the top of its kernel stack."**
I place the Context struct at `kstack + STACK_SZ - sizeof(Context)`. The save macro also uses SP for saving context. This works but means the kernel stack must be large enough for the context frame plus any function call depth. Real kernels (like Linux) store the `task_struct` and CPU context in a dedicated `thread_info` structure at the bottom or top of the kernel stack page, carefully sized.

### What I need to learn to improve this

- **Wait queues and blocking**: How a process voluntarily gives up the CPU (e.g., `wait_event()` in Linux). The scheduler marks it as BLOCKED, removes it from the run queue, and only re-enqueues it when the event fires. This is the foundation for all I/O.
- **Per-CPU run queues and load balancing**: On multi-core systems, each CPU has its own run queue. Tasks migrate between CPUs for balance. Understanding this means understanding why a single global `rq_head` doesn't scale.
- **Priority and fairness**: How CFS computes "virtual runtime" and why it produces fairness. Also, real-time scheduling classes (FIFO, round-robin) that have strict priority over normal tasks.
- **Kernel preemption**: How `preempt_count` works, what a "preemption point" is, and why some code paths (holding spinlocks) must not be preempted.

---

## 5. Exception Handling & Interrupts

### What I learned

- AArch64 has a fixed vector table layout: 4 groups of 4 vectors (sync, IRQ, FIQ, SError) x 4 exception sources (EL1t, EL1h, EL0_64, EL0_32) = 16 entries, each 128 bytes apart.
- ESR_EL1 (Exception Syndrome Register) tells you *why* the exception happened. Bits [31:26] are the exception class (EC): 0x15 = SVC (syscall), 0x20 = instruction abort from lower EL, 0x24 = data abort from lower EL.
- The GIC is a separate hardware block that mediates between device interrupt lines and the CPU core. GICD (distributor) manages which interrupts are enabled globally, GICC (CPU interface) manages per-CPU delivery.
- Interrupt flow: device asserts IRQ line -> GICD sees it -> GICC delivers to CPU -> CPU vectors to EL1 IRQ handler -> handler reads IAR to get interrupt ID -> handles it -> writes EOIR to acknowledge.
- The virtual timer (IRQ 27) is a per-CPU private peripheral interrupt (PPI). It counts down and fires when it reaches zero. I reprogram it each time to fire again.

### What I assumed

**"Only one interrupt source (timer) matters."**
The GIC only enables IRQ 27 (vtimer). Everything else is ignored. A real kernel enables many interrupt sources: UART RX (for console input), block device completion, network, etc. Each gets a handler registered via something like Linux's `request_irq()`.

**"I can handle everything in the IRQ handler directly."**
My `irq_dispatcher` does all work inline: acknowledges the timer, calls the scheduler, returns. For a timer tick this is fine (fast, bounded work). But for something like a network packet, the handler should do minimal work (acknowledge, copy data from device) and defer processing to a "bottom half" (softirq, tasklet, or workqueue in Linux terms). Long interrupt handlers block all other interrupts.

**"Synchronous exceptions from EL0 are just syscalls."**
`el0_sync_entry` checks for SVC and does `eret` for everything else. But EL0 can also generate data aborts (page fault when accessing unmapped memory), instruction aborts (jumping to unmapped address), alignment faults, and more. A real kernel dispatches on the EC field: SVC -> syscall handler, data abort -> page fault handler (which may allocate a page or kill the process), instruction abort -> segfault, etc.

**"No nested interrupts, no SError, no FIQ."**
FIQ vectors go to `default_entry: eret`. SError (asynchronous hardware errors) same. Nested interrupts (an IRQ during IRQ handling) aren't considered. Real kernels handle SError as a fatal machine check, optionally use FIQ for debugging (Linux NMI backtrace), and have careful interrupt masking to prevent re-entrance during critical sections.

### What I need to learn to improve this

- **Interrupt descriptor table / handler registration**: How Linux's `request_irq(irq_num, handler, flags, name, dev)` works. A table maps interrupt IDs to handler functions so each device's driver registers its own handler.
- **Top half / bottom half split**: IRQ handlers do minimal work (top half), then schedule deferred work (bottom half). `softirq` for high-frequency events (networking, block I/O), `tasklets` for simpler cases, `workqueues` for work that can sleep. Understanding why this split exists (keeping interrupt latency low).
- **Data abort handler / page fault path**: How to read FAR_EL1 (Fault Address Register) and ESR_EL1 to determine what went wrong, then dispatch: demand paging (allocate + map page), stack growth (extend stack VMA), copy-on-write (copy page and remap), or SIGSEGV (kill process).

---

## 6. Syscalls

### What I learned

- ARM64 syscall convention: user calls `SVC #0`, syscall number goes in x8, arguments in x0-x5, return value in x0.
- The SVC instruction synchronously traps to EL1. ESR_EL1 confirms EC = 0x15 (SVC).
- The kernel can read user registers from the saved context, do kernel work, put the result in x0 of the context, and ERET back.

### What I assumed (and what's currently broken)

**"I can call the C syscall handler without saving context."**
`el0_sync_entry` does `bl syscall_dispatcher` then `eret` without SAVE_CONTEXT/RESTORE_AND_ERET. This means: (a) the C function clobbers caller-saved registers (x0-x18) which are the user's live registers, (b) there's no way to modify x0 in the user's context to pass back the return value, (c) if a timer interrupt fires while inside the syscall, the scheduler has no context to save.

The assembly label `syscall_dispatcher:` at vectors.s:152 also shadows the C function, so the `bl` never actually reaches the C code anyway.

**"Stripping bits from x8 extracts the syscall number."**
The `BIC x8, x8, #0xffff` clears the lower 16 bits. BIC = Bit Clear. To extract, I should just use x8 directly (the syscall number is already there).

**In a real kernel:**
- Syscall entry saves full context (same as IRQ entry).
- A syscall table (array of function pointers) maps syscall numbers to handler functions: `syscall_table[nr](args...)`.
- `copy_from_user()` / `copy_to_user()` safely transfer data between kernel and user buffers, checking page permissions. You can't just dereference a user pointer from kernel mode -- the user might pass a kernel address to trick you into corrupting kernel memory.
- After the syscall, the kernel checks if it should reschedule (`need_resched` flag), handle pending signals, etc., before returning to user space.

### What I need to learn to improve this

- **Syscall table pattern**: Define `void *syscall_table[]` with one entry per syscall number. The entry stub extracts the number from context->x[8], bounds-checks it, calls `syscall_table[nr](context)`. This cleanly separates dispatch from implementation.
- **User pointer validation**: Why you can't just dereference user pointers. The user controls TTBR0 mappings -- they could map a kernel address into their space, pass it to `write()`, and trick the kernel into overwriting its own data. `copy_from_user` verifies the pointer is in the user range before accessing.
- **Signal delivery**: After every syscall (and every IRQ return to user), the kernel checks if signals are pending. If so, it modifies the context to redirect to the signal handler in user space, pushing the original context onto the user stack so it can be restored later (`sigreturn`).

---

## 7. EL0 / EL1 Separation

### What I learned

- EL0 (user mode) and EL1 (kernel mode) are hardware-enforced privilege levels. EL0 cannot execute MSR/MRS to system registers, cannot disable interrupts, cannot access kernel memory.
- TTBR0 gives EL0 its own address space. The kernel sets up TTBR0 per-process, so each process thinks it has the entire lower half of the address space to itself.
- Transition EL0 -> EL1 happens via exceptions (SVC, IRQ, abort). Transition EL1 -> EL0 happens via ERET with SPSR_EL1.M = 0 (EL0t).
- I set SPSR to `0b0000` (EL0t, all DAIF clear) in the initial context, so when I do `first_user_enter` -> RESTORE_AND_ERET, the CPU drops to EL0 and starts executing the user function.

### What I assumed

**"User processes can be C functions compiled into the kernel."**
I compile `user_process1()` as kernel code, then strip the `0xFFFF` prefix to get a "user" address, and map that physical page into the process's TTBR0. The function runs in EL0 but its code is physically in the kernel image. A real kernel loads separate ELF binaries from a filesystem. The user code is compiled independently, loaded into user-mapped pages, and has no relation to kernel symbols.

**"User and kernel share the same physical pages for code."**
Because user code is in the kernel image, the same physical page is mapped twice: once in TTBR1 (kernel) and once in TTBR0 (user). A real kernel never maps kernel code into user space. The separation is total: kernel pages are only in TTBR1, user pages only in TTBR0.

**"One page of code is enough for a user process."**
`map_user_to_physical(id, entry)` maps exactly one 4KB page for the process's code. If the function is larger than 4KB, the second page is unmapped -> instruction abort. The comment at kmain.c:100 already notes this. A real kernel maps all segments from the ELF file: .text (code), .data (initialized data), .bss (zeroed data), stack (grows down from a high address), heap (grows up via brk).

### What I need to learn to improve this

- **ELF loading**: The ELF binary format -- program headers define segments (PT_LOAD), each with a virtual address, file offset, and size. The kernel reads these, allocates pages, copies data from the file, and maps them at the specified virtual addresses.
- **User stack setup**: The initial stack for a process contains argc, argv pointers, environment pointers, and the auxiliary vector (AT_PHDR, AT_ENTRY, etc.). The kernel builds this before dropping to EL0. Understanding this ABI is needed for `execve`.
- **KPTI (Kernel Page Table Isolation)**: A security mitigation where user-mode TTBR1 is nearly empty (only the exception vector trampoline), so speculative execution from EL0 can't leak kernel memory via side channels. Understanding this means understanding Spectre/Meltdown.

---

## 8. Device Tree Parsing

### What I learned

- QEMU loads a Flattened Device Tree (FDT) blob into memory at 0x40000000 (for `virt` machine). It describes all hardware: RAM regions, interrupt controllers, UARTs, virtio devices, timers.
- FDT is a binary format with a header, a structure block (tree of nodes and properties), and a strings block (property names).
- Tokens: `BEGIN_NODE (0x1)` starts a node with a name, `PROP (0x3)` defines a property with length + string offset + value bytes, `END_NODE (0x2)` closes a node, `END (0x9)` terminates.
- Property values are big-endian. I use `read_b32()` to byte-swap them.
- The `memory@...` node's `reg` property gives the RAM base address and size. The `virtio_mmio@...` node gives the MMIO register base and interrupt number.

### What I assumed

**"I only need RAM size and one virtio device."**
I scan for a node starting with 'm' (memory) and a node starting with 'v' (virtio). I use character comparison (`name[0] == 'm' && name[5] == 'y'`) instead of proper string comparison. This is fragile -- any node starting with 'm' where the 6th character is 'y' would match. A real DTB parser uses `strcmp` on the full node name (or the `compatible` property for driver matching).

**"I can read the DTB at a hardcoded physical address before MMU, then at a hardcoded virtual address after."**
`get_total_ram()` reads from physical 0x40000000 (called before MMU). `get_drive_info()` reads from virtual 0xFFFF000085400000 (called after MMU). The virtual address is a manually chosen constant. A real kernel stores the DTB pointer from the bootloader (passed in x0 at entry on ARM64), maps it early, and uses it everywhere through that single pointer.

**"FDT parsing doesn't need bounds checking."**
I advance the pointer `p` through the structure block without checking if it goes past the end. A malformed DTB could cause me to read arbitrary memory. Real parsers (like `libfdt` used by Linux) validate offsets against the totalsize field.

### What I need to learn to improve this

- **libfdt / proper FDT library**: Using a real FDT library that handles all node types, string matching, #address-cells/#size-cells for proper `reg` property parsing. Understanding `#address-cells` and `#size-cells` is important -- they determine how many 32-bit values make up an address and a size in child nodes.
- **Device model / driver binding**: How Linux matches DTB `compatible` strings to drivers. Each driver declares what `compatible` strings it handles. The kernel walks the DTB, and for each node, finds the matching driver and calls its `probe()` function.

---

## 9. UART Driver

### What I learned

- PL011 UART is a standard ARM peripheral. It has a data register (write a byte to transmit), a flag register (check if TX buffer is full), and control registers for baud rate and format.
- Polling mode: spin-wait on the TX busy flag, then write. Simple and reliable for debug output.
- `uart_printf` is a minimal printf: parse the format string, for each `%` specifier, pull the next vararg and format it.

### What I assumed

**"Polling mode is sufficient."**
For debug output, yes. But polling TX means the CPU is wasted waiting for bytes to transmit. For real use, interrupt-driven UART: write to a buffer, enable TX interrupt, the ISR sends the next byte, acknowledges the interrupt. Also needed for RX -- I have no way to receive keyboard input right now.

**"uart_printf doesn't need %x (hex)."**
I have %d (signed int), %l (unsigned 64-bit), %s, %c. No hex output. When debugging kernel addresses and page table entries, hex is far more useful than decimal. Printing `0xFFFF000082400000` as `18446462598732840960` isn't helpful.

**"A 2048-byte stack buffer in uint2str is fine."**
`uint2str` and `int2str` allocate `char buf[2048]` on the stack. A uint64 is at most 20 decimal digits. This wastes ~2KB of kernel stack per printf call, and the kernel stack is only 4KB.

### What I need to learn to improve this

- **Interrupt-driven UART**: Enable RX interrupt in IMSC, add UART IRQ to GIC, write an ISR that reads received bytes into a ring buffer. This is the foundation for console input.
- **Ring buffers for I/O**: How a circular buffer with head/tail pointers provides lock-free single-producer/single-consumer communication between ISR (producer) and reader (consumer).

---

## 10. GIC (Interrupt Controller)

### What I learned

- GICv2 has two main blocks: Distributor (GICD, system-wide) and CPU Interface (GICC, per-CPU).
- GICD controls which interrupts are enabled globally. Writing a bit to ISENABLER0 enables that interrupt.
- GICC controls the local CPU's willingness to take interrupts. PMR (Priority Mask Register) sets a threshold -- only interrupts with priority higher (numerically lower) than the mask get through.
- IRQ acknowledgement flow: read IAR -> get interrupt ID -> handle -> write EOIR. Must write EOIR or the GIC won't deliver the next interrupt.
- IRQ 27 is the EL1 virtual timer. It's a PPI (Private Peripheral Interrupt), meaning each CPU has its own copy.

### What I assumed

**"Only one interrupt, one priority level, one CPU."**
I enable exactly one interrupt (timer) at one priority level, for one CPU. A real GIC setup: configures priority for each interrupt, sets target CPU affinity for SPIs (shared peripheral interrupts), configures edge vs level triggering, enables multiple interrupt sources across all CPUs.

**"PMR = 0xF0 is fine."**
Priority Mask 0xF0 means "accept interrupts with priority < 0xF0." Since I never set any interrupt's priority (default is 0x00 = highest), everything passes. But if I added interrupts with configured priorities, this mask would filter incorrectly. Understanding priority grouping and preemption of interrupts is important for a system with many interrupt sources.

### What I need to learn to improve this

- **GIC priority and preemption**: How interrupt priorities work, group 0 vs group 1, priority drop vs deactivation. Understanding this is needed for systems where some interrupts (like NMI or watchdog) must preempt others.
- **GICv3**: The modern interrupt controller used on newer ARM platforms. It replaces memory-mapped GICC with system register access (ICC_* registers), which is faster. Understanding the differences matters for real hardware.
- **Interrupt affinity**: How to route specific interrupts to specific CPUs for load distribution. Relevant for multi-core.

---

## 11. Virtio Block Device

### What I learned

- Virtio is a standardized interface for virtual devices. The "device" is emulated by QEMU, the "driver" runs in the guest kernel.
- MMIO transport: device registers are memory-mapped at addresses provided by the DTB. The driver reads/writes these registers for setup and notification.
- Virtqueue: a shared-memory ring buffer between guest and host. Three parts: descriptor table (buffers), available ring (guest -> host: "process these"), used ring (host -> guest: "I processed these").
- A block request needs 3 chained descriptors: request header (type + sector, read-only), data buffer (read or write), status byte (write by device).
- Feature negotiation: read device features, write back the subset you support, set FEATURES_OK, verify the device accepted.

### What I assumed (and what's broken)

**"The init sequence matches the spec."**
It doesn't. The code does: ACK -> DRIVER -> FEATURES_OK -> DRIVER_OK -> init_queue. But the spec says queues must be configured *between* FEATURES_OK and DRIVER_OK. Setting DRIVER_OK means "I'm done configuring," so the device may ignore queue setup done afterward.

**"Physical addresses for descriptors = virtual with upper bits stripped."**
I pass `kernal_to_user_space(ptr)` as the physical address for descriptor data. This only works because my kmalloc maps pages at phys 0xC0000000+ with identity mapping (VA lower bits == PA). But this is fragile and depends on the specific mapping layout. A real driver uses a proper `virt_to_phys()` function.

**"Polling the status byte is sufficient."**
After notifying, I spin-wait by polling the status byte. The virtio spec supports interrupt-driven completion via the used ring and an interrupt. Polling wastes CPU and has no timeout (infinite loop if the device never responds).

**"Setting avail.flags = 1 (VIRTQ_AVAIL_F_NO_INTERRUPT) is fine."**
This tells the host "don't interrupt me when you process entries." Fine for polling, but means I can never move to interrupt-driven I/O without changing this flag.

### What I need to learn to improve this

- **Virtio 1.1 spec compliance**: Read the spec carefully, especially section 3.1 (device initialization) and 2.6 (split virtqueues). The init order matters.
- **DMA and IOMMU**: The device accesses guest memory via DMA. The addresses in descriptors must be physical (guest-physical for a VM). Understanding DMA-safe memory allocation: the pages must be pinned, physically contiguous (or use scatter-gather), and mapped correctly.
- **Interrupt-driven I/O**: Configure the virtio interrupt in GIC, write a used-ring completion handler, wake blocked tasks waiting on I/O. This ties together GIC, scheduler wait queues, and virtio.

---

## 12. Things That Work But Are Fragile

### Context save/restore order

The SAVE_CONTEXT macro saves x0-x29 as pairs, then x30, then reads SP_EL0, ELR_EL1, SPSR_EL1, TTBR0_EL1. The RESTORE reverses this. The offsets must exactly match the Context C struct layout. If I ever add a field to Context without updating the asm offsets (CTX_SP_EL0, CTX_ELR, CTX_SPSR, CTX_TTBR0_EL1), context switching silently corrupts registers. Real kernels generate these offsets from the C struct using `asm-offsets.c` -- a build step that compiles a C file that emits `#define CTX_SP_EL0 248` (or whatever offsetof gives), so asm and C always agree.

### User process page table sub-tables at fixed offsets

Each user process gets sub-tables at +0x10000 increments from its L0 base: L1 at l0+0x10000, L2 at l0+0x20000, L3 at l0+0x30000. This means:
- Each process has exactly one L1, one L2, one L3 table.
- One L3 = 512 entries = 2MB of user virtual space.
- Two L2 entries in the same 1GB range would need two L3 tables, which isn't supported.
This works for tiny spin-loop processes. Anything with more than 2MB of code+data+stack would fault.

### Timer reprogramming

`set_cntv()` reprints the frequency every time it fires (`uart_printf("read back: %l\n", cntfrq)`). At ~10 Hz timer rate, this floods the UART with debug output. The timer works correctly, but the debug output makes it hard to see anything else.

---

## Summary: Learning Progression

| Concept | Status | Core Gap |
|---------|--------|----------|
| AArch64 boot sequence | Solid | EL2/EL3, multi-core |
| Page table structure & index calculation | Solid | - |
| MMU enable with identity map trampoline | Solid | Tear down identity map after |
| TTBR0/TTBR1 split for user/kernel | Solid | - |
| Memory attributes (MAIR, AttrIdx) | Understood theory, not applied | Set correct AttrIdx in descriptors |
| TLB invalidation | Understood theory, not applied | Uncomment and test |
| Physical page allocation | Working (bitmap) | Buddy allocator, slab allocator |
| Kernel virtual address allocation | Not started | vmalloc, ioremap |
| Timer interrupt -> scheduler | Solid | - |
| Context save/restore | Solid | asm-offsets for safety |
| Round-robin scheduling | Working | Wait queues, priorities, CFS |
| Per-process page tables | Working (2 processes) | Dynamic allocation, >2 processes |
| EL0/EL1 privilege separation | Working | Separate user binaries, ELF loading |
| Syscall entry/dispatch | Broken (asm label shadows C) | Fix, add context save, syscall table |
| Page fault handler | Not started | Demand paging, stack growth, CoW |
| GIC configuration | Working (timer only) | Multiple interrupts, priorities |
| UART output | Working | RX (input), interrupt-driven TX, %x format |
| DTB parsing | Working (fragile) | libfdt, #address-cells, driver binding |
| Virtio block device | Broken (init order) | Spec-compliant init, DMA, interrupt I/O |
| File system | Not started | Block layer, ext4/tmpfs, VFS |
| Process lifecycle (fork/exec/exit) | Not started | ELF loader, CoW, wait queues |
