#ifndef _OHOS_DEFS_H_
#define _OHOS_DEFS_H_
#include "los_interrupt.h"
#include "los_debug.h"

#ifdef __cplusplus
extern "C" {
#endif

#define osWaitForever     (-1)

#define ASSERT LOS_ASSERT

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef __inline
#define __inline          inline
#endif

#ifndef __weak
#define __weak            __attribute__((weak))
#endif

#ifndef __alias
#define __alias(f)        __attribute__((alias(#f)))
#endif

#ifndef __at_section
#define __at_section(sec) __attribute__((section(sec)))
#endif

#ifndef __used
#define __used __attribute__((used))
#endif

#ifndef __aligned
#define __aligned(n)      __attribute__((aligned(n)))
#endif

#ifndef __packed
#define __packed          __attribute__((packed))
#endif

#ifndef __init
//#define __init            __at_section("INIT.TXT")
#define __init
#endif

#ifndef __initdata
//#define __initdata        __at_section("INIT.DAT")
#define __initdata
#endif

#ifndef __initconst
//#define __initconst       __at_section("INIT.RO")
#define __initconst
#endif

#ifndef __rom
//#define __rom            __at_section(".rom.text")
#define __rom
#endif

#ifndef __romro
//#define __romro          __at_section(".rom.ro")
#define __romro
#endif

#ifndef __ram
//#define __ram            __at_section(".ram.text")
#define __ram
#endif

#ifndef __bobj
//#define __bobj            __at_section(".bobj")
#define __bobj
#endif

#ifndef __dsleep_text
//#define __dsleep_text     __at_section(".dsleep.text")
#define __dsleep_text
#endif

#ifndef __dsleep_date
//#define __dsleep_date     __at_section(".dsleep.data")
#define __dsleep_date
#endif

#ifndef NULL
#define NULL (void *)0
#endif

#ifndef BIT
#define BIT(a) (1UL << (a))
#endif

#ifndef offsetof
#define offsetof(type, member) ((long) &((type *) 0)->member)
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({      \
        void *__mptr = (void *)(ptr);                   \
        (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

#ifndef RETURN_ADDR
#define RETURN_ADDR() __builtin_return_address(0)
#endif

#ifndef __always_inline
#define __always_inline inline
#endif

#ifndef ASSERT_HOLDUP
#define ASSERT_HOLDUP 0
#endif

#ifndef ATCMD_ARGS_COUNT
#define ATCMD_ARGS_COUNT     (16)
#endif

#ifndef ATCMD_PRINT_BUF_SIZE
#define ATCMD_PRINT_BUF_SIZE (256)
#endif

static int inline min(int a, int b)
{
    return ((a)<(b)?(a):(b));
}
static int inline max(int a, int b)
{
    return ((a)>(b)?(a):(b));
}

#define cpu_dcache_disable() csi_dcache_clean_invalid(); csi_dcache_disable(); 
#define cpu_dcache_enable()  csi_dcache_enable()

#ifdef __cplusplus
}
#endif
#endif
