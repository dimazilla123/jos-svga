; @file
; Copyright (c) 2020, ISP RAS. All rights reserved.
; SPDX-License-Identifier: BSD-3-Clause

#define CR0_PE 0x00000001 ; Protection Enable
#define CR0_MP 0x00000002 ; Monitor coProcessor
#define CR0_EM 0x00000004 ; Emulation
#define CR0_TS 0x00000008 ; Task Switched
#define CR0_ET 0x00000010 ; Extension Type
#define CR0_NE 0x00000020 ; Numeric Errror
#define CR0_WP 0x00010000 ; Write Protect
#define CR0_AM 0x00040000 ; Alignment Mask
#define CR0_NW 0x20000000 ; Not Writethrough
#define CR0_CD 0x40000000 ; Cache Disable
#define CR0_PG 0x80000000 ; Paging

#define CR4_VME        0x00000001 ; V86 Mode Extensions
#define CR4_PVI        0x00000002 ; Protected-Mode Virtual Interrupts
#define CR4_TSD        0x00000004 ; Time Stamp Disable
#define CR4_DE         0x00000008 ; Debugging Extensions
#define CR4_PSE        0x00000010 ; Page Size Extensions
#define CR4_PAE        0x00000020 ; Physical address extension
#define CR4_MCE        0x00000040 ; Machine Check Enable
#define CR4_PGE        0x00000080 ; Page Global Enabled
#define CR4_PCE        0x00000100 ; Performance counter enable
#define CR4_OSFXSR     0x00000200 ; FXSAVE/FXRSTOR/SSE enable
#define CR4_OSXMMEXCPT 0x00000400 ; Enable SSE exceptions
#define CR4_UMIP       0x00000800 ; User-Mode Instruction Prevention
#define CR4_LA57       0x00001000 ; Level-5 paging
#define CR4_VMXE       0x00002000 ; Virtual Machine Extensions Enable
#define CR4_SMXE       0x00004000 ; Safer Mode Extensions Enable
#define CR4_FSGSBASE   0x00010000 ; Enable FSGSBASE instructions
#define CR4_PCIDE      0x00020000 ; PCID Enable
#define CR4_OSXSAVE    0x00040000 ; XSAVE Enable
#define CR4_SMEP       0x00100000 ; SMEP Enable
#define CR4_SMAP       0x00200000 ; SMAP Enable
#define CR4_PKE        0x00400000 ; Protected Key Enable

#define EFER_MSR 0xC0000080
#define EFER_LME (1ULL << 8)
#define EFER_LMA (1ULL << 10)
#define EFER_NXE (1ULL << 11)

bits 32

SECTION .text
    align 4

%macro GDT_DESC 2
    dw 0xFFFF, 0
    db 0, %1, %2, 0
%endmacro

GDT_BASE:
    dq  0x0             ; NULL segment
LINEAR_CODE_SEL:        equ $ - GDT_BASE
    GDT_DESC 0x9A, 0xCF
LINEAR_DATA_SEL:        equ $ - GDT_BASE
    GDT_DESC 0x92, 0xCF
LINEAR_CODE64_SEL:      equ $ - GDT_BASE
    GDT_DESC 0x9A, 0xAF
LINEAR_DATA64_SEL:      equ $ - GDT_BASE
    GDT_DESC 0x92, 0xCF

GDT_DESCRIPTOR:
    dw 0x28 - 1 
    dd GDT_BASE
    dd 0x0

KERNEL_ENTRY:
    dd 0

LOADER_PARAMS:
    dd 0
    
PAGE_TABLE:
    dd 0

global ASM_PFX(IsCpuidSupportedAsm)
ASM_PFX(IsCpuidSupportedAsm):
    ; Store original EFLAGS for later comparison.
    pushf
    ; Store current EFLAGS.
    pushf
    ; Invert the ID bit in stored EFLAGS.
    xor dword [esp], 0x200000
    ; Load stored EFLAGS (with ID bit inverted).
    popf
    ; Store EFLAGS again (ID bit may or may not be inverted).
    pushf
    ; Read modified EFLAGS (ID bit may or may not be inverted).
    pop eax
    ; Enable bits in RAX to whichver bits in EFLAGS were changed.
    xor eax, [esp]
    ; Restore stack pointer.
    popf
    ; Leave only the ID bit EFLAGS change result in RAX.
    and eax, 0x200000
    ; Shift it to the lowest bit be boolean compatible.
    shr eax, 21
    ; Return.
    ret

global ASM_PFX(CallKernelThroughGateAsm)
ASM_PFX(CallKernelThroughGateAsm):
    ; Transitioning from protected mode to long mode is described in Intel SDM
    ; 9.8.5 Initializing IA-32e Mode. More detailed explanation why paging needs
    ; to be disabled is explained in 4.1.2 Paging-Mode Enabling.

    ; Disable interrupts.
    cli

    ; Drop return pointer as we no longer need it.
    pop ecx

    ; Save kernel entry point passed by the bootloader.
    pop ecx
    mov eax, KERNEL_ENTRY
    mov [eax], ecx

    ; Save loading params address passed by the bootloader.
    pop ecx
    mov eax, LOADER_PARAMS
    mov [eax], ecx

    ; Save identity page table passed by the bootloader.
    pop ecx
    mov eax, PAGE_TABLE
    mov [eax], ecx

    ; 1. Disable paging.
    mov eax, cr0
    and eax, ~CR0_PG
    mov cr0, eax

    ; 2. Switch to our GDT that supports 64-bit mode and update CS to LINEAR_CODE_SEL.
    ; LAB 2: Your code here:
    ;push GDT_BASE
    ;lgdt [cs:esp]
    lgdt [GDT_DESCRIPTOR]
    jmp LINEAR_CODE_SEL:AsmWithOurGdt

AsmWithOurGdt:

    ; 3. Reset all the data segment registers to linear mode (LINEAR_DATA_SEL).
    ; LAB 2: Your code here:
    mov eax, LINEAR_CODE_SEL
    mov ds, eax
    mov ss, eax
    mov es, eax
    mov fs, eax
    mov gs, eax

    ; 4. Enable PAE/PGE in CR4, which is required to transition to long mode.
    ; This may already be enabled by the firmware but is not guaranteed.
    ; LAB 2: Your code here:
    mov eax, cr4
    or eax, CR4_PAE
    mov cr4, eax

    ; 5. Update page table address register (C3) right away with the supplied PAGE_TABLE.
    ; This does nothing as paging is off at the moment as paging is disabled.
    ; LAB 2: Your code here:
    mov eax, [PAGE_TABLE]
    mov cr3, eax

    ; 6. Enable long mode (LME) and execute protection (NXE) via the EFER MSR register.
    ; LAB 2: Your code here:
    mov ecx, EFER_MSR
    rdmsr
    or eax, EFER_NXE & EFER_LME
    wrmsr

    ; 7. Enable paging as it is required in 64-bit mode.
    ; LAB 2: Your code here:
    mov eax, cr0
    or eax, CR0_PG
    mov cr0, eax

    ; 8. Transition to 64-bit mode by updating CS with LINEAR_CODE64_SEL.
    ; LAB 2: Your code here:
    jmp LINEAR_CODE64_SEL:AsmInLongMode

AsmInLongMode:
    BITS 64

    ; 9. Reset all the data segment registers to linear 64-bit mode (LINEAR_DATA64_SEL).
    ; LAB 2: Your code here:

    mov rax, LINEAR_DATA64_SEL
    mov ds, rax
    mov ss, rax
    mov es, rax,
    mov fs, rax,
    mov gs, rax

    ; 10. Jump to the kernel code.
    mov ecx, [REL LOADER_PARAMS]
    mov ebx, [REL KERNEL_ENTRY]
    jmp rbx

noreturn:
    hlt
    jmp noreturn
