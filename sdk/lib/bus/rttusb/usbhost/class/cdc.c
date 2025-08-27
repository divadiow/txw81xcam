#include "include/usb_host.h"
#include "cdc.h"

#ifdef RT_USBH_CDC

static struct uclass_driver cdc_driver;
volatile ucdc_data_t cdc_d;
uint8_t buff_out[64] __attribute__((aligned(4)));
uint8_t buff_in[64] __attribute__((aligned(4)));

rt_err_t rt_usbh_cdc_send_command(uinst_t device, void* buffer, int nbytes)
{
    struct urequest setup;
    int timeout = USB_TIMEOUT_BASIC;

    RT_ASSERT(device != RT_NULL);

    setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_CLASS |
        USB_REQ_TYPE_INTERFACE;
    setup.bRequest = SEND_ENCAPSULATED_COMMAND;
    setup.wIndex = 0;
    setup.wLength = nbytes;
    setup.wValue = 0;

    if(rt_usb_hcd_setup_xfer(device->hcd, device->pipe_ep0_out, &setup, timeout) == 8)
    {
        if(rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_out, buffer, nbytes, timeout) == nbytes)
        {
            if(rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_in, RT_NULL, 0, timeout) == 0)
            {
                return nbytes;
            }
        }
    }
    return RT_ERROR;
}

rt_err_t rt_usbh_cdc_get_response(uinst_t device, void* buffer, int nbytes)
{
    struct urequest setup;
    int timeout = USB_TIMEOUT_BASIC;
    int ret_size;

    RT_ASSERT(device != RT_NULL);

    setup.request_type = USB_REQ_TYPE_DIR_IN | USB_REQ_TYPE_CLASS |
        USB_REQ_TYPE_INTERFACE;
    setup.bRequest = GET_ENCAPSULATED_RESPONSE;
    setup.wIndex = 0;
    setup.wLength = nbytes;
    setup.wValue = 0;

    if(rt_usb_hcd_setup_xfer(device->hcd, device->pipe_ep0_out, &setup, timeout) == 8)
    {
        ret_size = rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_in, buffer, nbytes, timeout);
        if(ret_size > 0)
        {
            if(rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_out, RT_NULL, 0, timeout) == 0)
            {
                return ret_size;
            }
        }
    }
    return RT_ERROR;
}

rt_err_t rt_usbh_cdc_get_line_coding(uinst_t device, int intf, void* buffer)
{
    struct urequest setup;
    int timeout = USB_TIMEOUT_BASIC;
    int ret_size;

    RT_ASSERT(device != RT_NULL);

    setup.request_type = USB_REQ_TYPE_DIR_IN | USB_REQ_TYPE_CLASS |
        USB_REQ_TYPE_INTERFACE;
    setup.bRequest = GET_LINE_CODING;
    setup.wIndex = intf;
    setup.wLength = 7;
    setup.wValue = 0;

    if(rt_usb_hcd_setup_xfer(device->hcd, device->pipe_ep0_out, &setup, timeout) == 8)
    {
        ret_size = rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_in, buffer, 7, timeout);
        if(ret_size == 7)
        {
            if(rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_out, RT_NULL, 0, timeout) == 0)
            {
                return ret_size;
            }
        }
    }
    return RT_ERROR;
}

rt_err_t rt_usbh_cdc_set_line_coding(uinst_t device, int intf, void* buffer)
{
    struct urequest setup;
    int timeout = USB_TIMEOUT_BASIC;

    RT_ASSERT(device != RT_NULL);
    setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE;
    setup.bRequest = SET_LINE_CODING;
    setup.wIndex = intf;   // interface
    setup.wLength = 7;
    setup.wValue = 0;

    if(rt_usb_hcd_setup_xfer(device->hcd, device->pipe_ep0_out, &setup, timeout) == 8)
    {
        if(rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_out, buffer, 7, timeout) == 7)
        {
            if(rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_in, RT_NULL, 0, timeout) == 0)
            {
                return RT_EOK;
            }
        }
    }
    return -RT_ERROR;
}

rt_err_t rt_usbh_cdc_set_control_line_state(uinst_t device, int intf, void * buffer, int len)
{
    struct urequest setup;
    int timeout = USB_TIMEOUT_BASIC;

    RT_ASSERT(device != RT_NULL);
    setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE;
    setup.bRequest = SET_CONTROL_LINE_STATE;
    setup.wIndex = intf;   // interface
    setup.wLength = len;
    setup.wValue = 0;

    if(rt_usb_hcd_setup_xfer(device->hcd, device->pipe_ep0_out, &setup, timeout) == 8)
    {    
        if(rt_usb_hcd_pipe_xfer(device->hcd, device->pipe_ep0_in, RT_NULL, 0, timeout) == 0)
        {
            return RT_EOK;
        }
    }
    return -RT_ERROR;
}

void analysis_cdc_line_coding(struct usb_cdc_line_coding * line_coding)
{
    os_printf("=========line coding=========\n");
    os_printf("dwDTERate:%d\n", line_coding->dwDTERate);
    os_printf("bCharFormat:%d\n", line_coding->bCharFormat);
    os_printf("bParityType:%d\n", line_coding->bParityType);
    os_printf("bDataBits:%d\n", line_coding->bDataBits);
    os_printf("=============================\n");
}

static rt_err_t rt_usbh_get_CDC_interface_descriptor(ucfg_desc_t cfg_desc, int num,
    uintf_desc_t* intf_desc)
{
    rt_uint32_t ptr, depth = 0;
    udesc_t desc;

    /* check parameter */
    RT_ASSERT(cfg_desc != RT_NULL);

    ptr = (rt_uint32_t)cfg_desc + cfg_desc->bLength;
    while(ptr < (rt_uint32_t)cfg_desc + cfg_desc->wTotalLength)
    {
        if(depth++ > 0x40)
        {
            *intf_desc = RT_NULL;
            return -RT_EIO;
        }
        desc = (udesc_t)ptr;
        if(desc->type == USB_DESC_TYPE_INTERFACE)
        {
            if(((uintf_desc_t)desc)->bNumEndpoints == 0)
            {
                ptr = (rt_uint32_t)desc + desc->bLength;
                continue;
            }
            if(((uintf_desc_t)desc)->bInterfaceNumber == num)
            {
                *intf_desc = (uintf_desc_t)desc;

                LOG_D("rt_usb_get_interface_descriptor: %d", num);
                return RT_EOK;
            }
        }
        ptr = (rt_uint32_t)desc + desc->bLength;
    }

    rt_kprintf("rt_usb_get_interface_descriptor %d failed\n", num);
    return -RT_EIO;
}

static rt_err_t rt_usbh_cdc_enable(void *arg)
{
    struct uhintf **intf = arg;
    uhcd_t hcd = NULL;

    if (intf == NULL) {
        return -EIO;
    }

    hcd = intf[0]->device->hcd;
    os_printf("subclass %d, protocal %d\r\n",
        intf[0]->intf_desc->bInterfaceSubClass,
        intf[0]->intf_desc->bInterfaceProtocol);

#ifdef RT_USBH_UVC
    int i;
    struct usb_cdc_line_coding line_coding;
    upipe_t pipe;
    uintf_desc_t intf_desc = RT_NULL;

    cdc_d = rt_malloc(sizeof(struct ucdc_data));
    if(cdc_d == RT_NULL)
    {
        rt_kprintf("allocate cdc_d memory failed\n");
        return -RT_ENOMEM;
    }
    os_printf("cdc_d:%x\n",cdc_d);

    cdc_d->line_coding = &line_coding;
    cdc_d->device = intf[0]->device;

    rt_memset(cdc_d, 0, sizeof(struct ucdc_data));
    intf[0]->user_data = (void *)cdc_d;

    rt_usbh_cdc_get_line_coding(intf[0]->device, intf[0]->intf_desc->bInterfaceNumber, &line_coding);

    analysis_cdc_line_coding(&line_coding);

    line_coding.dwDTERate = BAUD_RATE_2000000;
    line_coding.bCharFormat = STOP_BITS_1;
    line_coding.bParityType = PARITY_NONE;
    line_coding.bDataBits = DATA_BITS_8;

    rt_usbh_cdc_set_line_coding(intf[0]->device, intf[0]->intf_desc->bInterfaceNumber, &line_coding);


    analysis_cdc_line_coding(&line_coding);

    rt_usbh_cdc_get_line_coding(intf[0]->device, intf[0]->intf_desc->bInterfaceNumber, &line_coding);

    rt_usbh_cdc_set_control_line_state(intf[0]->device, intf[0]->intf_desc->bInterfaceNumber, RT_NULL, 0);

    analysis_cdc_line_coding(&line_coding);

    rt_usbh_get_CDC_interface_descriptor(intf[0]->device->cfg_desc, 1, &intf_desc);

    for(i = 0; i < intf_desc->bNumEndpoints; i++)
    {
        uep_desc_t ep_desc;
        rt_usbh_get_endpoint_descriptor(intf_desc, i, &ep_desc);
        if(ep_desc == RT_NULL)
        {
            rt_kprintf("rt_usb_get_endpoint_descriptor error\n");
            return -RT_ERROR;
        }
        analysis_usb_ep_desc(ep_desc); //获取端点描述符 打印端点描述符信息
        /* the endpoint type of mass storage class should be BULK */
        if((ep_desc->bmAttributes & USB_EP_ATTR_TYPE_MASK) != USB_EP_ATTR_BULK)
            continue;
        
        if (rt_usb_hcd_alloc_pipe(intf[0]->device->hcd, &pipe, intf[0]->device, ep_desc) != RT_EOK) {
            rt_kprintf("alloc pipe failed\n");
            return -RT_ERROR;
        }

        rt_usb_instance_add_pipe(intf[0]->device, pipe);

        if ((ep_desc->bEndpointAddress & USB_DIR_MASK) == USB_DIR_IN) {
            cdc_d->pipe_in = pipe;
            os_printf("cdc_d->pipe_in:%x pipe:%x\n",cdc_d->pipe_in,pipe);
        } else {
            cdc_d->pipe_out = pipe;
            os_printf("cdc_d->pipe_out:%x pipe:%x\n",cdc_d->pipe_out,pipe);
        }     

    }

    buff_out[0] = 0xAA;
    buff_out[1] = 0x55;
    buff_out[2] = 0xF1;

    rt_usb_hcd_pipe_xfer(intf[0]->device->hcd, cdc_d->pipe_out, buff_out, 8, 0);

    memset(buff_in,0,8);

    rt_usb_hcd_pipe_xfer(intf[0]->device->hcd, cdc_d->pipe_in, buff_in, 8, 0);

    for(int i = 0; i < 8; i++)
    {
        printf("buff_in[%d]:%x\n",i,buff_in[i]);
    }
#endif

    return RET_OK;
}

static rt_err_t rt_usbh_cdc_disable(void *arg)
{
#ifdef RT_USBH_UVC
    struct uhintf *intf = arg;

    if (intf == NULL) {
        return -EIO;
    }

    if(cdc_d != RT_NULL)
        rt_free(cdc_d);

#endif
    return RET_OK;
}

ucd_t rt_usbh_class_driver_cdc(void)
{
    cdc_driver.class_code = USB_CLASS_COMM;

    cdc_driver.enable = rt_usbh_cdc_enable;
    cdc_driver.disable = rt_usbh_cdc_disable;

    return &cdc_driver;
}

#endif