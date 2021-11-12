/* Simple command-line kernel monitor useful for
 * controlling the kernel and exploring the system interactively. */

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/env.h>
#include <inc/x86.h>
#include <inc/types.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/env.h>
#include <kern/trap.h>
#include <kern/kclock.h>

#define WHITESPACE "\t\r\n "
#define MAXARGS    16

/* Functions implementing monitor commands */
int mon_help(int argc, char **argv, struct Trapframe *tf);
int mon_kerninfo(int argc, char **argv, struct Trapframe *tf);
int mon_backtrace(int argc, char **argv, struct Trapframe *tf);

int mon_test_backtrace(int argc, char **argv, struct Trapframe *tf);
int mon_test_debug_info(int argc, char **argv, struct Trapframe *tf);

int mon_dumpcmos(int argc, char **argv, struct Trapframe *tf);


struct Command {
    const char *name;
    const char *desc;
    /* return -1 to force monitor to exit */
    int (*func)(int argc, char **argv, struct Trapframe *tf);
};

static struct Command commands[] = {
        {"help", "Display this list of commands", mon_help},
        {"kerninfo", "Display information about the kernel", mon_kerninfo},
        {"backtrace", "Print stack backtrace", mon_backtrace},

        {"test_backtrace", "Print stack backtrace after recursive function", mon_test_backtrace},
        {"test_debug_info", "Test procedure of getting debug line info", mon_test_debug_info},

        {"dumpcmos", "Print CMOS contents", mon_dumpcmos},

};
#define NCOMMANDS (sizeof(commands) / sizeof(commands[0]))

/* Implementations of basic kernel monitor commands */

int
mon_help(int argc, char **argv, struct Trapframe *tf) {
    for (size_t i = 0; i < NCOMMANDS; i++)
        cprintf("%s - %s\n", commands[i].name, commands[i].desc);
    return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf) {
    extern char _head64[], entry[], etext[], edata[], end[];

    cprintf("Special kernel symbols:\n");
    cprintf("  _head64 %16lx (virt)  %16lx (phys)\n", (unsigned long)_head64, (unsigned long)_head64);
    cprintf("  entry   %16lx (virt)  %16lx (phys)\n", (unsigned long)entry, (unsigned long)entry - KERN_BASE_ADDR);
    cprintf("  etext   %16lx (virt)  %16lx (phys)\n", (unsigned long)etext, (unsigned long)etext - KERN_BASE_ADDR);
    cprintf("  edata   %16lx (virt)  %16lx (phys)\n", (unsigned long)edata, (unsigned long)edata - KERN_BASE_ADDR);
    cprintf("  end     %16lx (virt)  %16lx (phys)\n", (unsigned long)end, (unsigned long)end - KERN_BASE_ADDR);
    cprintf("Kernel executable memory footprint: %luKB\n", (unsigned long)ROUNDUP(end - entry, 1024) / 1024);
    return 0;
}

struct StackFrameFooter;

struct StackFrameFooter
{
    struct StackFrameFooter *next;
    uintptr_t ret;
};

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf) {
    cprintf("Stack backtrace:\n");
    struct StackFrameFooter *frame = (struct StackFrameFooter*)read_rbp();

    //int res = debuginfo_rip((uintptr_t)&mon_help, &info);

    while (frame != NULL)
    {
        struct Ripdebuginfo info = {};
        int res = debuginfo_rip(frame->ret, &info);
        cprintf("  rbp %016lx  rip %016lx\n", (uintptr_t)frame, frame->ret);
        if (!res)
            cprintf("    %s:%d: %s+%ld\n", info.rip_file, info.rip_line, info.rip_fn_name, frame->ret - info.rip_fn_addr);
        //cprintf("    res = %d, line = %d\n", res, info.rip_line);
        frame = frame->next;
    }

    return 0;
}

int mon_test_debug_info(int argc, char **argv, struct Trapframe *tf)
{
    struct Ripdebuginfo info = {};
    int res = debuginfo_rip((uintptr_t)&mon_test_debug_info, &info);
    cprintf("res = %d line = %d\n", res, info.rip_line);
    return 0;
}

static void test_mon_backtrace(uint32_t depth)
{
    if (depth == 0)
    {
        mon_backtrace(0, 0, NULL);
        return;
    }
    test_mon_backtrace(--depth);
}

int
mon_test_backtrace(int argc, char **argv, struct Trapframe *tf) {
    if (argc < 2)
    {
        cprintf("Expected recursion depth\n");
        return 0;
    }
    uint32_t depth = 0;
    for (const char *s = argv[1]; '0' <= *s && *s <= '9'; ++s)
        depth = 10 * depth + (*s - '0');
    cprintf("depth = %u\n", depth);

    test_mon_backtrace(depth);

    return 0;
}

int
mon_dumpcmos(int argc, char **argv, struct Trapframe *tf) {
    // Dump CMOS memory in the following format:
    // 00: 00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF
    // 10: 00 ..
    // Make sure you understand the values read.
    // Hint: Use cmos_read8()/cmos_write8() functions.
    // LAB 4: Your code here

    static const int CMOS_BYTES = 128;
    static const int BYTES_IN_LINE = 16;

    for (int i = 0; i < CMOS_BYTES; i += BYTES_IN_LINE)
    {
        cprintf("%02x:", i);
        for (int j = 0; j < BYTES_IN_LINE; ++j)
        {
            uint8_t data = cmos_read8(i + j);
            cprintf(" %02x", data);
        }
        cputchar('\n');
    }

    return 0;
}

/* Kernel monitor command interpreter */

static int
runcmd(char *buf, struct Trapframe *tf) {
    int argc = 0;
    char *argv[MAXARGS];

    argv[0] = NULL;

    /* Parse the command buffer into whitespace-separated arguments */
    for (;;) {
        /* gobble whitespace */
        while (*buf && strchr(WHITESPACE, *buf)) *buf++ = 0;
        if (!*buf) break;

        /* save and scan past next arg */
        if (argc == MAXARGS - 1) {
            cprintf("Too many arguments (max %d)\n", MAXARGS);
            return 0;
        }
        argv[argc++] = buf;
        while (*buf && !strchr(WHITESPACE, *buf)) buf++;
    }
    argv[argc] = NULL;

    /* Lookup and invoke the command */
    if (!argc) return 0;
    for (size_t i = 0; i < NCOMMANDS; i++) {
        if (strcmp(argv[0], commands[i].name) == 0)
            return commands[i].func(argc, argv, tf);
    }

    cprintf("Unknown command '%s'\n", argv[0]);
    return 0;
}

void
monitor(struct Trapframe *tf) {

    cprintf("Welcome to the JOS kernel monitor!\n");
    cprintf("Type 'help' for a list of commands.\n");

    if (tf) print_trapframe(tf);

    char *buf;
    do buf = readline("K> ");
    while (!buf || runcmd(buf, tf) >= 0);
}
