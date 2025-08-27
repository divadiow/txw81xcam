
#ifndef __CSKY_SLEEP_H_
#define __CSKY_SLEEP_H_
#include "k_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OS_MS_PERIOD_TICK (1000/OS_SYSTICK_HZ)

#define os_jiffies_to_msecs(j) ((j)*OS_MS_PERIOD_TICK)
#define os_msecs_to_jiffies(m) ((m)/OS_MS_PERIOD_TICK)

#ifdef __cplusplus
}
#endif

#endif


