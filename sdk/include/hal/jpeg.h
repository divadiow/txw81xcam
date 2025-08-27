#ifndef _HAL_JPEG_H_
#define _HAL_JPEG_H_

enum jpg_device_irq_flag {
    JPG_IRQ_FLAG_JPG_DONE     = BIT(0),
    JPG_IRQ_FLAG_JPG_BUF_FULL = BIT(1),
    JPG_IRQ_FLAG_ERROR        = BIT(2),
    JPG_IRQ_FLAG_PIXEL_DONE   = BIT(3),
};

enum jpg_ioctl_cmd {
	JPG_IOCTL_CMD_IS_ONLINE,
    JPG_IOCTL_CMD_SET_ADR,
    JPG_IOCTL_CMD_SET_QT,
    JPG_IOCTL_CMD_SET_SIZE,
    JPG_IOCTL_CMD_SET_SRC_FROM,
    JPG_IOCTL_CMD_UPDATE_QT,
	JPG_IOCTL_CMD_DECODE_TAG,
	JPG_IOCTL_CMD_OPEN_DBG,
	JPG_IOCTL_CMD_SOFT_KICK,
	JPG_IOCTL_CMD_HW_CHK,
	JPG_IOCTL_CMD_IS_IDLE,
    JPG_IOCTL_CMD_SET_INIT_FINISH,
    JPG_IOCTL_CMD_GET_INIT_FLAG,
};


typedef int32(*jpg_irq_hdl)(uint32 irq_flag, uint32 irq_data, uint32 param1, uint32 param2);

struct jpg_device {
    struct dev_obj dev;
};

struct jpeg_hal_ops{
    struct devobj_ops ops;
    int32(*open)(struct jpg_device *jpg);
    int32(*close)(struct jpg_device *jpg);
	int32(*suspend)(struct jpg_device *jpg);
	int32(*resume)(struct jpg_device *jpg);
    int32(*init)(struct jpg_device *jpg, uint32 table_idx, uint32 qt);
	int32(*decode)(struct jpg_device *jpg,uint32 photo);
    int32(*ioctl)(struct jpg_device *jpg, enum jpg_ioctl_cmd cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct jpg_device *jpg, jpg_irq_hdl irq_hdl, uint32 irq_flag,  uint32 irq_data);
    int32(*release_irq)(struct jpg_device *jpg, uint32 irq_flag);
};

#ifdef TXW80X
#define JPG_NUM   1
#endif

#ifdef TXW81X
#define JPG_NUM   2
#endif

int32 jpg_open(struct jpg_device *jpg);
int32 jpg_close(struct jpg_device *jpg);
int32 jpg_init(struct jpg_device *jpg, uint32 table_idx, uint32 qt);
int32 jpg_updata_dqt(struct jpg_device *p_jpg, uint32 *dqtbuf);
int32 jpg_suspend(struct jpg_device *jpg);
int32 jpg_resume(struct jpg_device *jpg);
int32 jpg_set_size(struct jpg_device *p_jpg, uint32 h, uint32 w);
int32 jpg_set_addr(struct jpg_device *jpg, uint32 addr, uint32 buflen);
int32 jpg_set_qt(struct jpg_device *jpg, uint32 qt);
int32 jpg_open_debug(struct jpg_device *p_jpg, uint8 enable);
int32 jpg_soft_kick(struct jpg_device *p_jpg, uint8 kick);
int32 jpg_decode_target(struct jpg_device *p_jpg, uint8 to_scaler);
int32 jpg_decode_photo(struct jpg_device *p_jpg, uint32 photo);
int32 jpg_request_irq(struct jpg_device *jpg, jpg_irq_hdl irq_hdl, uint32 irq_flags,  void *irq_data);
int32 jpg_release_irq(struct jpg_device *jpg, uint32 irq_flags);
int32 jpg_set_data_from(struct jpg_device *p_jpg, uint8 src_from);
int32 jpg_set_hw_check(struct jpg_device *p_jpg, uint8 enable);
int32 jpg_is_online(struct jpg_device *p_jpg);
int32 jpg_is_idle(struct jpg_device *p_jpg);
int32 jpg_set_init_finish(struct jpg_device *p_jpg,uint32_t value);
int32 jpg_get_init_flag(struct jpg_device *p_jpg);



#endif
