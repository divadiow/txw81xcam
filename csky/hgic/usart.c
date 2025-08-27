#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "hal/uart.h"

#include "csi_core.h"

extern int8_t __disable_print__;
int32_t csi_usart_getchar(void* handle, uint8_t *ch)
{
    *ch = uart_getc((struct uart_device *)handle);
    return RET_OK;
}

int32_t csi_usart_putchar(void*      handle, uint8_t ch)
{
    if (__disable_print__) { return 0; }
    return uart_putc((struct uart_device *)handle, ch);
}
