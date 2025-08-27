#include "sys_config.h"
#include "typesdef.h"
#ifdef CSKY_OS
#include "csi_config.h"
#include "soc.h"
#include "csi_core.h"
#include "csi_kernel.h"
#endif
#ifdef OHOS
#include "los_task.h"
#endif
#include "txw81x/ticker.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/task.h"
#include "osal/work.h"
#include "devid.h"
#include "lib/heap/sysheap.h"
#include "lib/lmac/lmac.h"
#include "lib/common/sysevt.h"
#include "lib/common/common.h"
#include "dev/xspi/hg_xspi_psram.h"
#include "psram_cfg.h"
#ifdef CONFIG_SLEEP
#include "lib/common/dsleepdata.h"
#endif

extern int  main(void);
extern int  dev_init(void);
extern void device_init(void);
extern void set_psram_status(uint8_t res);
extern uint8_t get_psram_status();

extern int32_t g_top_irqstack;
extern uint32_t __heap_start;
extern uint32_t __heap_end;
extern uint32_t __psram_heap_start;
extern uint32_t __psram_heap_end;
extern struct os_workqueue main_wkq;
extern uint32 *sysvar_mgr;
extern uint8_t assert_holdup;

const uint32 g_DEFAULT_SYS_CLK = DEFAULT_SYS_CLK;
const uint32 g_SYS_CACHE_ENABLE = SYS_CACHE_ENABLE;

uint32 srampool_start = 0;
uint32 srampool_end   = 0;
uint32 psrampool_start = 0;
uint32 psrampool_end   = 0;
uint32 __sp_save[3];

#ifndef SYS_FACTORY_PARAM_SIZE
#define SYS_FACTORY_PARAM_SIZE 4
#endif
const uint16_t __used sys_factory_param[SYS_FACTORY_PARAM_SIZE / 2] __at_section("SYS_PARAM") = {SYS_FACTORY_PARAM_SIZE};

//为了不同工程,所以这里写一个空函数,即使没有save_boot_loader_addr也不会编译出错
__init __weak void save_boot_loader_addr() {};
    

__SYS_INIT void cache_open(void)
{
    sysctrl_cache_en();

    csi_cache_reset_profile();
    csi_cache_set_range(0, (uint32_t) & (__Vectors), CACHE_CRCR_8M, 0x1);
#if defined(PSRAM_HEAP)
    csi_cache_set_range(1, PSRAM_BASE, CACHE_CRCR_16M, 0x1);
#endif
    csi_cache_enable_profile();
    csi_dcache_enable();
}

__SYS_INIT __weak void system_clock_init(void)
{
    if (!sysctrl_cmu_sysclk_set(DEFAULT_SYS_CLK, 1)) {
        while (1);
    }
}

void irq_enable(uint32 irq)
{
    NVIC_EnableIRQ(irq);
}
void irq_disable(uint32 irq)
{
    NVIC_DisableIRQ(irq);
}
uint32 disable_irq(void)
{
    return __disable_irq();
}
void enable_irq(uint32 flag)
{
    if(!flag) __enable_irq();
}

int32 hg_qspi_flash_xip_mode(void *hw1, uint32 xip_en) {
    return RET_OK; 
}

/**
  * @retval : wp_en_last 
  */
int32 hg_qspi_flash_protect(void *hw1, uint32 wp_en)  {
    int32 wp_en_last = 0;
    return wp_en_last; 
}

/** @param
  *     pack = 0x0C : P25Q16SH, DC = CR1[2]  
  *            0x1C : GD25Q16E, DC = SR2[4] 
  *            0xFF : auto detect for TWX817-824
  * @retval
  *     RET_ERR or last hs status
  */
int32 hg_qspi_flash_hs_enable(void *hw1, uint32 pack) {
    int32 hs_en_last = 0;
    return hs_en_last; 
}

__SYS_INIT void SystemInit(void)
{
    srampool_start  = (uint32)&__heap_start;
    srampool_end    = (uint32)&__heap_end;
    psrampool_start = (uint32)&__psram_heap_start;
    psrampool_end   = (uint32)&__psram_heap_end;

    extern uint32 g_intstackbase;
    *((int *)((uint32)&g_intstackbase)) = 0xDEADBEEF;

    __set_VBR((uint32_t) & (__Vectors));

    SYSCTRL_REG_OPT_INIT();

#if  SYS_CACHE_ENABLE
    cache_open();
#endif
    
    sys_reset_detect();
    sys_reset_pending_clr();

    sysctrl_efuse_pwron_init();
    
//    if (sysctrl_get_softreset_pending()) {
//        sysctrl_clr_softreset_pending();
//        pmu_set_direct_run_pengding2();
//    } else {
//        pmu_clr_direct_run_pengding2();
//    }

    pmu_boot_direct_run_dis();
    SET_SKIP_PSRAM_SIGN(0);

    pmu_clr_deadcode_pending();

    mcu_watchdog_feed();
    mcu_watchdog_timeout_level(4);

//    sysctrl_err_resp_disable();
    sysctrl_cmu_init();

#ifndef FPGA_SUPPORT
    system_clock_init();
#endif

    void hg_xspi_sample_offset(uint32 ospi_base, int8 rx_offset, int8 tx_offset);
    hg_xspi_sample_offset((uint32)HG_OSPI_BASE, -2, 0);
    pmu_vdd_core_set(3);

#if defined(CONFIG_SEPARATE_IRQ_SP) && !defined(CONFIG_KERNEL_NONE)
    /* 801 not supported */
    __set_Int_SP((uint32_t)&g_top_irqstack);
    __set_CHR(__get_CHR() | CHR_ISE_Msk);
    VIC->TSPR = 0xFF;
#endif

    /* Clear active and pending IRQ */
    csi_vic_disable_all_irq();
    csi_vic_clear_all_pending_irq();
    csi_vic_clear_all_active();
    /* All peripheral interrupt priority is set to lowest */
    for (uint32 i = 0; i < IRQ_NUM; i++) {
        csi_vic_set_prio(i, 7);
    }

#ifdef CONFIG_KERNEL_NONE
    __enable_excp_irq();
#endif

    csi_coret_config(system_clock_get() / OS_SYSTICK_HZ, CORET_IRQn);    //1ms
#ifndef CONFIG_KERNEL_NONE
    csi_vic_enable_irq(CORET_IRQn);
#endif

    request_irq(LVD_IRQn, lvd_irq_handler, 0);
    irq_enable(LVD_IRQn);

    #ifndef CONFIG_SLEEP
        sysctrl_gpio_funcmap_default();
    #else
		int32 is_dsleep_wakeup(void);
        if(!is_dsleep_wakeup()) sysctrl_gpio_funcmap_default(); 
    #endif
    
    pmu_set_deadcode_pending();
    mcu_watchdog_feed();
    mcu_watchdog_timeout_level(8);
}

#if MPOOL_ALLOC
__init void malloc_init(void)
{
    uint32 flags = 0;
#ifdef MEM_TRACE
    flags |= SYSHEAP_FLAGS_MEM_LEAK_TRACE | SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK;
#endif
    sram_heap.name = "sram";
    sysheap_init(&sram_heap, (void *)SYS_HEAP_START, SYS_HEAP_SIZE, flags);
}

__init void malloc_psram_init(void)
{
#ifdef PSRAM_HEAP
    uint32 flags = SYSHEAP_FLAGS_MEM_ALIGN_16;
#ifdef MEM_TRACE
    flags |= SYSHEAP_FLAGS_MEM_LEAK_TRACE | SYSHEAP_FLAGS_MEM_OVERFLOW_CHECK;
#endif
    psram_heap.name = "psram";
	sysheap_init(&psram_heap, (void *)psrampool_start, psrampool_end - psrampool_start, flags);
#endif
}
void ota_msg_show()
{
    os_printf("[OTA_MSG] OTA_NUM:%d\tota_version:%d\trun_addr:%X\n",pmu_get_boot_code_pos(),get_boot_svn_version(),get_boot_loader_addr());
}

#endif

__init void pre_main(void)
{
    sdk_version = SDK_VERSION;
    svn_version = SVN_VERSION;
    app_version = APP_VERSION;
    assert_holdup = ASSERT_HOLDUP;
    save_boot_loader_addr();

	uint8_t res;
    if(NOW_PSRAM > PSRAM_DEF(E_PSRAM)) {
        //printf("now_psram:%d\t%d\n",NOW_PSRAM,PSRAM_DEF(E_PSRAM));
		res = psram_init(APS1604M_3SQR, (60*1000000),512);
        //外部psram初始化,所以需要配置对应的状态
        set_psram_status(res);
    } else {
        //is not ready?
        if(!get_psram_status()) {
            res = psram_auto_init();
            set_psram_status(res);
        }
    }

#if MPOOL_ALLOC
    malloc_init();
	malloc_psram_init();
#endif

#ifdef CSKY_OS
    csi_kernel_init();
#endif

#ifdef OHOS
    LOS_KernelInit();
#endif

    dev_init();
    device_init();

    
#ifdef CONFIG_SLEEP
    sys_sleepcb_init();
#endif
#ifdef CONFIG_SLEEP
    sys_sleepdata_init();
#endif
    
    sysctrl_rst_lmac_phy();
    sysctrl_efuse_validity_handle();
    ota_msg_show();
    VERSION_SHOW();
    module_version_show();
    sys_reset_show();
    os_workqueue_init(&main_wkq, "MAIN", OS_TASK_PRIORITY_NORMAL, 2048);
    mainwkq_monitor_init();
    os_run_func((os_run_func_t)main, 0, 0, 0);

#ifdef CSKY_OS
    csi_kernel_start();
#endif
#ifdef OHOS
    LOS_Start();
#endif
}

