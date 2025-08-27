#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/scale.h"


int32 scale_open(struct scale_device *p_scale)
{
    if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->open) {
        return ((const struct scale_hal_ops *)p_scale->dev.ops)->open(p_scale);
    }
    return RET_ERR;
}

int32 scale_close(struct scale_device *p_scale)
{
    if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->close) {
        return ((const struct scale_hal_ops *)p_scale->dev.ops)->close(p_scale);
    }
    return RET_ERR;
}

int32 scale_suspend(struct scale_device *p_scale)
{
    if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->suspend) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->suspend(p_scale);
    }
    return RET_ERR;
}

int32 scale_resume(struct scale_device *p_scale)
{
    if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->resume) {
        return ((const struct scale_hal_ops *)p_scale->dev.ops)->resume(p_scale);
    }
    return RET_ERR;
}


int32 scale_request_irq(struct scale_device *p_scale, uint32 irq_flags, scale_irq_hdl irq_hdl, uint32 irq_data)
{
    if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->request_irq) {
        return ((const struct scale_hal_ops *)p_scale->dev.ops)->request_irq(p_scale, irq_flags, irq_hdl, irq_data);
    }
    return RET_ERR;
}

int32 scale_release_irq(struct scale_device *p_scale, uint32 irq_flags)
{
    if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->release_irq) {
        return ((const struct scale_hal_ops *)p_scale->dev.ops)->release_irq(p_scale, irq_flags);
    }
    return RET_ERR;
}


int32 scale_set_in_out_size(struct scale_device *p_scale, uint32 s_w,uint32 s_h,uint32 o_w,uint32 o_h)
{
    if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
        return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_SET_IN_OUT_SIZE, (s_h<<16)|s_w, (o_h<<16)|o_w);
    }
    return RET_ERR;
}

int32 scale_set_step(struct scale_device *p_scale, uint32 s_w,uint32 s_h,uint32 o_w,uint32 o_h)
{
    if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
        return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_SET_STEP, (s_h<<16)|s_w, (o_h<<16)|o_w);
    }
    return RET_ERR;
}

int32 scale_set_start_addr(struct scale_device *p_scale, uint32 s_x,uint32 s_y)
{
	if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_SET_START_ADDR, (s_y<<16)|s_x, 0);
	}
	return RET_ERR;
}


int32 scale_set_srambuf_wlen(struct scale_device *p_scale, uint32 wlen)
{
	if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_SET_SRAMBUF_WLEN, wlen, 0);
	}
	return RET_ERR;
}

int32 scale_set_in_yaddr(struct scale_device *p_scale, uint32 yaddr)
{
	if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_SET_IN_Y_ADDR, yaddr, 0);
	}
	return RET_ERR;
}

int32 scale_set_in_uaddr(struct scale_device *p_scale, uint32 uaddr)
{
	if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_SET_IN_U_ADDR, uaddr, 0);
	}
	return RET_ERR;
}


int32 scale_set_in_vaddr(struct scale_device *p_scale, uint32 vaddr)
{
	if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_SET_IN_V_ADDR, vaddr, 0);
	}
	return RET_ERR;
}


int32 scale_set_out_yaddr(struct scale_device *p_scale, uint32 yaddr)
{
	if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_SET_OUT_Y_ADDR, yaddr, 0);
	}
	return RET_ERR;
}

int32 scale_set_out_uaddr(struct scale_device *p_scale, uint32 uaddr)
{
	if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_SET_OUT_U_ADDR, uaddr, 0);
	}
	return RET_ERR;
}


int32 scale_set_out_vaddr(struct scale_device *p_scale, uint32 vaddr)
{
	if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_SET_OUT_V_ADDR, vaddr, 0);
	}
	return RET_ERR;
}

int32 scale_set_line_buf_num(struct scale_device *p_scale, uint32 num)
{
	if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_SET_LINE_BUF_NUM, num, 0);
	}
	return RET_ERR;
}

int32 scale_set_line_buf_addr(struct scale_device *p_scale, uint32 addr)
{
	if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_SET_SRAM_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 scale_set_dma_to_memory(struct scale_device *p_scale, uint32 en)
{
	if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_DMA_TO_MEMORY, en, 0);
	}
	return RET_ERR;
}

int32 scale_set_data_from_vpp(struct scale_device *p_scale, uint32 en)
{
	if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_SET_DATA_FROM, en, 0);
	}
	return RET_ERR;
}

int32 scale_get_inbuf_num(struct scale_device *p_scale)
{
	if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_GET_INBUF_NUM, 0, 0);
	}
	return RET_ERR;
}

int32 scale_set_inbuf_num(struct scale_device *p_scale,uint32 num,uint32 start_line)
{
	if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_SET_INBUF_NUM, num, start_line);
	}
	return RET_ERR;
}


int32 scale_set_new_frame(struct scale_device *p_scale,uint8 en)
{
	if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_SET_NEW_FRAME, en, 0);
	}
	return RET_ERR;
}

int32 scale_set_end_frame(struct scale_device *p_scale,uint8 en)
{
	if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_SET_END_FRAME, en, 0);
	}
	return RET_ERR;
}

int32 scale_get_heigh_cnt(struct scale_device *p_scale)
{
	if (p_scale && ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl) {
		return ((const struct scale_hal_ops *)p_scale->dev.ops)->ioctl(p_scale, SCALE_IOCTL_CMD_GET_HEIGH_CNT, 0, 0);
	}
	return RET_ERR;
}


