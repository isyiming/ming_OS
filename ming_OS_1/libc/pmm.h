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
