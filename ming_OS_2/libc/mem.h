#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h>

extern uint8_t kern_start[];//链接的时候指定的内核代码段起始位置，在link.ld中
extern uint8_t kern_end[];

void memory_copy(uint8_t *source, uint8_t *dest, int nbytes);
void memory_set(uint8_t *dest, uint8_t val, uint32_t len);


uint32_t kmalloc(uint32_t sz);

/* At this stage there is no 'free' implemented. */
uint32_t _kmalloc(size_t size, int align, uint32_t *phys_addr);

/* all kmalloc0* routines will ensure that the memory requested is filled with
 * 0x0 */
void *kmalloc_a(uint32_t sz);

void *kmalloc_p(uint32_t sz, uint32_t *phys);

void *kmalloc_ap(uint32_t sz, uint32_t *phys);

#endif
