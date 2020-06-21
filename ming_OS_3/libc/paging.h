#ifndef PAGING_H
#define PAGING_H


#include <stdint.h>
#include <stddef.h>
#include "mem.h"
#include "../cpu/isr.h"
#include "../drivers/screen.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include "../libc/heap.h"

///////////////////////////////////////////
// 支持的最大物理内存地址(512MB 0x20000000,) 我这个qemu模拟的i386不知道怎么回事，都没有0xF00000？有时间
#define PMM_END_ADDR   0x20000000
#define PMM_START_ADDR 0x100000

// 物理内存页框大小
#define PMM_PAGE_SIZE 0x1000

// 最多支持的物理页面个数
#define PAGE_MAX_SIZE (PMM_END_ADDR-PMM_START_ADDR)/PMM_PAGE_SIZE

extern uint8_t kern_start[];//链接的时候指定的内核代码段起始位置，在link.ld中
extern uint8_t kern_end[];

//页表条目PTE的定义 一个页表条目应该是32bit（在32位系统中）
typedef struct page
{
   uint32_t present    : 1;   // Page present in memory
   uint32_t rw         : 1;   // Read-only if clear, readwrite if set
   uint32_t user       : 1;   // Supervisor level only if clear
   uint32_t accessed   : 1;   // Has the page been accessed since last refresh?
   uint32_t dirty      : 1;   // Has the page been written to since last refresh?
   uint32_t unused     : 7;   // Amalgamation of unused and reserved bits
   uint32_t frame      : 20;  // Frame address (shifted right 12 bits)
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
   /**
      Array of pointers to pagetables.
   **/
   page_table_t *tables[128];
   /**
      Array of pointers to the pagetables above, but gives their *physical*
      location, for loading into the CR3 register.
   **/
   uint32_t tablesPhysical[128];
   /**
      The physical address of tablesPhysical. This comes into play
      when we get our kernel heap allocated and the directory
      may be in a different location in virtual memory.
   **/
   uint32_t physicalAddr;
} page_directory_t;

static void set_frame(uint32_t frame_addr);
static void clear_frame(uint32_t frame_addr);
static uint32_t test_frame(uint32_t frame_addr);
static uint32_t first_frame();
void alloc_frame(page_t *page, int is_kernel, int is_writeable);
void free_frame(page_t *page);

void initialise_paging();
void switch_page_directory(page_directory_t *dir);
page_t *get_page(uint32_t address, int make, page_directory_t *dir);


#endif
