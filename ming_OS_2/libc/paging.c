#include "../libc/paging.h"



// A bitset of frames - used or free.
uint32_t *frames;
uint32_t nframes;

// Defined in kheap.c
extern uint32_t free_mem_addr;

/* The kernel's page directory */
struct page_directory_t *kernel_directory;//内核页目录
/* The current page directory */
struct page_directory_t *current_directory;//当前进程页目录

// Macros used in the bitset algorithms.
#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

// Static function to set a bit in the frames bitset
static void set_frame(uint32_t frame_addr)
{
   uint32_t frame = frame_addr/PMM_PAGE_SIZE;
   uint32_t idx = INDEX_FROM_BIT(frame);
   uint32_t off = OFFSET_FROM_BIT(frame);
   frames[idx] |= (0x1 << off);
}

// Static function to clear a bit in the frames bitset
static void clear_frame(uint32_t frame_addr)
{
   uint32_t frame = frame_addr/PMM_PAGE_SIZE;
   uint32_t idx = INDEX_FROM_BIT(frame);
   uint32_t off = OFFSET_FROM_BIT(frame);
   frames[idx] &= ~(0x1 << off);
}

// Static function to test if a bit is set.
static uint32_t test_frame(uint32_t frame_addr)
{
   uint32_t frame = frame_addr/PMM_PAGE_SIZE;
   uint32_t idx = INDEX_FROM_BIT(frame);
   uint32_t off = OFFSET_FROM_BIT(frame);
   return (frames[idx] & (0x1 << off));
}

// Static function to find the first free frame.
static uint32_t first_frame()
{
   uint32_t i, j;
   for (i = 0; i < INDEX_FROM_BIT(nframes); i++)
   {
       if (frames[i] != 0xFFFFFFFF) // nothing free, exit early.
       {
           // at least one bit is free here.
           for (j = 0; j < 32; j++)
           {
               uint32_t toTest = 0x1 << j;
               if ( !(frames[i]&toTest) )
               {
                   return i*4*8+j;
               }
           }
       }
   }
}

////////////////////////////////////////
// Function to allocate a frame.
void alloc_frame(page_t *page, int is_kernel, int is_writeable)
{
   if (page->frame != 0)
   {
       return; // Frame was already allocated, return straight away.
   }
   else
   {
       uint32_t idx = first_frame(); // idx is now the index of the first free frame.
       if (idx == (uint32_t)-1)
       {
           // PANIC is just a macro that prints a message to the screen then hits an infinite loop.
					 // PANIC("No free frames!");
					 kprint("No free frames!");
       }
       set_frame(idx*PMM_PAGE_SIZE); // this frame is now ours!
       page->present = 1; // Mark it as present.
       page->rw = (is_writeable)?1:0; // Should the page be writeable?
       page->user = (is_kernel)?0:1; // Should the page be user-mode?
       page->frame = idx;
   }
}

// Function to deallocate a frame.
void free_frame(page_t *page)
{
   uint32_t frame;
   if (!(frame=page->frame))
   {
       return; // The given page didn't actually have an allocated frame!
   }
   else
   {
       clear_frame(frame); // Frame is now free again.
       page->frame = 0x0; // Page now doesn't have a frame.
   }
}
//以上是页表操作的处理函数
///////////////////////////////////////////////////////////

void page_fault(registers_t regs)
{
   // A page fault has occurred.
   // The faulting address is stored in the CR2 register.
   uint32_t faulting_address;
   asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

   // The error code gives us details of what happened.
   int present   = !(regs.err_code & 0x1); // Page not present
   int rw = regs.err_code & 0x2;           // Write operation?
   int us = regs.err_code & 0x4;           // Processor was in user-mode?
   int reserved = regs.err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
   int id = regs.err_code & 0x10;          // Caused by an instruction fetch?

   // Output an error message.
   kprint("Page fault! ( ");
   if (present) {kprint("present ");}
   if (rw) {kprint("read-only ");}
   if (us) {kprint("user-mode ");}
   if (reserved) {kprint("reserved ");}
   kprint(") at 0x");
   kprint(faulting_address);
   kprint("\n");
   // PANIC("Page fault");
	 kprint("Page fault!");

}


void initialise_paging()
{
   // The size of physical memory.
   uint32_t mem_end_page = PMM_END_ADDR;
   nframes = mem_end_page / PMM_PAGE_SIZE;
   frames = (uint32_t*)kmalloc(INDEX_FROM_BIT(nframes));
   memory_set(frames, 0, INDEX_FROM_BIT(nframes));

   // Let's make a page directory.
   kernel_directory = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));//为页目录分配空间
   memory_set(kernel_directory, 0, sizeof(page_directory_t));
   current_directory = kernel_directory;

   // We need to identity map (phys addr = virt addr) from
   // 0x0 to the end of used memory, so we can access this
   // transparently, as if paging wasn't enabled.
   // NOTE that we use a while loop here deliberately.
   // inside the loop body we actually change free_mem_addr
   // by calling kmalloc(). A while loop causes this to be
   // computed on-the-fly rather than once at the start.

   int i = 0;
   while (i < PMM_END_ADDR)//这里一定要用PMM_END_ADDR，PMM_END_ADDR是我们设定的mem_end_page的大小。对kernel_directory进行初始化一定要将全部页面初始化。否则系统不断重启。
   {
       // Kernel code is readable but not writeable from userspace.
       alloc_frame( get_page(i, 1, kernel_directory), 0, 0);
       i += PMM_PAGE_SIZE;
   }
   // Before we enable paging, we must register our page fault handler.
   register_interrupt_handler(IRQ14, page_fault);

   // Now, enable paging!
   switch_page_directory(kernel_directory);

	 // kprint("test: "); kprint("\n");
	 // print_hex(free_mem_addr); kprint("\n");
	 // for(uint32_t tmpaddr=0x0;tmpaddr<free_mem_addr;tmpaddr+=0x10){
		// 	page_t *respage;
		// 	respage=get_page(tmpaddr, 1, kernel_directory);
		// 	kprint("tmpaddr: "); print_hex(tmpaddr); kprint("\n");
		// 	// kprint("&respage: "); print_hex(&respage); kprint("\n");
		// 	kprint("respage->frame: "); print_hex(respage->frame); kprint("\n");
	 // }
	 //
	 // kprint("kernel_directory: "); print_hex(kernel_directory); kprint("\n");
	 //
	 // uint32_t cr3;
	 // asm volatile("mov %%cr3, %0": "=r"(cr3));	//先将cr0寄存器的值取出来保存在变量cr0中
	 // print_hex(cr3); kprint("\n");
}

void switch_page_directory(page_directory_t *dir)
{
   current_directory = dir;
   asm volatile("mov %0, %%cr3":: "r"(&dir->tablesPhysical));
   uint32_t cr0;
   asm volatile("mov %%cr0, %0": "=r"(cr0));	//先将cr0寄存器的值取出来保存在变量cr0中
	 cr0 |= 0x80000000; // Enable paging!
   asm volatile("mov %0, %%cr0":: "r"(cr0));
}

//输入虚拟内存地址，返回要获取的页物理内存地址
page_t *get_page(uint32_t address, int make, page_directory_t *dir)
{
	//address 输入想要访问的虚拟内存地址
   // Turn the address into an index. address/=0x1000后就变成了对应的页目表条目序号
   address /= 0x1000;
   // Find the page table containing this address. table_idx是想要访问的虚拟内存地址所在的页目录条目序号
   uint32_t table_idx = address / 1024;
   if (dir->tables[table_idx]) // If this table is already assigned，如果这个页目录已经存在了
   {
       return &dir->tables[table_idx]->pages[address%1024];
   }
   else if(make)
   {
       uint32_t tmp;
       dir->tables[table_idx] = (page_table_t*)kmalloc_ap(sizeof(page_table_t), &tmp);
       memory_set(dir->tables[table_idx], 0, 0x1000);
       dir->tablesPhysical[table_idx] = tmp | 0x7; // PRESENT, RW, US.
       return &dir->tables[table_idx]->pages[address%1024];
   }
   else
   {
       return 0;
   }
}
