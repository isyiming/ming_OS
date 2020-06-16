#include "mem.h"


void memory_copy(uint8_t *source, uint8_t *dest, int nbytes) {
    int i;
    for (i = 0; i < nbytes; i++) {
        *(dest + i) = *(source + i);
    }
}

void memory_set(uint8_t *dest, uint8_t val, uint32_t len) {
    uint8_t *temp = (uint8_t *)dest;
    for ( ; len != 0; len--) *temp++ = val;
}



uint32_t free_mem_addr = kern_end;

//无 页面对齐
uint32_t kmalloc(uint32_t sz)
{
  uint32_t tmp = free_mem_addr;
  free_mem_addr += sz;
  return tmp;
}

/* Implementation is just a pointer to some free memory which
 * keeps growing */
uint32_t _kmalloc(size_t size, int align, uint32_t *phys_addr) {
    /* Pages are aligned to 4K, or 0x1000 */
    if (align == 1 && (free_mem_addr & 0xFFFFF000)) {
        free_mem_addr &= 0xFFFFF000;
        free_mem_addr += 0x1000;
    }
    /* Save also the physical address */
    if (phys_addr){
        *phys_addr = free_mem_addr;
    }

    uint32_t ret = free_mem_addr;
    free_mem_addr += size; /* Remember to increment the pointer */
    return ret;
}


void *
kmalloc0(uint32_t sz)
{
	return (_kmalloc(sz, NULL,  0x2));
}

//页面对齐
void *kmalloc_a(uint32_t sz)
{

	return (_kmalloc(sz, 1, NULL));
}

//返回物理地址但是不对齐
void *kmalloc_p(uint32_t sz, uint32_t *phys)
{

	return (_kmalloc(sz, 0, phys));
}

//返回物理地址同时对齐
void *kmalloc_ap(uint32_t sz, uint32_t *phys)
{

	return (_kmalloc(sz, 1, phys));
}



void
kfree(void *ptr)
{

	// free(ptr, kernel_heap);
}
