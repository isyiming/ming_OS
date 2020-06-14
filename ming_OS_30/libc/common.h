#ifndef COMMON_H
#define COMMON_H
/*
 * common.h -- Defines typedefs and some global functions.
 * From JamesM's kernel development tutorials.
 */
// #include <null.h>
// #include <types.h>
// #include <__attributes__.h>

#include <stdint.h>
#include <stddef.h>
#include "mem.h"
#include "../cpu/isr.h"
#include "../drivers/screen.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include "../libc/stdarg.h"

#define NELEM(x)	(sizeof((x)) / sizeof((x)[0]))

void		outb(uint16_t port, uint8_t value); /* write a byte out to port */
uint8_t		inb(uint16_t port); /* read a byte out from port */
uint16_t	inw(uint16_t port); /* read two bytes out from port */


#define PANIC(s, ...)	_panic("%s:%u in %s: " s, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
void	_panic(const char *fmt, ...);

#define ASSERT(msg, cond) (                                       \
		(cond) ?                                           \
		(void)0 :                                          \
		PANIC("assertion failed: %s (%s)", (msg), (#cond)) \
)

void *	memset(void *b, int c, size_t len);
void	bzero(void *b, size_t len);
void *	memcpy(void *dest, const void *src, size_t count);


// #include <kmalloc.h>
// #include <freebsd.h>
#endif /* ndef COMMON_H */
