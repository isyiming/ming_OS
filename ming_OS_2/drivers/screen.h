#ifndef SCREEN_H
#define SCREEN_H

#define VIDEO_ADDRESS 0xb8000
#define MAX_ROWS 25
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0f
#define RED_ON_WHITE 0xf4

/* Screen i/o ports */
#define REG_SCREEN_CTRL 0x3d4
#define REG_SCREEN_DATA 0x3d5
#include "../libc/string.h"

/* Public kernel API */
void clear_screen();
void kprint_at(char *message, int col, int row,char attr);
void kprint(char *message);
void kprintColor(char *message,char attr);//ming添加的指定颜色显示函数，但是我的mac和qemu前端不兼容。我看不到颜色。不知道这个颜色是否生效了
void kprint_backspace();
void print_int(int val);//屏幕打印int
void print_hex(uint32_t val);//屏幕打印hex

#endif
