#ifndef USBD_MASS_SPEED_OPTIMIZE_H
#define USBD_MASS_SPEED_OPTIMIZE_H
#include "sys_config.h"

#define PINGPANG_BUF_EN	        0       //usbd mass读写速度优化线程使能

//SD   : malloc sram = 512  x MULTI_SECTOR_COUNT x 2 (默认MULTI_SECTOR_COUNT = 4)
//FLASH: malloc sram = 4096 x MULTI_SECTOR_COUNT x 2 (默认MULTI_SECTOR_COUNT = 2)
#if PINGPANG_BUF_EN
#if USBDISK == 1
#define MULTI_SECTOR_COUNT      4       
#elif USBDISK == 2
#define MULTI_SECTOR_COUNT      2
#endif
#else
#define MULTI_SECTOR_COUNT      1
#endif

#define USB_IO_TEST_TIME        0


#if PINGPANG_BUF_EN
enum read_write_flag
{
    READ_FLAG = 0,
    WRITE_FLAG,
    RELEASE_FLAG,
};

typedef int (*usbd_mass_read)(uint32 lba, uint32 count, uint8* buf);
typedef int (*usbd_mass_write)(uint32 lba, uint32 count, uint8* buf);

struct usbd_mass_speed_info
{
    uint8_t *buf_1;
    uint8_t *buf_2;
    struct os_task usbd_mass_speed_task;
};

struct usbd_mass_speed_optimize_mq
{
    uint32_t read_write_flag;   //0:read, 1:write, 2:delete thread
    uint32_t sector_addr;
    uint32_t sector_count;
};

struct usbd_mass_speed_dev_t
{
    struct usbd_mass_speed_info *disk;
    struct os_msgqueue *msgqueue;
    struct os_semaphore *sem;
    struct os_semaphore *usb_sem_write;
    struct usbd_mass_speed_optimize_mq mq;

    uint32_t error_flag;
    uint32_t thread_status;

    usbd_mass_read  udisk_read;
    usbd_mass_write udisk_write;
};

extern volatile uint32_t pingpang_flag;
extern struct usbd_mass_speed_dev_t *g_dev;

int usbd_mass_speed_optimize_send_mq(struct usbd_mass_speed_dev_t *dev, uint32_t sector_addr, uint32_t sector_count, uint32_t read_write_flag);
void* usbd_mass_speed_optimize_thread_init(void *arg);
void  usbd_mass_speed_optimize_thread_deinit();

#endif

#endif
