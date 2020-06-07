# mingOS
### 系统的虚拟内存实现
### 配合《深入理解计算机系统》食用

---------------
### part2 开始分页

那么开始构建关于页面管理的数据结构吧！

页表条目 PTE
```c
//页表条目PTE的定义 一个页表条目应该是32bit（在32位系统中）
typedef struct page
{
   u32int present    : 1;   // Page present in memory
   u32int rw         : 1;   // Read-only if clear, readwrite if set
   u32int user       : 1;   // Supervisor level only if clear
   u32int accessed   : 1;   // Has the page been accessed since last refresh?
   u32int dirty      : 1;   // Has the page been written to since last refresh?
   u32int unused     : 7;   // Amalgamation of unused and reserved bits
   u32int frame      : 20;  // Frame address (shifted right 12 bits)
} page_t;

//二级页表，每个二级页表中保存着1024个页表条目，那么一张二级页表就映射了1024*4KB=4MB的空间
typedef struct page_table
{
   page_t pages[1024];
} page_table_t;

//一级页表，每个一级页表中保存着1024张二级页表，那么一张一级页表就正好映射了4GB的内存空间
//page_directory里有两个数组，
//physicalAddr用于页目录复制或者切换进程时切换页目录。
typedef struct page_directory
{

   // Array of pointers to pagetables.
   page_table_t * tables[1024];

   //Array of pointers to the pagetables above, but gives their physical location, for loading into the CR3 register.
   u32int tablesPhysical[1024];

    // The physical address of tablesPhysical. This comes into play
    // when we get our kernel heap allocated and the directory
    // may be in a different location in virtual memory.
   u32int physicalAddr;
} page_directory_t;
```

在 void initialise_paging() 中首先要给要映射的内存空间的页表开辟内存。
```c
// The size of physical memory.
uint32_t mem_end_page = PMM_END_ADDR;
nframes = mem_end_page / PMM_PAGE_SIZE;
frames = (uint32_t*)kmalloc(INDEX_FROM_BIT(nframes));
memory_set(frames, 0, INDEX_FROM_BIT(nframes));
```
然后给kernel_directory页目录分配物理内存空间。
```c
kernel_directory = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));//为页目录分配空间
memory_set(kernel_directory, 0, sizeof(page_directory_t));
current_directory = kernel_directory;
```

```c
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
```

然后再把0~PMM_END_ADDR的内存空间都在kernel_directory中做好映射。PMM_END_ADDR是我们在pmm.h中设定的一个最大内存空间，我设定的是512MB。
这里有一个困扰我两天的bug，开始我设定PMM_END_ADDR为内核占用的地址结束位置kern_end。这样是不行的。至少在我们目前的代码上不行，一旦将cr0的分页标志位置1后，系统会不断重启。这是因为 https://stackoverflow.com/questions/38157968/qemu-triple-faults-when-enabling-paging
一定要将kernel_directory的全部页面都初始化。设定缺页异常的中断向量号是IRQ14。然后switch_page_directory将cr3寄存器保存为kernel_directory的物理地址，并且将cr0更改开启分页模式。

不过我还有一个bug没有解决，就是在开启分页后，显卡缓冲区的数据就突然变成了乱码。然后整个屏幕就充斥着乱码符号。但是不影响目前的系统使用，后面清下屏幕就好了。


---------------
### 内存管理的解决办法-虚拟内存
在保护模式下, 寄存器CR0的高位1表示开启分页.0表示不开启.

1. 虚拟内存
  为了更高效的管理多进程系统的内存。它为每一个进程提供一个大的，一致的私有的空间。
  虚拟内存提供了三种能力：
    1）将主存看作是磁盘地址空间的一个高速缓存，在主存中只保留活动区域
    2）它为每个进程提供了一致的地址空间，简化内存管理
    3）保护每个进程之间的地址空间独立，互相不破坏

2. 具体分页实现

  1). 内存分页：

      将物理内存空间分割为4kb大小的页，32位机器可以寻址2^32=4GB内存空间。将4GB空间划分为4KB大小的页，一共可分为的页数为：1MB

  2). 页表条目（page table entry，PTE）

      用一个unit32型变量（4个字节）来保存每一页的起始地址，这就是页表条目。

      但是我们的页是4K对齐的，所以这个32bit的变量的低12位总是0.保存在PTE里什么作用都不起太浪费了，那就用这12bit作为有效位吧。

  3). 页表

      将每个PTE保存在一个数组里，这就是页表。我们需要访问那一段内存的时候，就通过查询页表访问。

      用一个unit32型变量（4个字节）保存这个每一页的起始地址，一张页表映射4MB的内存空间。那么一张页表本身的大小就是

  4). PDT Page Directory Entry
      和PTE概念类似，保存着页表地址。

  5). 页目录
      保存PDT的数组。页目录的地址由CR3寄存器保存。这样可以快速找到页目录地址，比在内存中访问要快上百倍。

首先我们一定要区分虚拟内存和物理内存。在本节我一定在每一个地方都用这两个词，而不是用“内存”。
为了为每个应用程序提供相同的“4GB内存”，我们需要利用磁盘，在磁盘上一定有这么大的空间容纳我们分配的n*4GB的空间。
而物理内存将作为磁盘的缓存。再次强调，构建虚拟内存是提供给用户模式的应用程序使用的。我们为物理内存构建一张表格，表格中记录了每一页物理内存的起始地址。

一级页表中的每个PTE负责映射虚拟地址空间的一个4MB的片，每一片中有1024个连续的页。二级页表中的每个PTE负责映射虚拟地址空间的一个4kB的页。具体到实际的32位的地址寻址上，一级页表条目保存着高10位，对应着1024个4MB空间；二级页表保存着中间10位，对应着1024个4KB空间；剩下的12位正好代表4kb的地址偏移量。如果一级页表中的某一片是空的，那么二级页表就不会存在。只有一级页表需要总是存在主存中，频繁使用的二级页表缓存在主存中。这就是为何要使用二级页表的原因，完全是为了不要让4MB的页表一直占用物理内存空间。

那么当一个进程想要使用某一页虚拟内存的时候，比如说0x10000~0x11000这一页。系统怎样将这一页的内容传递给进程呢？
寻址过程：虚拟地址/0x1000=页表序号。 页序号/1024就是页目录PDT中第几个条目。找到这个页目录条目中保存的的物理地址就是页表的地址。（虚拟地址/0x1000）/0x1000就是PTE序号。读取PTE的20bit物理地址左移12bit，就是物理地址了。

cr0寄存器的高位用来开启分页。


还是要从平坦模型说起。既然我们不采用分段的内存管理模式了，那么整个4GB内存就被看成是一段。这里的内存指的是物理主存。哎，脑壳疼。系统要为每一个进程创建对应的页表和页目录。进程基于系统的虚拟内存这一抽象，它无需考虑具体的物理地址。那么对每个进程而言，它可以访问的地址当然从头开始比较方便呀。所以虚拟内存中，0-3GB的地址空间就划分给进程了。但是如果你从头写过操作系统就一定知道，最开始系统内核其实是在物理内存的低位置处的。Linux采用的页面管理方案是：内核映射到线性地址3GB以上空间，而应用程序占据线性地址空间的0-3GB位置。但是我暂时不打算这样做了，因为最近时间真的很紧张，以后再看看吧。

这一部分我们要在part1的基础上实现虚拟内存的页面映射功能。不知道今晚上可不可以搞完，但是很想快点搞定。果然没搞完哈哈。
