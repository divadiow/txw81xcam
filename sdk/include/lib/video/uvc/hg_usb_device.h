#ifndef _HG_USB_DEVICE_H
#define _HG_USB_DEVICE_H

#define USB_TEST_WIFI           0
#define USB_TEST_AUDIO          0
#define USB_TEST_MASS           1
#define RX_BUFF_SIZE            1024
struct hg_usb_device_test {
#if USB_TEST_WIFI

    //正常host不需要使用
    #if USB_HOST_EN
    int8   rx_buf[4];   /* usb rx buffer reserved 4byte for no overflow control */
    int8   tx_buf[4];

    #else
    int8   rx_buf[512*16+4];   /* usb rx buffer reserved 4byte for no overflow control */
    int8   tx_buf[512*16];
    #endif
#endif
#if USB_TEST_AUDIO
    int8   rx_buf[512*4+4];
    int8   tx_buf[512*4];
#endif

#if USB_TEST_MASS
    int8   rx_buf[RX_BUFF_SIZE];   /* usb rx buffer reserved 4byte for no overflow control */
    int8   tx_buf[RX_BUFF_SIZE];
#else 
    int8   rx_buf[512];   /* usb rx buffer reserved 4byte for no overflow control */
    int8   tx_buf[512];
#endif
    int32  ready;
    uint32 total_tx_len;
    uint32 total_rx_len;
    struct os_mutex    rx_lock;
    struct os_mutex    tx_lock;
    struct os_semaphore    sem;
    struct usb_device *dev;
    struct os_task scsi_task;
    struct os_task sd_task;
    
};

#endif