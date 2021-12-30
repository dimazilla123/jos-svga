/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_TRAP_H
#define JOS_KERN_TRAP_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/trap.h>
#include <inc/mmu.h>

/* The kernel's interrupt descriptor table */
extern struct Gatedesc idt[];
extern struct Pseudodesc idt_pd;

extern bool in_page_fault;

extern void clock_thdlr();
extern void timer_thdlr();

extern void  divide_thdlr();
extern void   debug_thdlr();
extern void     nmi_thdlr();
extern void   brkpt_thdlr();
extern void   oflow_thdlr();
extern void   bound_thdlr();
extern void   illop_thdlr();
extern void  device_thdlr();
extern void  dblflt_thdlr();
extern void     tss_thdlr();
extern void   segnp_thdlr();
extern void   stack_thdlr();
extern void   gpflt_thdlr();
extern void   pgflt_thdlr();
extern void   fperr_thdlr();
extern void   align_thdlr();
extern void    mchk_thdlr();
extern void simderr_thdlr();

extern void kbd_thdlr();
extern void serial_thdlr();

extern void syscall_thdlr();

void clock_idt_init(void);
void trap_init(void);
void trap_init_percpu(void);
void print_regs(struct PushRegs *regs);
void print_trapframe(struct Trapframe *tf);

#endif /* JOS_KERN_TRAP_H */
