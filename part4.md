# mingOS
### 系统的虚拟内存实现
### 配合《深入理解计算机系统》食用

---------------
### part4 进程

进程和线程在实现上的区别就在于，我们是否给他们配置了单独的页表和页目录。我们之前的全部程序，都是一个顺序的逻辑执行的，唯一的特殊情况就是中断触发的时候了。进程切换对CPU来讲，就是上下文切换。想要实现多进程，其实就把进程切换搞明白就可以啦。而涉及到最全面的函数应该就是fork函数了。那就按照fork的实现流程来说进程切换的实现吧。


为子进程创建一个新的页表和页目录
```c
page_directory_t *clone_directory(page_directory_t *src)
{
		u32int phys;
		// Make a new page directory and obtain its physical address.
		page_directory_t *dir = (page_directory_t*)kmalloc_ap(sizeof(page_directory_t), &phys);
		// Ensure that it is blank.
		memset(dir, 0, sizeof(page_directory_t));
		// Get the offset of tablesPhysical from the start of the page_directory_t structure.
		u32int offset = (u32int)dir->tablesPhysical - (u32int)dir;

		// Then the physical address of dir->tablesPhysical is:
		dir->physicalAddr = phys + offset;

		int i;
		for (i = 0; i < 1024; i++)
		{
				if (!src->tables[i])
						continue;
						current_directory == kernel_directory;
						if (kernel_directory->tables[i] == src->tables[i])
		        {
		           // It's in the kernel, so just use the same pointer.
		           dir->tables[i] = src->tables[i];
		           dir->tablesPhysical[i] = src->tablesPhysical[i];
		        }
		        else
		        {
		           // Copy the table.
		           u32int phys;
		           dir->tables[i] = clone_table(src->tables[i], &phys);
		           dir->tablesPhysical[i] = phys | 0x07;
		        }
					}
		return dir;
}

static page_table_t *clone_table(page_table_t *src, u32int *physAddr)
{
   // Make a new page table, which is page aligned.
   page_table_t *table = (page_table_t*)kmalloc_ap(sizeof(page_table_t), physAddr);
   // Ensure that the new table is blank.
   memset(table, 0, sizeof(page_directory_t));

   // For every entry in the table...
   int i;
   for (i = 0; i < 1024; i++)
   {
     if (!src->pages[i].frame)
       continue;
			 // Get a new frame.
       alloc_frame(&table->pages[i], 0, 0);
       // Clone the flags from source to destination.
       if (src->pages[i].present) table->pages[i].present = 1;
       if (src->pages[i].rw)      table->pages[i].rw = 1;
       if (src->pages[i].user)    table->pages[i].user = 1;
       if (src->pages[i].accessed)table->pages[i].accessed = 1;
       if (src->pages[i].dirty)   table->pages[i].dirty = 1;
       // Physically copy the data across. This function is in process.s.
       copy_page_physical(src->pages[i].frame*0x1000, table->pages[i].frame*0x1000);
		 }
		return table;
}
```
对于内核代码和堆，只需要进行映射这个页表就好了。而属于进程自己的页表，就要复制了。

复制页
```
[GLOBAL copy_page_physical]
copy_page_physical:
   push ebx              ; According to __cdecl, we must preserve the contents of EBX.
   pushf                 ; push EFLAGS, so we can pop it and reenable interrupts
                         ; later, if they were enabled anyway.
   cli                   ; Disable interrupts, so we aren't interrupted.
                         ; Load these in BEFORE we disable paging!
   mov ebx, [esp+12]     ; Source address
   mov ecx, [esp+16]     ; Destination address

   mov edx, cr0          ; Get the control register...
   and edx, 0x7fffffff   ; and...
   mov cr0, edx          ; Disable paging.

   mov edx, 1024         ; 1024*4bytes = 4096 bytes to copy

.loop:
   mov eax, [ebx]        ; Get the word at the source address
   mov [ecx], eax        ; Store it at the dest address
   add ebx, 4            ; Source address += sizeof(word)
   add ecx, 4            ; Dest address += sizeof(word)
   dec edx               ; One less word to do
   jnz .loop

   mov edx, cr0          ; Get the control register again
   or  edx, 0x80000000   ; and...
   mov cr0, edx          ; Enable paging.

   popf                  ; Pop EFLAGS back.
   pop ebx               ; Get the original value of EBX back.
   ret
```
这段汇编代码是JamesM的教程中的代码，再次感谢JamesM！
在开始复制分页的时候，要关闭中断，禁用分页。这样保证复制分页顺利进行。实际上Linux的实现上，使用的是写时复制的方式，只有在对某一页写操作的时候，才进行真正的页面复制，否则只是进行页面映射。

```c

void initialise_paging()
{
   // The size of physical memory. For the moment we
   // assume it is 16MB big.
   u32int mem_end_page = 0x1000000;

   nframes = mem_end_page / 0x1000;
   frames = (u32int*)kmalloc(INDEX_FROM_BIT(nframes));
   memset(frames, 0, INDEX_FROM_BIT(nframes));

   // Let's make a page directory.
   u32int phys; // ********** ADDED ***********
   kernel_directory = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
   memset(kernel_directory, 0, sizeof(page_directory_t));
   // *********** MODIFIED ************
   kernel_directory->physicalAddr = (u32int)kernel_directory->tablesPhysical;

   // Map some pages in the kernel heap area.
   // Here we call get_page but not alloc_frame. This causes page_table_t's
   // to be created where necessary. We can't allocate frames yet because they
   // they need to be identity mapped first below, and yet we can't increase
   // placement_address between identity mapping and enabling the heap!
   int i = 0;
   for (i = KHEAP_START; i < KHEAP_END; i += 0x1000)
       get_page(i, 1, kernel_directory);

   // We need to identity map (phys addr = virt addr) from
   // 0x0 to the end of used memory, so we can access this
   // transparently, as if paging wasn't enabled.
   // NOTE that we use a while loop here deliberately.
   // inside the loop body we actually change placement_address
   // by calling kmalloc(). A while loop causes this to be
   // computed on-the-fly rather than once at the start.
   // Allocate a lil' bit extra so the kernel heap can be
   // initialised properly.
   i = 0;
   while (i < placement_address+0x1000)
   {
       // Kernel code is readable but not writeable from userspace.
       alloc_frame( get_page(i, 1, kernel_directory), 0, 0);
       i += 0x1000;
   }

   // Now allocate those pages we mapped earlier.
   for (i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += 0x1000)
       alloc_frame( get_page(i, 1, kernel_directory), 0, 0);

   // Before we enable paging, we must register our page fault handler.
   register_interrupt_handler(14, page_fault);

   // Now, enable paging!
   switch_page_directory(kernel_directory);

   // Initialise the kernel heap.
   kheap = create_heap(KHEAP_START, KHEAP_START+KHEAP_INITIAL_SIZE, 0xCFFFF000, 0, 0);

   // ******** ADDED *********
   current_directory = clone_directory(kernel_directory);
   switch_page_directory(current_directory);
}

void switch_page_directory(page_directory_t *dir)
{
   current_directory = dir;
   asm volatile("mov %0, %%cr3":: "r"(dir->physicalAddr)); // ******** MODIFIED *********
   u32int cr0;
   asm volatile("mov %%cr0, %0": "=r"(cr0));
   cr0 |= 0x80000000; // Enable paging!
   asm volatile("mov %0, %%cr0":: "r"(cr0));
}
```
当然这里的分页初始化函数也需要更改了。原来的current_directory直接赋值为kernel_directory。现在我们使用clone_directory，真正为它创建一个独有的页目录和页表。




13.内存映射
  Linux系统将一个虚拟内存区域与一个磁盘上的对象关联
  虚拟内存区域可以映射到两种类型的文件中的一种：
    1）Linux文件系统中的普通文件，这种情况下只是将PTE更改了，但是磁盘上的文件没有真正复制到主存中，只有cpu访问该文件触发缺页异常后，才载入主存。
    2）匿名文件，映射到匿名文件时，这个就是由内核创建的了，这个文件中都是二进制0.然后如果可以的话，直接将内存中的该页写为二进制0.这样磁盘和主存之间没有数据传送
    无论那种情况，一旦一个虚拟页被创建了，它就在一个由内核维护的专门的交换文件（swap file）之间换来换去。任何时刻，交换空间都限制着当前进程能够分配的虚拟页面数目。

交换空间 交换空间swap space是啥？

14.内存映射和共享对象
  进程这一抽象可以为每个进程提供独立的私有虚拟内存空间，但是有时候有些数据是多个进程共用的。例如程序包含的库文件，系统调用。如果每个进程都在物理内存中保留它们的副本，那太占用内存了。
  一个对象如果可以被映射到虚拟内存中的一个区域，要么作为共享对象，要么作为私有对象。

  如果一个对象被映射为共享对象，那么共享该对象的进程都可以看到这个对象的改动。
  如果一个对象被映射为私有对象，那么该对象只对它所在的进程可见。
  每个对象都有唯一的文件名，所以内核可以快速判定进程1已经映射了这个对象。（文件系统这里还没看呢呀）

15.私有对象的写时复制 copy-on-write
  如果两个进程创建时有一个相同的私有对象，比如两个进程都调用了同一个库，但是都设置为私有的。
  那么在物理内存中只会有一个私有对象真实存在，只有当其中一个进程要对一个私有区域的某个页面时，就会触发一个保护故障。如果保护故障程序发现保护异常是由于进程对私有写时复制区域进行修改引起的，那就将该对象在物理内存中创建一个新的副本。这样做还是为了充分利用稀有的内存资源。

 动态内存分配
  1.低级的mmap和munmap可以动态的创建和删除虚拟内存区域。但是使用动态内存分配器更方便内存管理。动态内存分配器维护着一个进程的虚拟内存区域，称之为堆。堆紧接着.bss开始，向上生长。对于每个进程，内核维护着一个变量brk，它指向堆顶。堆定以上是共享库的内存映射区，再上面是栈。分配器将堆视为一组不同大小的块block的集合，每个块都是一个连续的虚拟内存空间，要么是已经分配的，要么是空闲的。
  分配器有两种实现风格：
    显式分配器：要求应用显式的释放任何已经分配的块。C标准库提供malloc和free函数来分配和释放block。c++中的new和free于此相当。
    隐式分配器：要求分配器检测什么时候这个block不再使用，那么就释放这个块。所以隐式分配器也称之为垃圾收集器，而自动释放块的过程称之为垃圾回收。java等高级语言就依赖的垃圾回收机制。OC也是哎。
  malloc函数，输入size，返回一个指针，指向大小为size的块。如果没有足够的内存空间，就返回null。malloc不初始化内存块，只返回地址。
  calloc初始化内存块。
  free函数释放已经分配的内存块，但是free没有返回值，只是根据输入地址释放，所以如果输入地址不当，可能会有难以预料的错误。
  2.碎片
  碎片使得堆的利用率降低。
  内部碎片：指的是block块内的碎片，分配的块比请求的块大，就会产生碎片。对齐是一个原因。
  外部碎片：当空闲碎片合计可以满足一个新的分配请求，但是

16. 再看fork函数
  fork函数调用一次，返回两次。返回给父进程子进程的PID。返回给子进程0。如果出现错误，就返回一个负值。
  fork出的子进程会完全复制父进程当前的状态，所以它被创建后会和父进程同步继续执行下去。

  当fork函数被当前进程调用后，内核会为新的进程创建各种数据结构，并分配给它一个唯一的PID（进程标识符，process ID）。它创建了当前进程的mm_struct,区域结构和页表的原样副本。它将两个进程的每个页面都标记为只读，并将两个进程中的每个私有区域都标记为写时复制。
  当fork函数在新的子进程返回时，新进程现在的虚拟内存刚好和调用fork时存在的虚拟内存相同。当两个进程中的任意一个对私有区域进行写操作时，写时复制机制就会在物理内存中创建新页面。

17. 再看execve函数
  execve函数在当前进程中加载并运行包含在可执行目标文件a.out，用a.out有效地替代了当前程序。加载并运行a.out需要如下步骤：
    删除已经存在的用户区域。删除当前进程虚拟地址的用户部分中已经存在的区域结构。
    映射私有区域。为新程序的代码，数据，bss和栈区域创建新的区域结构。所有的这些新的区域都是私有的，写时复制的。
    映射共享区域。如果a.out与共享对象链接，那么这些对象就会动态的链接到这个程序，然后在映射到用户虚拟地址空间中的共享区域。
    设置程序计数器。execve做的最后一件事就是设置当前进程上下文的程序计数器，这样子进程和父进程就同步执行了。

18. 使用mmap函数的用户级内存映射
  Linux进程可以使用mmap函数来创建新的虚拟内存区域，并将对象映射到这些区域中。

15. 共享对象

15.Linux的虚拟内存系统
  系统为每个进程都维护一个单独的虚拟内存空间，从0x4000 0000开始是：代码段.text，静态存储区和全局变量.data，未初始化的数据.bss, heap堆（进程调用malloc分配内存的时候，新的内存就分配到堆上），共享内存区（默认大小是32M），stack栈（用户创建的局部变量），内核代码和数据，物理内存，与进程相关的数据结构，如页表等。


16. Linux的缺页异常处理



9.虚拟内存作为保护内存的工具
  通过PTE的上的限制，比如在PTE上加上额外的许可位来限制进程的访问。SUP表示只有内核才能访问，READ和WRITE位表示读写权限。
  如果一个进程违反了这个许可条件，就会触发一个一般保护故障，将控制传递给一个内核中的异常处理程序。shell一般将这中异常报告为“段错误”。
10.地址翻译
  1）处理器产生一个虚拟地址，将它传递给MMU。
    实际上对于处理器来将，它不知道这个地址是不是虚拟的，它只负责要一个地址的数据。但是这个地址是不是有效的那是虚拟内存机制的来处理的。
  2）MMU（内存管理单元）生成PTE地址，并向主存请求得到它。其实就是MMU查阅页表。
  3）主存将PTE返回给MMU。查阅页表成功了。
  4）MMU根据PTE构造物理地址，并从主存中读取这个地址里的数据。   这一步要是缺页了，就会触发异常。那cpu就执行中断程序了，那是另一个程序逻辑了，我们不用理会。反正最后，主存要载入这一页的数据。
  5）主存将数据传递给cpu。

11.利用TLB（翻译后备缓冲器）
  CPU每调用一次主存中的数据，MMU就要在主存中查阅一次PTE，PTE数据量很小，但是MMU访问主存这个时间相比CPU的时钟周期太慢了。所以通常在SRAM（L1）中有一个PTE的缓存区，因为SRAM是比主存还要高速的设备，所以减少了频繁的访问主存，而只为了一条PTE数据。



1.Linux下一个进程可以开多少线程
  取决于设定的线程最小栈空间大小
  32位linux系统最大内存地址4G，0-3GB的给用户进程(User Space)使用，3-4GB给内核使用
　stack size (kbytes,-s)10240表示线程堆栈大小,3G/10M=最大线程数，
