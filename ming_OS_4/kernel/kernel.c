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


    // Start multitasking.
    initialise_tasking();

    // Initialise the initial ramdisk, and set it as the filesystem root.
    fs_root = initialise_initrd(initrd_location);

    // Create a new process in a new address space which is a clone of this.
    int ret = fork();

    monitor_write("fork() returned ");
    monitor_write_hex(ret);
    monitor_write(", and getpid() returned ");
    monitor_write_hex(getpid());
    monitor_write("\n============================================================================\n");

    // The next section of code is not reentrant so make sure we aren't interrupted during.
    asm volatile("cli");
    // list the contents of /
    int i = 0;
    struct dirent *node = 0;
    while ( (node = readdir_fs(fs_root, i)) != 0)
    {
        monitor_write("Found file ");
        monitor_write(node->name);
        fs_node_t *fsnode = finddir_fs(fs_root, node->name);

        if ((fsnode->flags&0x7) == FS_DIRECTORY)
        {
            monitor_write("\n\t(directory)\n");
        }
        else
        {
            monitor_write("\n\t contents: \"");
            char buf[256];
            u32int sz = read_fs(fsnode, 0, 256, buf);
            int j;
            for (j = 0; j < sz; j++)
                monitor_put(buf[j]);

            monitor_write("\"\n");
        }
        i++;
    }
    monitor_write("\n");

    asm volatile("sti");
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
