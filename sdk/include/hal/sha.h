#ifndef _HAL_SHA_H_
#define _HAL_SHA_H_


struct sha_dev
{
    struct dev_obj dev;
};

enum SHA_CALC_FLAGS{
    SHA_CALC_LAST_DATA       = 0x01,
    SHA_CALC_SHA256          = 0x02,
    SHA_CALC_SHA384          = 0x04,
    SHA_CALC_SHA512          = 0x08,
};

enum SHA_TYPE {
    SHA_SHA256 = 0x01,
    SHA_SHA384 = 0x02,
    SHA_SHA512 = 0x04,
};

struct sha_desc{
    void *state;
    uint32 type;
};

struct sha256_state {
    uint8  buf[64];
    uint32 cnt;
    uint8  output[32];
};

enum sha_ioctl_cmd {
    SHA_IOCTL_CMD_START,
    SHA_IOCTL_CMD_END,
    SHA_IOCTL_CMD_RESET,
};

struct sha_hal_ops
{
    struct devobj_ops ops;
    int32 (*calc)(struct sha_dev *dev, uint8 input[], uint32 len, enum SHA_CALC_FLAGS flags);
    int32 (*read)(struct sha_dev *dev, uint8 output[], uint32 timeout);
    int32 (*ioctl)(struct sha_dev *dev, uint32 cmd, uint32 para);
    int32 (*requset_irq)(struct sha_dev *dev, void *irq_handle, void *args);
    int32 (*release_irq)(struct sha_dev *dev);
};

/*
    使用cache_clean去确保input对应的数据总是最新的
    sha_calc和sha_read已经淘汰了，请使用init->update*n->final
*/
int32 sha_calc(struct sha_dev *dev, uint8 input[], uint32 len, enum SHA_CALC_FLAGS flags);
int32 sha_read(struct sha_dev *dev, uint8 output[], uint32 timeout);
int32 sha_ioctl(struct sha_dev *dev, uint32 cmd, uint32 para);
int32 sha_init(struct sha_dev *dev, enum SHA_CALC_FLAGS flags);
int32 sha_update(struct sha_dev *dev, uint8 *input, uint32 len);
int32 sha_final(struct sha_dev *dev, uint8 *output);
int32 sha_requset_irq(struct sha_dev *dev, void *irq_handle, void *args);
int32 sha_release_irq(struct sha_dev *dev);





#endif
