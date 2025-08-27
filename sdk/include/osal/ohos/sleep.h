
#ifndef __CSKY_SLEEP_H_
#define __CSKY_SLEEP_H_
#include "los_tick.h"

#ifdef __cplusplus
extern "C" {
#endif

#define os_jiffies_to_msecs(j) LOS_Tick2MS(j)
#define os_msecs_to_jiffies(m) LOS_MS2Tick(m)

#ifdef __cplusplus
}
#endif

#endif


