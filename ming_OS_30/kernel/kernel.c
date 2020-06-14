#include "../cpu/isr.h"
#include "../drivers/screen.h"
#include "kernel.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include <stdint.h>
#include "../libc/paging.h"

void kernel_main() {
    isr_install();
    irq_install();

    asm("int $2");
    asm("int $3");

    kprint("Type something, it will go through the kernel\n"
        "Type END to halt the CPU or PAGE to request a kmalloc()\n> ");

    // clear_screen();

    kprint("kernel in memory start: ");      print_hex(kern_start);      kprint("\n");
    kprint("kernel in memory end  : ");      print_hex(kern_end);      kprint("\n");


    //开启分页
    initialise_paging();
    kprint("Hello, paging world!\n");
    // uint32_t *ptr = (uint32_t*)0x1E000000;
    // uint32_t do_page_fault = *ptr;

}

void user_input(char *input) {
    if (strcmp(input, "END") == 0) {
        kprint("Stopping the CPU. Bye!\n");
        asm volatile("hlt");
    }
    else if (strcmp(input, "PAGE") == 0) {
        kprint("physical address: ");
        kprint("\n");

    }
    kprint("You said: ");
    kprintColor("You said: ",RED_ON_WHITE);

    kprint(input);
    kprint("\n> ");
}
