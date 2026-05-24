#include <stdint.h>
#include <stddef.h>
#include "mmu.h"
#include "syscall.h"

#include "../drivers/block/block_driver.h"
#include "scheduler/context.h"
#include "scheduler/sched.h"
#include "../drivers/virtio_blk/virtio_blk.h"
extern void get_drive_info(uint64_t *result);
extern uint64_t get_total_ram(void);
extern void uart_init(void);
extern void uart_printf(const char* fmt, ...);
extern void *mem_block(uint64_t size);
extern void timer_init_1khz(void);
extern void gic_init(void);
extern void map_user_to_physical(uint64_t id, uint64_t address);
extern void map_user_to_physical_test(uint64_t id, uint64_t address);



#define STACK_SZ 4096

int64_t sys_write(uint64_t fd, char *buff, uint64_t size)
{
    int64_t result;
    asm volatile (
        "mov x8, #64 \t\n"
        "svc #0"
        : "=r" (result)
        : "r" (fd), "r" (buff), "r" (size)
        : "x8", "memory"
    );
    return result;
}

/*
TODO: 
	how can we map this function it the user virtual space and update all of it page tables
*/
void user_process1(void)
{

	/*
	
	TODO:

	- currenltly sys_write is not mapped in the user table,
	 we have to map that and try again, currenlut we just get table fault when we jump into the sys_write
	- but we have implemented sys_write, and called here
	*/
	// uart_printf("Task 1 is running\n");
	char test[5];
	int i = 0;
	test[i++] = 'A';
	test[i++] = 'A';
	test[i++] = 'A';
	test[i++] = 'A';
	test[i++] = 'A';
	int64_t result = sys_write(1, test, 512);

	// uart_printf("result of write: %l\n", result);

	i = 0;
	while (1) {
	if (i % 100 == 0) {

	// uart_printf("result of write: %l\n", result);
	}
		
		i++;
	
	};
}



// TODO:
// map all lib into the user process, find a proper way to do this, currenly only sys_write is mapped, we need to mapp lots of such libs

void user_process2(void)
{
	// uart_printf("Task 2 is running\n");
char test[5];
	int i = 4;

	test[i++] = 'C';
	test[i++] = 'C';
	test[i++] = 'C';
	test[i++] = 'C';
	test[i++] = 'C';
	int64_t result = sys_write(1, test, 512);
	while (1) {
		
		
	// uart_printf("Task 2 is running\n");

		volatile int i = 1;};
}






/*
eg: 0xffff000c000000
kernel_to_user_space = 0x000000000c000000

this address is managed by kmalloc using bitmap array

we need table for this address to the physcial

0x000000000c0000 => 0x00000000c0000




*/

Task *init_task(int id, const char *name, void (*entry)(void), uint64_t sz)
{

	// a helper that can map any lib into the task
	// and we want ot make sure all the refernces are linked
	// eg: if we use sys_write and if loadinto VA lets 0x800005000, but the instruction would have 0x0000000080001f38
	// page table entry within userspace VA
	// from  0x80001f38 (UVA) => 0x80001f38 (PA)
	// we have table
	// sys_write: 0x8000001, physical: 0x80001f38
	// 0x8000001 => 0x80001f38
	map_user_to_physical(id, (uint64_t)sys_write);
	map_user_to_physical(id, (uint64_t)uart_printf);

	Task *task = (Task *)kmalloc(sizeof(Task));
	uart_printf("Task: %l\n", task);
	
	uint8_t *kstack = (uint8_t *)kmalloc(sz);
	uart_printf("kstack: %l\n", kstack);
	uint64_t ustack_t = kmalloc(sz);
	uint8_t *ustack = (uint8_t *)kernal_to_user_space(ustack_t);
	// i am inside el1, i have current table that has identy mapping or table for 0xffff0000Csomething -> 0x000000000Csomthing
	// my kmalloc gives me 0xfffff0000Csomething as address
	// i am tryibng to convert that into my user space by zeoing the upper bits
	// 0x00000000Csomething
	// i am trying to mapping thins 0x0000000csomthing (virtual within my userspace) -> 0x0000000Csomething (physical range)
	map_user_to_physical_test(id, (uint64_t)(ustack));

	ustack = (uint8_t *)(((uint64_t)ustack) + sz);
	uart_printf("ustack: %l\n", ustack);

	task->id = id;
	task->name = name;
	task->kernel_stack = kstack;
	task->user_stack = ustack;

	// for (uint64_t i=0; i<3; ++i)
	// {
	// 	task->fds[i] = (File *)kmalloc(&GlobalBitMapArray, sizeof(File));
	// 	task->fds[i]->fd = i;

	// 	initilize_file(i);
	// }

	uint8_t *ktop = (uint8_t *)kstack + sz;
	Context *ctx = (Context *)(ktop - sizeof(Context));
	entry = (void*)kernal_to_user_space((uint64_t)entry);

	uart_printf("id: %d => entry: %l\n", id, (uint64_t)entry);

	map_user_to_physical(id, (uint64_t)entry); // if the function is bigger the 4kb, we might have to update the entry + 1 etc, because we map one page from that starting address
	build_initial_context(ctx, entry, ustack);
	ctx->ttbr0_el1 = (id == 1) ? USER_PROCESS_1_TABLE_START : USER_PROCESS_2_TABLE_START; // TODO: fix this to make it dynamic in future
	uart_printf("context %l, elr_el1: %l\n",ctx, ctx->elr_el1);


	// add kernel entries for 0x80000000 to 0x1000 * 40
	// uart_printf("after this");
	// for (int i = 0; i < 100; ++i)
	// {
	// 	uint64_t cu = 0x80000000 + (0x1000 * i);
	// 	map_user_to_physical(id, cu);
	// }
	/*
	
	

    |          |
	|          |
	|          | - 0x70 (SP)
	|          |
	|          | - 0x50 (after decrement SP)
	
	*/
	task->context = ctx;

	uart_printf("init: task->context %l, task: %l\n",task->context, task);

	sched_ready_enqueue(task);

	return task;
}


uint8_t *init_allocation_map(uint64_t total_physical_ram)
{

    // uart_printf("test: %l\n", (uint64_t)__block_memory_start);
    uart_printf("Total RAM: %l\n", total_physical_ram);
    uint64_t bitmap_size = (total_physical_ram / 4096) / 8;
    uart_printf("Total BITMAP Size: %l\n", bitmap_size);
    uint8_t *bitmap = (uint8_t*)mem_block(bitmap_size);
    uart_printf("bitmap starting address: %l\n", bitmap);

    if (bitmap == 0) uart_printf("Allocation failed: %d\n", bitmap_size);
    return bitmap;
}


int32_t testme(void)

{
	return 1 + 1;
}





void kmain(uint64_t total_ram)
{
    // uint64_t *val= (uint64_t *)0x80A00000;
    uart_init();
	// char val = ((char*)(0xffff01f40000))[0];
    uart_printf("Kernel Starting\n");

    uart_printf("Kernel Inited\n");

	gic_init();
	uart_printf("Timers are started1\n");
	timer_init_1khz();
	uart_printf("Timers are started2\n");


	uart_printf("Timers are started\n");


    // total_ram = 4294967296;
    uint8_t *bitmap = init_allocation_map(total_ram);
	init_bitmap(bitmap);

	uint64_t si = 16000;
	uint64_t test = kmalloc( si);
	uart_printf("pointer: %l\n", test);
	// ((char*)(0xffff000001f40000))[0] = 'a';
	// uart_printf("name1: %d, %c\n", test, ((char*)(0xffff000001f40000))[0]);
	// int32_t a = testme();
	((char*)(test))[0] = 'f';
	((char*)(test))[1] = 'o';
	((char*)(test))[2] = 'r';
	((char*)(test))[3] = 'k';
	((char*)(test))[13000] = 'f';
	((char*)(test))[4] = '\0';
	
	uart_printf("name1: %l, %d\n", test, ((char*)test)[0]);
	uart_printf("name2: %l, %d\n", test, ((char*)test)[13000]);
	uart_printf("name: %s\n", ((char*)test));

	uint64_t *drive_info = (uint64_t *)kmalloc( sizeof(uint64_t) * 6);
	get_drive_info(drive_info);
	for (int i = 0; i < 5; ++i) {
		uart_printf("result: %l\n", drive_info[i]);
	}


	// TODO:
	// - prepare the request to write into sector 100
	// - put that into queue and notify the registers


	// figure out why this layout is not working, looks like we are not writting the poper flags


	// we initaize the entire virtio register and backend address
	init_virtio_backend(drive_info[0]);

	// TODO:

	// - test with read examples, read/write works now
	// - group both read/write into high level api
	// - try batch request - multiple requests are working now
	char *data = (char *)kmalloc( sizeof(512));

	data[1] = 'Y';
	data[2] = 'Y';
	data[3] = 'Y';
	data[4] = 'Y';
	data[5] = 'Y';
	data[6] = 'Y';
	int i =0;
	uint64_t read = block_write(1000, data, 512);
	read = block_read(1000, data, 512);
	/*
	TODO:

	currently we can only make two request, first write and then a read, as all teh index within avail and used block are static, 1,2,3 and the 4,5,6 with index as 0 and 1


	make this process dynamic so we can read and writte wihtout having to udpate index, it should be done automattically



	this is done


	after a request is done, we need to set that index as freed? do this first - Tested but not in parallel case, for future

	now lets think about writing one mutex kinda data strucuture, so the high operations can be synced ebtween two processses as they all gonna be raccing eventruallly


	todo:

	- iplement spinlock with the asm ldxr/stxr
	- add some test with mutlipel users tryint to read block


	do this, ebfore we get into the VFS implementaion

	we need to test parallel use of block read/write from user space, currentluy we only tested is sequentially




	*/


	for (i = 0; i < 512; i++) {

	uart_printf("%d", data[i]);
	}
	// what do we need here?

	// we have written three descriptors, they are chained together
	// we have athe registers updated correctly
	// do we have the descriptor in the memory aligned properly?

	// we have added memory barier, and we update the flag to 1 in the avail ring, now we see the qemu logs with status, but nothing much changes

	// maybe we need to think about the mode, because the specification says that we should only have read-only or write-only permsiions, and i dont have that

	__asm__ volatile("msr daifset, #0xf");

	// while (1){}
	
	// /* 
	// TODO:
	// 	user process -> alloc_mem(300);
	// 	syccall -> syscall(5, 300)
	// 	we store the context
	// 	switch to EL1
	// 	we find the handler -> syscall handler
	// 	syscall_alloc_mem(bytes)
	// 	kmalloc(bytes) -> returns virtual address
	// 	restore the context
	// 	user process will get the virtual memory


	// 	we have inited the tasks
	// 	we have started the schulder code

	// 	we have to setup the vector table with the schedule on tick
	// 	we have to setup the vector table with user space interupts, we need a handler that will check the syscall code

	// 	irq_dispatcher has to check the code of current irq and call the repective handler of it, eg: if it 27 we call schedule on tick

	// 	irq_sync has to check if the code is as syscall, if it as call, then i twill be using SVC in arm, we ahve to call the sycall handler

	// 	we have the code for irq and sycall dispatcher
	// 	and also we finished vector setup


	// 	we have fixed the setup, so things are currently grouped and working just fine
	// */


	// File *user_process_file_1 = kopen("/root/test1");

	Task *task1 = init_task(1, "user_process1", user_process1, (uint64_t)STACK_SZ);
	uart_printf("started 1: task->context %l, task: %l\n",task1->context, task1);
	Task *task2 = init_task(2, "user_process2", user_process2, (uint64_t)STACK_SZ);
	uart_printf("started 2-1: task->context %l, task: %l\n",task1->context, task1);
	uart_printf("started 2-2: task->context %l, task: %l\n",task2->context, task2);
	
	uart_printf("Inited Two Tasks\n");
    
	sched_set_current(task1);
	uart_printf("Task 1 is scheduled\n");




	__asm__ volatile("msr DAIFCLr, #2");

	uart_printf("Jumping into Task1 %l, %l\n", task1, task1->context);
	first_user_enter(task1->context);
	// int32_t a = testme();



	// TODO:
	// check why our process are not swtich after the task1's time slice is over
	// check why the page table entry for GIC is not workings

	// mapping of L3 entries is working
	// think about why we fixed the L3 entry, maybe we want to add GIC physical map?
	// GIC mapping is also working, we updated the page entry for it and also fixed the virtual address inside the file

	// after the irq got handled, the next instruction doest seem to have the page table entry
	// check if the table register needs to be saved and put back into place?

	// IRQ to kernel code switch is forked

	// how come there is no cycle just after we return from the timer interrupt

	/*
		TIMER_INTERRUPT - 10 ms
		handle it       - 14 ms
		gotten back to kernel code - 17ms
		we sould be runing next instructions for 3ms
		TIMER_INTERRUPT - 20 ms

		if i disable the timmer, the timer just stops firing after the first time.

		interrupt is fired ,fi the value was zero

		and after the first interrupt, we never updated teh vlaue, that is why we forked here

		now the value is updated whenever there is a timer interrrupt and now we are able to switch between the kernel and the interrupt handler properly

		when we get into the user code, we are using a stack pointer that is as kernel address ie 0xffff

		- we have to check if we can add a separate page table for every process


		Design:

		- we need a physical location to store the User page table, that is of 512 * 4 size and we have two user process - maybe we hardcode this address 
		- we need to map the user space into some physical address, and we need keep track of that address - maybe we just reuse kmalloc here


		- we have updated all the places with the table register to save and restore
		- we have added some helpers to convert the user maps
		- now it compiles
		- we have to make it work next week

		now we somehow fixed the struct manual indexing issue, ie our index wihtin the struct was not correct, we updated that

		now, if we think about all things that are need to completely switch from one process to another, SP pointers dont magically get switched, we need to store and restore this tooo



		so SP is just an alias to SP_ELx

		and based on the EL, this alias will be pointing to the correct physical register

		this is theo nly thing we learned today

		- save both kernel and user stack and pop the into SP_EL0 and SP_EL1
		- then, confirm if we switch from EL1 into EL0, the alias of SP is pointer to hte EL0 correctly, or do we have to update teh state pointer (PSTATE.SPSel) before eret


		we are inside EL1, i am updating my SP_EL0, but when the next instruction tried to access SP, this had the new SP_EL0 value.

		the variables are, we enable PSTATE.SPSel=1 

		we have stack pointer 0xc0006000, 

		we have page table netry for 0xc0006000 + 4kb

		but the stack grows below the 0xc0006000

		ie: 0xc0005ff0, so we get the fault


		1484680-...with ESR 0x24/0x9200004f
		1484681:...with FAR 0xc0006ff0

		so the page table entry is there, but we get a permission fault

		- what excatly should i update? 
		
		is it i my page block descriptor that has some bit to updated?

		is it i my TCR that need update to add this extra permission

		or is my question completly wrong

		so the questions were correct, and we have update the page block descriptor with proper access permsission bit, and the permsion bit was in 7:6, and the value hsould rw for any exception level, so we just 6th bit to 1
		
		https://developer.arm.com/documentation/ddi0406/c/System-Level-Architecture/Virtual-Memory-System-Architecture--VMSA-/Memory-access-control/Access-permissions?lang=en


		right now the user level table is working

		but the momenet, we get an timer interrupt, we have switch back to the EL1

		in EL1, we are using TBR0 register, at this point of execution, this TBR0 is pointing ot the userspace table, so it wont have the table entries for executing anything in our vector table

		- why is that whene we enter into the EL1, we are not inside 0xffff


		we have a page table entry,

		but it doest have proper execution permission in EL1

		kernel's virtual 0x80000000 => physical 0x80000000


		final thoughts, TODO for next week

		we are executing in EL0

		we get a timmer interrtupt, we switch to EL1

		the first instruction here, which is just the vector for el1_irq_entry is getting fault

		and the fault is 0xf, ie permission fault at L3

		we tried to update the PXN, UXN for the L3 entries, which is always zero - nothing changed here

		we tried to index the proper attr from MIR register in pagbe entry - which is currently set to 0, so we are indexing 0th which is pointing to device memory
		not sure if this has any thing to do with our error, but it is possible that we are ducking somthign here

		in general, we have to figure out how to give execution permission for this page entriess

		my entrie kernel is in range 0x8000000

		when mmu is started, if i want to access this range, i need table entrie in TBR0 - ie the upper are zero, this table will be used for translation

		when jump into user process, i have uupdate TBR0 to an user specific table, this table doest have entries for that kernel range

		when the interrupt occurs, we get into EL1, the interrupt handler code lies withing 0x80000

		so it will automatically try to use the TBR0, which doest have entries for 0x8000

		i was trying just add those entries, which is really bad and still failed with some permission issues

		someone pointed that i could just update my VBAR to point to 0xffff, so when i get an interrupt, i wont need that TBR0 at all


		after update the vbar to address 0xffff, thihgns are working fine

		we are switch betweeen userprocess1 and timer interrupt

		we should at somepoint run out of timeslice and swtich to process2


		currenlty we are able to switche between tasks and interrupts

		but the return address for one of task is not pointing to 0x0, andlook like the context is not passed correclty to the process1, lets ereview this

		we have schedule on tick that run on every timer interrupt

		we have to set some process as the current process
		we enable interrupts

		at this point, we havent yet jumped into the process, so the schdule on tick would update the current context with kmain context


		we can check if the previous execution was EL1 or EL0

		if it is EL0 - we can run the schulde on tick, if not we shouldnt


		now we check the previous EL state before runing the schduler

		but is this valid for cases where a syscall a happens and we came from EL0 to EL1 and at this moment the timer inrrutp occurs

		TODO:

		- check if this scenario comes up in linux, if yes how it is handles
		- start implement the syscall for printing from the user space


		- fix the sys handler to read the input syscall number from w8
		- fix the return value in x0
		- add the sycall for write, read, close, exec, and few more important syscalls
		- add userspace mapp for syscall 5, mapp the virtual pages into the userprocess
		- read 63
		- write 64
		- exit 93
		- close 57
		- execve 221
		- mmap 222
		- brk 214



		- we need a file system, let it be a temp ram baseed file system
		- initd that the kernel should jump into, ie a bash shell with cd, ls, exec
		- we can put the actual user process inside this file system and run it iin using the shell 


		- we managed to read the DTD for the driver info, we go the register and the intruppt no 

		- write the block layer, virtio
	*/



    while (1) {
		
	// uart_printf("kerel runnign\n");
		int i = 1;
	}
}













/*

// uint64_t allocated1 = allocate_mem(&mainGlobalBitmapArray, 19900, res);
	// print_bit_index(res, allocated1);

	// free_pages(&mainGlobalBitmapArray, res, allocated1);
	// allocated1 = allocate_mem(&mainGlobalBitmapArray, 19900, res);
	// print_bit_index(res, allocated1);
	// return 0;
	// allocated1 = allocate_mem(&mainGlobalBitmapArray, 200, res);
	// print_bit_index(res, allocated1);
	// allocated1 = allocate_mem(&mainGlobalBitmapArray, 200, res);
	// print_bit_index(res, allocated1);
	// allocated1 = allocate_mem(&mainGlobalBitmapArray, 200, res);
	// print_bit_index(res, allocated1);
	// allocated1 = allocate_mem(&mainGlobalBitmapArray, 200, res);
	// print_bit_index(res, allocated1);
	// allocated1 = allocate_mem(&mainGlobalBitmapArray, 200, res);
	// print_bit_index(res, allocated1);
	// allocated1 = allocate_mem(&mainGlobalBitmapArray, 200, res);
	// print_bit_index(res, allocated1);
	// allocated1 = allocate_mem(&mainGlobalBitmapArray, 200, res);
	// print_bit_index(res, allocated1);
	// allocated1 = allocate_mem(&mainGlobalBitmapArray, 200, res);
	// print_bit_index(res, allocated1);
	// allocated1 = allocate_mem(&mainGlobalBitmapArray, 200, res);
	// print_bit_index(res, allocated1);


*/