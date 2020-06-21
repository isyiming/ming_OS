//
// task.c - Implements the functionality needed to multitask.
// Written for JamesM's kernel development tutorials.
//

#include "task.h"
#include "paging.h"

// The currently running task.
volatile task_t *current_task;

// The start of the task linked list.
volatile task_t *ready_queue;

// Some externs are needed to access members in paging.c...
extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;
extern void alloc_frame(page_t*,int,int);
extern u32int initial_esp;
extern u32int read_eip();

// The next available process ID.
u32int next_pid = 1;

void initialise_tasking()
{
	//关闭中断
	asm volatile("cli");

	// 将栈的移到512MB处
	move_stack((void*)0x20000000, 0x2000);

	// 初始化内核进程
	current_task = ready_queue = (task_t*)kmalloc(sizeof(task_t));
	current_task->id = next_pid++;
	current_task->esp = current_task->ebp = 0;
	current_task->eip = 0;
	current_task->page_directory = current_directory;
	current_task->next = 0;

	// 启用中断
	asm volatile("sti");
}

int fork()
{
   // 关闭中断
   asm volatile("cli");

   // Take a pointer to this process' task struct for later reference.
   task_t * parent_task = (task_t*)current_task;

   // Clone the address space.
   page_directory_t * directory = clone_directory(current_directory);
	 // Create a new process.
	task_t * new_task = (task_t*)kmalloc(sizeof(task_t));
	new_task->id = next_pid++;
	new_task->esp = new_task->ebp = 0;
	new_task->eip = 0;
	new_task->page_directory = directory;
	new_task->next = 0;

	// Add it to the end of the ready queue.
	// Find the end of the ready queue...
	task_t * tmp_task = (task_t*)ready_queue;
	while (tmp_task->next)
			tmp_task = tmp_task->next;
	// ...And extend it.
	tmp_task->next = new_task;

	// We could be the parent or the child here - check.
	if (current_task == parent_task)
	{
			// We are the parent, so set up the esp/ebp/eip for our child.
			u32int esp; asm volatile("mov %%esp, %0" : "=r"(esp));
			u32int ebp; asm volatile("mov %%ebp, %0" : "=r"(ebp));
			new_task->esp = esp;
			new_task->ebp = ebp;
			new_task->eip = eip;
			// All finished: Reenable interrupts.
			asm volatile("sti");
			return new_task->id;
	}
	else
	{
		// We are the child - by convention return 0.
		return 0;
	}
}

void switch_task()
{
   // If we haven't initialised tasking yet, just return.
   if (!current_task)
       return;

			 // Read esp, ebp now for saving later on.
u32int esp, ebp, eip;
asm volatile("mov %%esp, %0" : "=r"(esp));
asm volatile("mov %%ebp, %0" : "=r"(ebp));

// Read the instruction pointer. We do some cunning logic here:
// One of two things could have happened when this function exits -
// (a) We called the function and it returned the EIP as requested.
// (b) We have just switched tasks, and because the saved EIP is essentially
// the instruction after read_eip(), it will seem as if read_eip has just
// returned.
// In the second case we need to return immediately. To detect it we put a dummy
// value in EAX further down at the end of this function. As C returns values in EAX,
// it will look like the return value is this dummy value! (0x12345).
eip = read_eip();

// Have we just switched tasks?
if (eip == 0x12345)
		return;

		// No, we didn't switch tasks. Let's save some register values and switch.
current_task->eip = eip;
current_task->esp = esp;
current_task->ebp = ebp;

// Get the next task to run.
current_task = current_task->next;
// If we fell off the end of the linked list start again at the beginning.
if (!current_task) current_task = ready_queue;

esp = current_task->esp;
ebp = current_task->ebp;

// Here we:
// * Stop interrupts so we don't get interrupted.
// * Temporarily put the new EIP location in ECX.
// * Load the stack and base pointers from the new task struct.
// * Change page directory to the physical address (physicalAddr) of the new directory.
// * Put a dummy value (0x12345) in EAX so that above we can recognise that we've just
// switched task.
// * Restart interrupts. The STI instruction has a delay - it doesn't take effect until after
// the next instruction.
// * Jump to the location in ECX (remember we put the new EIP in there).
asm volatile("         \
	cli;                 \
	mov %0, %%ecx;       \
	mov %1, %%esp;       \
	mov %2, %%ebp;       \
	mov %3, %%cr3;       \
	mov $0x12345, %%eax; \
	sti;                 \
	jmp *%%ecx           "
						 : : "r"(eip), "r"(esp), "r"(ebp), "r"(current_directory->physicalAddr));
}
