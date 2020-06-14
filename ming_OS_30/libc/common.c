/*
 * common.c -- Defines some global functions.
 * From JamesM's kernel development tutorials.
 */
#include "../libc/common.h"

void
outb(uint16_t port, uint8_t value)
{
	asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}


uint8_t
inb(uint16_t port)
{
	uint8_t ret;

	asm volatile ("inb %1, %0" : "=a" (ret) : "dN" (port));
	return (ret);
}


uint16_t
inw(uint16_t port)
{
	uint16_t ret;

	asm volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
	return (ret);
}


void *
memset(void *b, int c, size_t len)
{
	char *bb;

	for (bb = b; len > 0; len--)
		*bb++ = c;

	return (b);
}


void
bzero(void *b, size_t len)
{

	(void)memset(b, 0, len);
}


void *
memcpy(void *dest, const void *src, size_t count)
{
	char* dst8 = (char *)dest;
	char* src8 = (char *)src;

	while (count--)
		*dst8++ = *src8++;

	return dest;
}


void
_panic(const char *fmt, ...)
{
	va_list ap;

	kprint("\n***** Kernel panic! *****\n");


	va_start(ap, fmt);
	(void)vprintf(fmt, ap);
	va_end(ap);

	kprint("\n*************************\n");

	for (;;)
		;
	/* NOTREACHED */
}
