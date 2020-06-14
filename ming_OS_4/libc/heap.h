#ifndef HEAP_H
#define HEAP_H


#include <stdint.h>
#include <stddef.h>
#include "mem.h"
#include "../cpu/isr.h"
#include "../drivers/screen.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include "../libc/ordered_array.h"

// 堆起始地址
#define KHEAP_START         0xC0000000 //堆堆起始地址 25MB处 0x20000000
#define KHEAP_INITIAL_SIZE  0x100000  //堆堆初始化大小1MB 0x100000
#define HEAP_INDEX_SIZE   0x20000
#define HEAP_MAGIC        0x123890AB
#define HEAP_MIN_SIZE     0x70000

// 内存块管理结构
typedef
struct header {
	struct header *prev; 	// 前后内存块管理结构指针
	struct header *next;
	uint32_t allocated : 1;	// 该内存块是否已经被申请
	uint32_t length : 31; 	// 当前内存块的长度
} header_t;

// 初始化堆
void init_heap();

// 内存申请
void *kmalloc_heap(uint32_t len);

// 内存释放
void kfree(void *p);

// 测试内核堆申请释放
void test_heap();

#endif
