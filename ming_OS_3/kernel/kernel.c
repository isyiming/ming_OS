#include "../cpu/isr.h"
#include "../drivers/screen.h"
#include "kernel.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include <stdint.h>
#include "../libc/pmm.h"
#include "../libc/paging.h"

void kernel_main() {
    isr_install();
    irq_install();

    asm("int $2");
    asm("int $3");

    kprint("Type something, it will go through the kernel\n"
        "Type END to halt the CPU or PAGE to request a kmalloc()\n> ");

    kprint("kernel in memory start: ");      print_hex(kern_start);      kprint("\n");
    kprint("kernel in memory end  : ");      print_hex(kern_end);      kprint("\n");

    // init_pmm();
    // // 打印保存页表条目的地址空间
    // uint32_t *add=pmm_stack.PhysicalAddr;
    // kprint("pmm_stack.PhysicalAddr start address: "); print_hex(add); kprint("\n");
    // add=pmm_stack.topAddr;
    // kprint("pmm_stack.PhysicalAddr end address: "); print_hex(add); kprint("\n");


    //开启分页
    initialise_paging();
    clear_screen();

    kprint("Hello, paging world!\n");

    uint32_t *ptr = (uint32_t*)0x8000000;
    uint32_t do_page_fault = *ptr;
    print_int(do_page_fault);
}

void user_input(char *input) {
    if (strcmp(input, "END") == 0) {
        kprint("Stopping the CPU. Bye!\n");
        asm volatile("hlt");
    }
    else if (strcmp(input, "PAGE") == 0) {
        uint32_t phys_addr = pmm_alloc_page();
        kprint("physical address: ");
        print_hex(phys_addr);
        kprint("\n");

    }
    kprint("You said: ");
    kprintColor("You said: ",RED_ON_WHITE);

    kprint(input);
    kprint("\n> ");
}
