#ifndef __SDK_SYS_CONFIG_H__
#define __SDK_SYS_CONFIG_H__
#include "project_config.h"

#define SYS_CACHE_ENABLE    1

#define PROJECT_TYPE        PRO_TYPE_FPV
 
#ifndef SYSCFG_ENABLE
#define SYSCFG_ENABLE 1
#endif

#ifndef MPOOL_ALLOC
#define MPOOL_ALLOC 1
#endif

#ifndef TXWSDK_POSIX
#define TXWSDK_POSIX
#endif

#define __ram    __at_section(".ram.text")
#define __dsleep_text     __at_section(".dsleep.text")
#define __dsleep_date     __at_section(".dsleep.data")

#define SRAM_POOL_START   (srampool_start)
#define SRAM_POOL_SIZE    (srampool_end - srampool_start)
#define SYS_FACTORY_PARAM_SIZE          2048

#ifndef SYS_HEAP_SIZE
#define  SYS_HEAP_SIZE (SRAM_POOL_SIZE)
#endif

#ifndef WIFI_RX_BUFF_SIZE
#define  WIFI_RX_BUFF_SIZE (17*1024)
#endif

#ifndef SYS_HEAP_START
#define SYS_HEAP_START    (SRAM_POOL_START)
#endif

#ifndef SKB_POOL_SIZE
#define SKB_POOL_SIZE     (30*1024)
#endif

//#define WIFI_SINGLE_DEV

#ifndef DEFAULT_SYS_CLK
#define DEFAULT_SYS_CLK   240000000UL //options: <= 180Mhz
#endif

#ifndef ATCMD_UARTDEV
#define ATCMD_UARTDEV     HG_UART0_DEVID
#endif

#define LWIP_NETIF_REMOVE_CALLBACK      1
#define TCPIP_THREAD_PRIO               OS_TASK_PRIORITY_BELOW_NORMAL+2

#ifdef PSRAM_HEAP
#define TCPIP_MBOX_SIZE                 128
#define DEFAULT_UDP_RECVMBOX_SIZE       64
#define DEFAULT_TCP_RECVMBOX_SIZE       64
#define DEFAULT_ACCEPTMBOX_SIZE         16

//#define MEM_LIBC_MALLOC 1
//#define MEMP_MEM_MALLOC 1 
#define MEM_SIZE                        80*1024
#define MEMP_NUM_PBUF                   40
#define MEMP_NUM_NETCONN                16
#define MEMP_NUM_NETBUF                 64
#define MEMP_NUM_UDP_PCB                8
#define MEMP_NUM_TCP_PCB                16
#define MEMP_NUM_TCP_SEG                320
#define PBUF_POOL_SIZE                  80

#define TCP_SND_BUF                    (40 * TCP_MSS)
#define TCP_WND                        (40 * TCP_MSS)
#endif

#ifndef DCDC_EN
#define DCDC_EN                         0           //是否使能DCDC功能。选错会导致芯片重启
#endif

#ifndef WIFI_FEM_CHIP
#define WIFI_FEM_CHIP                   LMAC_FEM_NONE   //FEM芯片类型。LMAC_FEM_NONE以外的值会进行对应的FEM初始化
#endif

#ifndef WIFI_TEMPERATURE_COMPESATE_EN
#define WIFI_TEMPERATURE_COMPESATE_EN   1           //是否使能温度补偿
#endif

#ifndef WIFI_FREQ_OFFSET_TRACK_MODE
#define WIFI_FREQ_OFFSET_TRACK_MODE     LMAC_FREQ_OFFSET_TRACK_ALWAYS_ON    //默认一直打开频偏跟踪功能
#endif

#ifndef WIFI_HIGH_PRIORITY_TX_MODE_EN
#define WIFI_HIGH_PRIORITY_TX_MODE_EN   0           //是否使能无线高优线发送模式
#endif

#ifndef WIFI_MODULE_80211W_EN
#define WIFI_MODULE_80211W_EN           1           //是否使能802.11w功能
#endif

#ifndef BLE_UUID_128
#define BLE_UUID_128 1
#endif

#ifndef BLE_METER_TEST_EN
#define BLE_METER_TEST_EN              0           //是否使能BLE量产测试命令 
#endif

//---dsleep test start------
#ifndef WIFI_PSALIVE_SUPPORT
#define WIFI_PSALIVE_SUPPORT 0
#endif

#ifndef SYS_APP_DSLEEP_TEST
#define SYS_APP_DSLEEP_TEST 0
#endif

#ifndef SYS_APP_UDPKALIVE_TEST
#define SYS_APP_UDPKALIVE_TEST 0
#endif
//---sleep test end------

#endif

