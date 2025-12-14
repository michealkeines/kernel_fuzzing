now i have simple kernel that can switch between two user tasks based on the current round robin scheduling
it is driven by timer interrupts passing through gic, so both gic an interrupt handlerl ike el1_irq, el0_syn are defined and tested with syscall that reach el0_syn and timer interrupts that read el1_irq

now, this will be my base, i need to bring in MMU unit, a simple malloc and free, this is not about high implmentation, we are writing this learn how MMU is show showcase memmory + convert physical address into virtual adrress


PSTATE=600003c5 -ZC- EL1h    FPU disabled
Taking exception 4 [Data Abort] on CPU 0
...from EL1 to EL1
...with ESR 0x25/0x96000004
...with FAR 0xffff00000000
...with SPSR 0x600003c5
...with ELR 0xffff0000800001c4
...to EL1 PC 0x80001a00 PSTATE 0x3c5



Taking exception 4 [Data Abort] on CPU 0
...from EL1 to EL1
...with ESR 0x25/0x96000005
...with FAR 0xffff000000000000
...with SPSR 0x600003c5
...with ELR 0xffff0000800001c4
...to EL1 PC 0x80001a00 PSTATE 0x3c5







