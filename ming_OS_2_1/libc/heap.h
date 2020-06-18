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

#define KHEAP_START         0x20000000 //堆堆起始地址 25MB处 0x20000000
#define KHEAP_INITIAL_SIZE  0x100000  //堆堆初始化大小1MB 0x100000
#define HEAP_INDEX_SIZE   0x20000
#define HEAP_MAGIC        0x123890AB
#define HEAP_MIN_SIZE     0x70000

/**
  Size information for a hole/block
**/
typedef struct
{
   uint32_t magic;   // Magic number, used for error checking and identification.
   uint8_t is_hole;   // 1 if this is a hole. 0 if this is a block.
   uint32_t size;    // size of the block, including the end footer.
} header_t;

typedef struct
{
   uint32_t magic;     // Magic number, same as in header_t.
   header_t *header; // Pointer to the block header.
} footer_t;

typedef struct
{
   ordered_array_t index;
   uint32_t start_address; // The start of our allocated space.
   uint32_t end_address;   // The end of our allocated space. May be expanded up to max_address.
   uint32_t max_address;   // The maximum address the heap can be expanded to.
   uint8_t supervisor;     // Should extra pages requested by us be mapped as supervisor-only?
   uint8_t readonly;       // Should extra pages requested by us be mapped as read-only?
} heap_t;

heap_t *kheap;

/**
  Create a new heap.
**/
// heap_t *create_heap(int32_t start, int32_t end_addr, int32_t max, uint8_t supervisor, uint8_t readonly);

/**
  Allocates a contiguous region of memory 'size' in size. If page_align==1, it creates that block starting
  on a page boundary.
**/
// void *alloc(uint32_t size, uint8_t page_align, heap_t *heap);
/**
  Releases a block allocated with 'alloc'.
**/
// void free(void *p, heap_t *heap);

#endif
