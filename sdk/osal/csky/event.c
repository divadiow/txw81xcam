#include "typesdef.h"
#include "errno.h"
#include "list.h"
#include "osal/event.h"
#include "csi_kernel.h"
#include "osal/string.h"

#define EVENT_MAGIC (0xa67b3cd4)

int32 os_event_init(struct os_event *evt)
{
    if(evt->magic == EVENT_MAGIC){
        os_printf(KERN_WARNING"event repeat initialization ????\r\n");
    }
    evt->hdl = csi_kernel_event_new();
    ASSERT(evt->hdl);
    if (evt->hdl) { 
        evt->magic = EVENT_MAGIC; 
    }
    return (evt->hdl ? RET_OK : RET_ERR);
}

int32 os_event_del(struct os_event *evt)
{
    ASSERT(evt && evt->magic == EVENT_MAGIC);
    int32 ret = csi_kernel_event_del(evt->hdl);
    evt->hdl = NULL;
    evt->magic = 0;
    return ret;
}

int32 os_event_set(struct os_event *evt, uint32 flags, uint32 *rflags)
{
    uint32 _rflags;
    ASSERT(evt && evt->magic == EVENT_MAGIC);
    if(rflags == NULL) rflags = &_rflags;
    return csi_kernel_event_set(evt->hdl, flags, rflags);
}

int32 os_event_clear(struct os_event *evt, uint32 flags, uint32 *rflags)
{
    uint32 _rflags;
    ASSERT(evt && evt->magic == EVENT_MAGIC);
    if(rflags == NULL) rflags = &_rflags;
    return csi_kernel_event_clear(evt->hdl, flags, rflags);
}

int32 os_event_get(struct os_event *evt, uint32 *rflags)
{
    ASSERT(evt && evt->magic == EVENT_MAGIC);
    return csi_kernel_event_get(evt->hdl, rflags);
}

int32 os_event_wait(struct os_event *evt, uint32 flags, uint32 *rflags, uint32 mode, int32 timeout)
{
    uint32 _rflags;
    k_event_opt_t options = KEVENT_OPT_SET_ALL;
    uint8_t clr_on_exit   = (mode & OS_EVENT_WMODE_CLEAR) ? 1 : 0;

    ASSERT(evt && evt->magic == EVENT_MAGIC);
    if (mode & OS_EVENT_WMODE_OR) {
        options = KEVENT_OPT_SET_ANY;
    }
    if(rflags == NULL) rflags = &_rflags;
    return csi_kernel_event_wait(evt->hdl, flags, options, clr_on_exit, rflags, timeout);
}

