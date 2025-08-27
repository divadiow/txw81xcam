#include "typesdef.h"
#include "osal/string.h"
#include "csi_core.h"
#include <k_api.h>
#include <csi_kernel.h>

void csi_kernel_task_dump(k_task_handle_t task_handle, void* stack);

__bobj uint8_t assert_holdup;

void assert_call_addr(const char *__function, unsigned int __line,void* addr)
{
    os_printf(KERN_ERR"%s failed: function: %s, line %d\tcall addr:%X\r\n", __FUNCTION__,__function, __line,(uint32_t)addr);
}
void assert_internal(const char *__function, unsigned int __line, const char *__assertion)
{
    int print_loop = 0;
    uint32_t in_int = __in_interrupt();
    ktask_t *task = csi_kernel_task_get_cur();

    disable_print(0);
    if (assert_holdup) {
        __disable_irq();
        mcu_watchdog_timeout(0); //disable watchdog
        jtag_map_set(1);
    }

    os_printf(KERN_ERR"[%s:%p]: assertation \"%s\" failed: function: %s, line %d\r\n", 
              (in_int ? "Interrupt" : task->task_name), (in_int ? 0 : task),
              __assertion, __function, __line);

    sys_errlog_flush(0xffffffff, 0, 0);

    if(!in_int && task){
        csi_kernel_task_dump(task, 0);
    }

    do {
        os_printf(KERN_ERR"[%s:%p]: assertation \"%s\" failed: function: %s, line %d\r\n", 
                  (in_int ? "Interrupt" : task->task_name), (in_int ? 0 : task),
                  __assertion, __function, __line);
        delay_us(1000*1000);
    } while (assert_holdup || print_loop++ < 5);

    mcu_reset();
}


