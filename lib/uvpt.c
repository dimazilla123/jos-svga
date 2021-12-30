/* User virtual page table helpers */

#include <inc/lib.h>
#include <inc/mmu.h>

extern volatile pte_t uvpt[];     /* VA of "virtual page table" */
extern volatile pde_t uvpd[];     /* VA of current page directory */
extern volatile pdpe_t uvpdp[];   /* VA of current page directory pointer */
extern volatile pml4e_t uvpml4[]; /* VA of current page map level 4 */

pte_t
get_uvpt_entry(void *va) {
    if (!(uvpml4[VPML4(va)] & PTE_P)) return uvpml4[VPML4(va)];
    if (!(uvpdp[VPDP(va)] & PTE_P) || (uvpdp[VPDP(va)] & PTE_PS)) return uvpdp[VPDP(va)];
    if (!(uvpd[VPD(va)] & PTE_P) || (uvpd[VPD(va)] & PTE_PS)) return uvpd[VPD(va)];
    return uvpt[VPT(va)];
}

int
get_prot(void *va) {
    pte_t pte = get_uvpt_entry(va);
    int prot = pte & PTE_AVAIL & ~PTE_SHARE;
    if (pte & PTE_P) prot |= PROT_R;
    if (pte & PTE_W) prot |= PROT_W;
    if (!(pte & PTE_NX)) prot |= PROT_X;
    if (pte & PTE_SHARE) prot |= PROT_SHARE;
    return prot;
}

bool
is_page_dirty(void *va) {
    pte_t pte = get_uvpt_entry(va);
    return pte & PTE_D;
}

bool
is_page_present(void *va) {
    return get_uvpt_entry(va) & PTE_P;
}

int
foreach_shared_region(int (*fun)(void *start, void *end, void *arg), void *arg) {
    /* Calls fun() for every shared region */
    // LAB 11: Your code here
    for (int pml4ei = 0; pml4ei < PML4_ENTRY_COUNT; pml4ei++) {
        uintptr_t pml4_addr = (uintptr_t)MAKE_ADDR(pml4ei, 0, 0, 0, 0);
        pml4e_t pml4e = uvpml4[VPML4(pml4_addr)];
        if (!(pml4e & PTE_P)) {
            continue;
        }
        for (int pdpei = 0; pdpei < PDP_ENTRY_COUNT; pdpei++) {
            uintptr_t pdpe_addr = (uintptr_t)MAKE_ADDR(pml4ei, pdpei, 0, 0, 0);
            pdpe_t pdpe = uvpdp[VPDP(pdpe_addr)];
            if (!(pdpe & PTE_P)) {
                continue;
            }
            if (pdpe & PTE_PS) {
                if (pdpe & PTE_SHARE) {
                    fun((void*)pdpe_addr, (void*)(pdpe_addr + 1024 * 1024 * 1024), arg); 
                }
                continue;
            }
            for (int pdei = 0; pdei < PD_ENTRY_COUNT; pdei++) {
                uintptr_t pde_addr = (uintptr_t)MAKE_ADDR(pml4ei, pdpei, pdei, 0, 0);
                pde_t pde = uvpd[VPD(pde_addr)];
                if (!(pde & PTE_P)) {
                    continue;
                }
                if (pde & PTE_PS) {
                    if (pde & PTE_SHARE) {
                        fun((void*)pde_addr, (void*)(pde_addr + 1024 * 1024 * 2), arg);
                    }
                    continue;
                }
                for (int ptei = 0; ptei < PT_ENTRY_COUNT; ptei++) {
                    uintptr_t pte_addr = (uintptr_t)MAKE_ADDR(pml4ei, pdpei, pdei, ptei, 0);
                    pte_t pte = uvpt[VPT(pte_addr)];
                    if ((pte & PTE_P) && (pte & PTE_SHARE)) {
                        fun((void*)pte_addr, (void*)(pte_addr + PAGE_SIZE), arg);
                    }
                }
            }
        }
    }
    
    return 0;
}
