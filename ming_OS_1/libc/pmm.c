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
