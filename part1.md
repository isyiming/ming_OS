# mingOS
### 系统的虚拟内存实现
### 配合《深入理解计算机系统》食用

我们首先来愉快的看下系统的虚拟内存是如何实现的吧～
---------------
### part1 获取可用的内存空间地址范围

还记得cfenollosa在他的os-tutorial最后一节添加的那个kmalloc()函数吗？
```c
/* This should be computed at link time, but a hardcoded
 * value is fine for now. Remember that our kernel starts
 * at 0x1000 as defined on the Makefile */
uint32_t free_mem_addr = 0x10000;
/* Implementation is just a pointer to some free memory which
 * keeps growing */
uint32_t kmalloc(size_t size, int align, uint32_t *phys_addr) {
    /* Pages are aligned to 4K, or 0x1000 * /
    if (align == 1 && (free_mem_addr & 0x00000FFF)) {
        free_mem_addr &= 0xFFFFF000;
        free_mem_addr += 0x1000;
    }
    /* Save also the physical address * /
    if (phys_addr){
        *phys_addr = free_mem_addr;
    }

    uint32_t ret = free_mem_addr;
    free_mem_addr += size; /* Remember to increment the pointer * /
    return ret;
}
```
这个函数可以为内核返回未使用的内存空间地址。但是当时他的实现有缺陷。free_mem_addr时刻记录着未分配的地址的起点。这是内核启动后就开始调用的函数，此时内核代码刚刚加载到主存中。除了内核代码，还有中断向量表IDT和全局描述符表GDT这些内核在启动时加载到主存中的数据结构。还记不记得GDT要在内核启动之前就要加载好，所以它开始只能放在1M以下的地址空间中。总之，我们知道内核代码加载，并且初始化一些数据结构后，较高地址空间都是未使用的。那么我们设定一个起始地址free_mem_addr，由free_mem_addr～4GB为内核分配内存就好了。

free_mem_addr就是时刻记录着未经使用过的内存段的起始地址。这个其实地址最开始应该是内核占用内存的结束位置，应该在链接的时候记录下来的。但是cfenollosa直接指定了一个地址：0x10000。
我们看下makefile文件中的链接指令：

```makefile
i386-elf-ld -o $@ -Ttext 0x1000 $^ --oformat binary
```
-Ttext 0x1000 指定了代码段的起始地址是0x1000。而free_mem_addr的起始地址是0x10000，看来cfenollosa默认了自己的代码段长度没超过0xF000。我就感觉这个地址很奇怪，既然你不知道内核占用内存的结束位置，那你就设定一个安全一点的位置呀，怎么也应该是1M以后的地址。因为实模式只能寻址到1M。

好了，不管这个了。我们说下真正可靠的方法是如何做的吧。我们要在链接的时候记录下内核加载到内存的结束位置。
```
/* Link.ld -- Linker script for the kernel - ensure everything goes in the */
/*            Correct place.  */
/*            Original file taken from Bran's Kernel Development */
/*            tutorials: http://www.osdever.net/bkerndev/index.php. */
/* 这是 JamesM 的教程中的链接文件，ming在此添加了记录内核代码段起始位置和内核占用内存结束位置的两个变量：kern_start 和kern_end
ENTRY(start)
SECTIONS
{
  . = 0x1000;
  PROVIDE( kern_start = . );
  .text  :
  {
    code = .; _code = .; __code = .;
    *(.text)
    . = ALIGN(4096);
  }

  .data :
  {
     data = .; _data = .; __data = .;
     *(.data)
     *(.rodata)
     . = ALIGN(4096);
  }

  .bss :
  {
    bss = .; _bss = .; __bss = .;
    *(.bss)
    . = ALIGN(4096);
  }

  PROVIDE( kern_end = . );

  end = .; _end = .; __end = .;
}
```
将这个link.ld文件放在makefile的同级目录下，我们再修改下Makefile文件中的链接指令
```makefile
# '--oformat binary' deletes all symbols as a collateral, so we don't need
# to 'strip' them manually on this case
kernel.bin: boot/kernel_entry.o ${OBJ}
	# i386-elf-ld -o $@ -Ttext 0x1000 $^ --oformat binary
	i386-elf-ld -Tlink.ld -melf_i386 -o $@ $^ --oformat binary

# Used for debugging purposes
kernel.elf: boot/kernel_entry.o ${OBJ}
	# i386-elf-ld -o $@ -Ttext 0x1000 $^
	i386-elf-ld -Tlink.ld -melf_i386 -o $@ $^
```
原来cfenollosa的代码（就是被我注释掉的部分），-Ttext 0x1000 的意思是指定了代码段的起始地址是0x1000。我们不再直接在链接指令中指定代码段的起始位置了，而是在链接脚本中指定。并在link.ld中用kern_start 和kern_end两个变量记录下代码段的起始地址和内核加载到内存的结束位置。这两个变量在.c文件中extern下就可以使用了。
我们在men.h中添加如下代码：
```c
extern uint8_t kern_start[];//链接的时候指定的内核代码段起始位置，在link.ld中
extern uint8_t kern_end[];
```
由于kernel.c包含了men.h 就可以直接使用这两个变量了。那就在内核启动的时候，打印下这两个变量吧
```c
void kernel_main() {
    isr_install();
    irq_install();

    asm("int $2");
    asm("int $3");

    kprint("Type something, it will go through the kernel\n"
        "Type END to halt the CPU or PAGE to request a kmalloc()\n> ");

    char kern_start_str[16] = "";
    char kern_end_str[16] = "";
    hex_to_ascii(kern_start, kern_start_str);
    hex_to_ascii(kern_end, kern_end_str);
    kprint("kernel in memory start: ");      kprint(kern_start_str);      kprint("\n");
    kprint("kernel in memory end  : ");      kprint(kern_end_str);      kprint("\n");
}
```
记得更改程序后，再make之前要先make clean下，清除之前的目标文件，因为我们这次链接部分有了改动。不然就找不到kern_end和kern_end_str了。最后启动系统后的结果，会首先输出下：
```c
> kernel in memory start: 0x1000
kernel in memory end  : 0x4000
```

我们在链接的时候除了text段，还申请了data和bss段，实际上除了GDT和IDT，现在我还不确定内核还加载了那些数据结构到主存中。我印象中应该是没有。而GDT和IDT加载到了什么位置，我之前看cfenollosa的教程的时候没有注意，但是今天也懒得回去看啦～

好了，最后我们把free_mem_addr的初始值设置为kern_end吧～
```c
> kernel in memory start: 0x1000
kernel in memory end  : 0x5c20
PAGE
Page: 0x6000, physical address: 0x6000
You said: You said: PAGE
```
这样我们就获取了主存中除去内核外，可用的地址范围了。注意下，新开辟的空间是从0x6000开始的，而不是0x5c20，因为我们设置了页面对齐嘛。

再想一下，如果我们把内核加载的起始地址设置为0x100000会怎样？那就加载不了了。因为16位模式下没办法寻址到1M以上空间。

---------------
### 选择合适的数据结构描述物理内存
将第一部分的代码略作修改，将内核启动后可使用的内存空间分页，然后将每一页的地址放到一个数组中保存起来吧。感觉页表的眉目逐渐显现了哈哈。
添加了pmm.c和pmm.h

```c
/*
 * =====================================================================================
 *
 *       Filename:  pmm.h
 *
 *    Description:  页面内存管理
 *
 *        Version:  1.0
 *        Created:  2020年6月1日 23时06分35秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ming, isyiming.zhang@outlook.com
 *   这个页表管理的代码是Hurley的原创，我在cfenollosa的基础上继续实现系统的时候，原本是看JamesM的文章。
 *   但是那个网站年久失修，完整的代码都难以找到了。好在发现了Hurley的文档，他也是在JamesM先生的基础上完善代码。
 *   并且将他重构的代码无私的开源。感谢他的贡献，相信为很多人学习操作系统提供了遍历。
 *   无意中发现，我和他竟然还是校友，真是奇妙的缘分，感谢学长！
 * =====================================================================================
 */
#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include "mem.h"
#include "../cpu/isr.h"
#include "../drivers/screen.h"
#include "../libc/string.h"
#include "../libc/pmm.h"

// 支持的最大物理内存地址(512MB 0x20000000)
#define PMM_END_ADDR   0x110000
#define PMM_START_ADDR 0x100000

// 物理内存页框大小
#define PMM_PAGE_SIZE 0x1000

// 最多支持的物理页面个数
#define PAGE_MAX_SIZE (PMM_END_ADDR-PMM_START_ADDR)/PMM_PAGE_SIZE

extern uint8_t kern_start[];//链接的时候指定的内核代码段起始位置，在link.ld中
extern uint8_t kern_end[];

// 物理内存页面管理的数组
typedef
struct {
  uint32_t PhysicalAddr[PAGE_MAX_SIZE+1];
  uint32_t topAddr;
 uint32_t phy_page_count;//当前栈中保存的页面地址的数量，动态分配物理内存页的总数
} PMM_STACK;

PMM_STACK pmm_stack;

// 初始化物理内存管理
void init_pmm();
uint32_t pmm_alloc_page();

#endif
```

其实这个pmm就是定义了一个数组，将内核可分配的内存按照4k的间隔划分开，将每一页起始地址保存在pmm_stack中。

```c
/*
 * =====================================================================================
 *
 *       Filename:  pmm.c
 *
 *    Description:  页面内存管理
 *
 *        Version:  1.0
 *        Created:  2020年6月1日 23时06分35秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ming, isyiming.zhang@outlook.com
 *   这个页表管理的代码是Hurley的原创，我在cfenollosa的基础上继续实现系统的时候，原本是看JamesM的文章。
 *   但是那个网站年久失修，完整的代码都难以找到了。好在发现了Hurley的文档，他也是在JamesM先生的基础上完善代码。
 *   并且将他重构的代码无私的开源。感谢他的贡献，相信为很多人学习操作系统提供了遍历。
 *   无意中发现，我和他竟然还是校友，真是奇妙的缘分，感谢学长！
 * =====================================================================================
 */
// #include "multiboot.h"
// #include "common.h"

#include "pmm.h"


void init_pmm()
{
 //将第一页data【0】设置为0，可以用以验证该页表是否有效。当然这里就没什么用了，可能后面虚拟内存的页表会在此基础上改写。
 pmm_stack.PhysicalAddr[0]=0;
 pmm_stack.topAddr=&pmm_stack.PhysicalAddr[PAGE_MAX_SIZE];

 pmm_stack.phy_page_count=0;

 for (uint32_t page_addr = PMM_START_ADDR; page_addr < PMM_END_ADDR; page_addr+=PMM_PAGE_SIZE) {
     // 把内核结束位置到结束位置的内存段，按页存储到页管理栈里
     // 最多支持到PMM_START_ADDR 512MB的物理内存
     //因为我这里设定的就是4k的整数了，所以没必要对齐了
     pmm_stack.phy_page_count++;
     pmm_stack.PhysicalAddr[pmm_stack.phy_page_count]=page_addr;//入栈

     kprint("init page index: "); print_int(pmm_stack.phy_page_count);kprint("   ");
     kprint("  page addr      : "); print_hex(page_addr);      kprint("   ");  print_hex(PMM_END_ADDR);   kprint("\n");
 }

 pmm_stack.phy_page_count=0;
}

uint32_t pmm_alloc_page()
{
 //超过最大分配页数限制
 if(pmm_stack.phy_page_count>=PAGE_MAX_SIZE){
   kprint("Out of memory，please free memory\n");
   return -1;
 }
 else{
   pmm_stack.phy_page_count++;
   uint32_t page = pmm_stack.PhysicalAddr[pmm_stack.phy_page_count];
   return page;
 }
}

```
两个函数的实现就是这样的。很简单，我在init_pmm()中打印了每一页的起始地址，在Kernel.c中打印了pmm_stack.PhysicalAddr这个数组的起始和终止地址。
```c
void kernel_main() {
    isr_install();
    irq_install();

    asm("int $2");
    asm("int $3");

    kprint("Type something, it will go through the kernel\n"
        "Type END to halt the CPU or PAGE to request a kmalloc()\n> ");

    kprint("kernel in memory start: ");      print_hex(kern_start);      kprint("\n");
    kprint("kernel in memory end  : ");      print_hex(kern_end);      kprint("\n");

    init_pmm();

    //打印保存页表条目的地址空间
    uint32_t * add=pmm_stack.PhysicalAddr;
    kprint("pmm_stack.PhysicalAddr start address: "); print_hex(add); kprint("\n");
    add=pmm_stack.topAddr;
    kprint("pmm_stack.PhysicalAddr end address: "); print_hex(add); kprint("\n");
}
```
我们来看下打印的结果。
```c
kernel in memory start: 0x1000
kernel in memory end  : 0x5c80
init page index: 1     page addr      : 0x100000   0x110000
init page index: 2     page addr      : 0x101000   0x110000
init page index: 3     page addr      : 0x102000   0x110000
init page index: 4     page addr      : 0x103000   0x110000
init page index: 5     page addr      : 0x104000   0x110000
init page index: 6     page addr      : 0x105000   0x110000
init page index: 7     page addr      : 0x106000   0x110000
init page index: 8     page addr      : 0x107000   0x110000
init page index: 9     page addr      : 0x108000   0x110000
init page index: 10     page addr      : 0x109000   0x110000
init page index: 11     page addr      : 0x10:000   0x110000
init page index: 12     page addr      : 0x10b000   0x110000
init page index: 13     page addr      : 0x10c000   0x110000
init page index: 14     page addr      : 0x10d000   0x110000
init page index: 15     page addr      : 0x10e000   0x110000
init page index: 16     page addr      : 0x10f000   0x110000
pmm_stack.PhysicalAddr start address: 0x5000
pmm_stack.PhysicalAddr end address: 0x5040
```
可以看到，内核代码在编译时开辟的内存空间的起始和终止地址是0x1000-0x5c80，都在1M一下。我们设定的开始分页的起始地址是0x100000，一直到0x110000，这是我们在pmm.h里设置的。
