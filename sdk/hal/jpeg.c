#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/jpeg.h"

int32 jpg_open(struct jpg_device *p_jpg)
{
    if(p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->open) {
        return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->open(p_jpg);
    }
    return RET_ERR;
}

int32 jpg_close(struct jpg_device *p_jpg)
{
    if(p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->close) {
        return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->close(p_jpg);
    }
    return RET_ERR;
}

int32 jpg_init(struct jpg_device *p_jpg, uint32 table_idx, uint32 qt)
{
    if(p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->init) {
        return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->init(p_jpg, table_idx, qt);
    }
    return RET_ERR;
}

int32 jpg_suspend(struct jpg_device *p_jpg)
{
    if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->suspend) {
		return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->suspend(p_jpg);
    }
    return RET_ERR;
}

int32 jpg_resume(struct jpg_device *p_jpg)
{
    if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->resume) {
        return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->resume(p_jpg);
    }
    return RET_ERR;
}

int32 jpg_is_online(struct jpg_device *p_jpg)
{
	if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl)
	    return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl(p_jpg, JPG_IOCTL_CMD_IS_ONLINE, 0, 0);
	return RET_ERR;
}

int32 jpg_set_init_finish(struct jpg_device *p_jpg,uint32_t value)
{
	if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl)
    	return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl(p_jpg, JPG_IOCTL_CMD_SET_INIT_FINISH, value, 0);
	return RET_ERR;
}

int32 jpg_get_init_flag(struct jpg_device *p_jpg)
{
	if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl)
    	return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl(p_jpg, JPG_IOCTL_CMD_GET_INIT_FLAG, 0, 0);
	return RET_ERR;
}

int32 jpg_set_data_from(struct jpg_device *p_jpg, uint8 src_from)
{
	if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl)
    	return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl(p_jpg, JPG_IOCTL_CMD_SET_SRC_FROM, src_from, 0);
	return RET_ERR;
}



int32 jpg_set_size(struct jpg_device *p_jpg, uint32 h, uint32 w)
{
    if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl) {
        return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl(p_jpg, JPG_IOCTL_CMD_SET_SIZE, h, w);
    }
    return RET_ERR;
}

int32 jpg_set_hw_check(struct jpg_device *p_jpg, uint8 enable)
{
	if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl)
		return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl(p_jpg, JPG_IOCTL_CMD_HW_CHK, enable, 0);
	return RET_ERR;
}

int32 jpg_open_debug(struct jpg_device *p_jpg, uint8 enable)
{
	if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl)
	    return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl(p_jpg, JPG_IOCTL_CMD_OPEN_DBG, enable, 0);
	return RET_ERR;
}

int32 jpg_soft_kick(struct jpg_device *p_jpg, uint8 kick)
{
	if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl)
	    return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl(p_jpg, JPG_IOCTL_CMD_SOFT_KICK, kick, 0);
	return RET_ERR;
}

int32 jpg_updata_dqt(struct jpg_device *p_jpg, uint32 *dqtbuf)
{
    if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl) {
        return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl(p_jpg, JPG_IOCTL_CMD_UPDATE_QT, (uint32)dqtbuf, 0);
    }
    return RET_ERR;
}

int32 jpg_set_addr(struct jpg_device *p_jpg, uint32 addr, uint32 len)
{
    if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl) {
        return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl(p_jpg, JPG_IOCTL_CMD_SET_ADR, addr, len);
    }
    return RET_ERR;
}

int32 jpg_set_qt(struct jpg_device *p_jpg, uint32 qt)
{
    if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl) {
        return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl(p_jpg, JPG_IOCTL_CMD_SET_QT, qt, 0);
    }
    return RET_ERR;
}


int32 jpg_decode_target(struct jpg_device *p_jpg, uint8 to_scaler)
{
    if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl) {
        return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl(p_jpg, JPG_IOCTL_CMD_DECODE_TAG, to_scaler, 0);
    }
    return RET_ERR;
}


int32 jpg_is_idle(struct jpg_device *p_jpg)
{
	if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl)
	    return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->ioctl(p_jpg, JPG_IOCTL_CMD_IS_IDLE, 0, 0);
	return RET_ERR;
}

int32 jpg_decode_photo(struct jpg_device *p_jpg, uint32 photo)
{
    if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->decode) {
        return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->decode(p_jpg, photo);
    }
    return RET_ERR;
}


int32 jpg_request_irq(struct jpg_device *p_jpg, jpg_irq_hdl isr, uint32 irq, void *dev_id)
{
    if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->request_irq) {
        return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->request_irq(p_jpg, isr, irq, (uint32)dev_id);
    }
    return RET_ERR;
}

int32 jpg_release_irq(struct jpg_device *p_jpg, uint32 irq_flags)
{
    if (p_jpg && ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->release_irq) {
        return ((const struct jpeg_hal_ops *)p_jpg->dev.ops)->release_irq(p_jpg, irq_flags);
    }
    return RET_ERR;
}

