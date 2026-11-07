#ifndef BSP_H
#define BSP_H
#include "hal_status.h"
/* Leader owns BSP, startup, clock and FreeRTOSConfig.h. */
hal_status_t bsp_system_init(void);
_Noreturn void bsp_panic(void);
#endif
