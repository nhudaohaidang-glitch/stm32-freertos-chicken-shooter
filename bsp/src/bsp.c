#include "bsp.h"
hal_status_t bsp_system_init(void)
{
    /* TODO(leader): configure and validate clock/pins on the actual board.
     * Explicit failure prevents the scaffold from pretending to be a game. */
    return HAL_NOT_IMPLEMENTED;
}
_Noreturn void bsp_panic(void)
{
    __asm volatile ("cpsid i" ::: "memory");
    for (;;) { __asm volatile ("nop"); }
}
