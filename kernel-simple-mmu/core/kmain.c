#include <stdint.h>
#include <stddef.h>
#include "mmu.h"
#include "scheduler/context.h"
#include "scheduler/sched.h"

extern uint64_t get_total_ram(void);
extern void uart_init(void);
extern void uart_printf(const char* fmt, ...);
extern void *mem_block(uint64_t size);
extern void timer_init_1khz(void);
extern void gic_init(void);
extern uint64_t kmalloc(AllocateMem *mem, uint64_t size);



AllocateMem GlobalBitMapArray = {0}; // this should in .Data segment

#define STACK_SZ 4096

/*
TODO: 
	how can we map this function it the user virtual space and update all of it page tables
*/
void user_process1(void)
{
	uart_printf("Task 1 is running\n");
	while (1) {volatile int i = 1;};
}

void user_process2(void)
{
	while (1) {volatile int i = 1;};
}

Task *init_task(int id, const char *name, void (*entry)(void), uint64_t sz)
{

	/*
	TODO:
		we need user specific page table and store that into TTBR0_EL1
		
		when we switch context, we update this register with the respective page table
	*/
	Task *task = (Task *)kmalloc(&GlobalBitMapArray, sizeof(Task));
	uart_printf("Task: %l\n", task);
	
	uint8_t *kstack = (uint8_t *)kmalloc(&GlobalBitMapArray, sz);
	uart_printf("kstack: %l\n", kstack);
	uint8_t *ustack = (uint8_t *)kmalloc(&GlobalBitMapArray, sz);
	uart_printf("ustack: %l\n", ustack);

	task->id = id;
	task->name = name;
	task->kernel_stack = kstack;
	task->user_stack = ustack;

	uint8_t *ktop = (uint8_t *)kstack + sz;
	Context *ctx = (Context *)(ktop - sizeof(Context));

	build_initial_context(ctx, entry, ustack);
	uart_printf("context: %l\n", ctx);

	task->context = ctx;

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


	GlobalBitMapArray.buffer = bitmap;
	uint64_t si = 16000;
	uart_printf("allocating mem: %l\n", bitmap);
	uint64_t test = kmalloc(&GlobalBitMapArray, si);
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

	Task *task1 = init_task(1, "user_process1", user_process1, (uint64_t)STACK_SZ);
	Task *task2 = init_task(2, "user_process2", user_process2, (uint64_t)STACK_SZ);

	uart_printf("Inited Two Tasks\n");
    
	sched_set_current(task1);
	uart_printf("Task 1 is scheduled\n");

	__asm__ volatile("msr DAIFCLr, #2");

	uart_printf("Jumping into Task1\n");
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