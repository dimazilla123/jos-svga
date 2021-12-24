/* implement fork from user space */

#include <inc/string.h>
#include <inc/lib.h>

/* User-level fork with copy-on-write.
 * Create a child.
 * Lazily copy our address space and page fault handler setup to the child.
 * Then mark the child as runnable and return.
 *
 * Returns: child's envid to the parent, 0 to the child, < 0 on error.
 * It is also OK to panic on error.
 *
 * Hint:
 *   Use sys_map_region, it can perform address space copying in one call
 *   Remember to fix "thisenv" in the child process.
 */
envid_t
fork(void) {
    // LAB 9: Your code here

    envid_t child = sys_exofork();
    if (child < 0)
        return child;

    if (child == 0)
    {
        thisenv = &envs[ENVX(sys_getenvid())];
        return 0;
    }

    int res = sys_map_region(
                            0,
                            NULL,
                            child,
                            NULL,
                            MAX_USER_ADDRESS,
                            PROT_ALL | PROT_LAZY | PROT_COMBINE
                            );
    if (res < 0)
        goto child_destroy;

    res = sys_env_set_pgfault_upcall(
                                     child,
                                    envs[ENVX(sys_getenvid())].env_pgfault_upcall);
    if (res < 0)
        goto child_destroy;

    res = sys_env_set_status(child, ENV_RUNNABLE);
    if (res < 0)
        goto child_destroy;

    return child;

child_destroy:;
    
    int res_chd = sys_env_destroy(child);
    if (res_chd < 0)
        panic("Fork double fault");

    return res;
}

envid_t
sfork() {
    panic("sfork() is not implemented");
}
