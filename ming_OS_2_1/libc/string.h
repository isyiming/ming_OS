#ifndef STRINGS_H
#define STRINGS_H

#include <stdint.h>
#include <stddef.h>
#include "../libc/stdarg.h"

void int_to_ascii(int n, char str[]);
void hex_to_ascii(int n, char str[]);
void reverse(char s[]);
int strlen(char s[]);
void backspace(char s[]);
void append(char s[], char n);
int strcmp(char s1[], char s2[]);

int vprintf(const char *fmt, va_list ap);

#endif
