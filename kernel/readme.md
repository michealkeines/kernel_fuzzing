

Core
    Exception handling - we have the sync handler that will do syscall dispatching, we have this timer iterrupts, and scheduling
    Context switch - we have round-robin scheduler that can swithc el0 proces based on timeslices 
    EL1→EL0 switch - we have swithcing working between el0 and el1
    Page tables per process - we havve helper that can create user page table and we use that witch between process and have the virtual separation

Memory
    Page allocator - we have page allocator and userace to page map helper
    Page fault handler - we need an hanlder the decide whether to allocate or say this is wrong and kill the process
    brk - to implemene a user malloc, we need brk
    mmap - optional

Process
    fork/clone
        - it has copy data from ELF into a new address space
        - duplicate exifint file descriptors
        - create a new Task struct
    execve
        fork()
            ↓
            child
            ↓
            execve("/bin/ls")
            ↓
            load ELF
            map segments
            setup stack
            jump to entry
    exit
        Free memory
        Close files
        Notify parent
        Mark as zombie

    wait
        You get zombies.
        Shell cannot know when command finished.

Files
    open
    read
    write
    close
    getdents


## Flow

ls

syscall read is called to read the input from stdin into a buffer

fork - this will alos new table, new task struct

execev - will load the elf, all memory erlate the porcess and run it

during exection, the process might more syscall, read, write

exit - xith process

wait - the shell is waiting for the child process to finish

this is the flow that we are gonna finish