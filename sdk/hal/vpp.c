#include "typesdef.h"
#include "list.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"
#include "hal/vpp.h"

int32 vpp_open(struct vpp_device *p_vpp)
{
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->open) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->open(p_vpp);
    }
    return RET_ERR;
}

int32 vpp_close(struct vpp_device *p_vpp)
{
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->close) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->close(p_vpp);
    }
    return RET_ERR;
}

int32 vpp_suspend(struct vpp_device *p_vpp)
{
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->suspend) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->suspend(p_vpp);
    }
    return RET_ERR;
}

int32 vpp_resume(struct vpp_device *p_vpp)
{
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->resume) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->resume(p_vpp);
    }
    return RET_ERR;
}

int32 vpp_dis_uv_mode(struct vpp_device *p_vpp,uint8 enable){
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_DIS_UV_MODE, enable, 0);
    }
    return RET_ERR;
}

int32 vpp_set_threshold(struct vpp_device *p_vpp,uint32 dlt,uint32 dht){
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_THRESHOLD, dlt, dht);
    }
    return RET_ERR;
}

int32 vpp_set_water0_rc(struct vpp_device *p_vpp,uint8 enable){
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_RC, enable, 0);
    }
    return RET_ERR;
}

int32 vpp_set_water1_rc(struct vpp_device *p_vpp,uint8 enable){
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK1_RC, enable, 0);
    }
    return RET_ERR;
}

int32 vpp_set_water0_color(struct vpp_device *p_vpp,uint8 y,uint8 u,uint8 v){
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_COLOR, y|(u<<8)|(v<<16), 0);
    }
    return RET_ERR;
}

int32 vpp_set_water1_color(struct vpp_device *p_vpp,uint8 y,uint8 u,uint8 v){
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK1_COLOR, y|(u<<8)|(v<<16), 0);
    }
    return RET_ERR;
}

int32 vpp_set_water0_bitmap(struct vpp_device *p_vpp,uint32 bitmap){
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_BMPADR, bitmap, 0);
    }
    return RET_ERR;
}

int32 vpp_set_water1_bitmap(struct vpp_device *p_vpp,uint32 bitmap){
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK1_BMPADR, bitmap, 0);
    }
    return RET_ERR;
}

int32 vpp_set_water0_locate(struct vpp_device *p_vpp,uint8 x,uint8 y){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_LOCATED, (y<<8)|x, 0);
	}
	return RET_ERR;
}

int32 vpp_set_water1_locate(struct vpp_device *p_vpp,uint8 x,uint8 y){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK1_LOCATED, (y<<8)|x, 0);
	}
	return RET_ERR;
}


int32 vpp_set_water0_contrast(struct vpp_device *p_vpp,uint8 contrast){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_CONTRAST, contrast, 0);
	}
	return RET_ERR;
}

int32 vpp_set_water1_contrast(struct vpp_device *p_vpp,uint8 contrast){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK1_CONTRAST, contrast, 0);
	}
	return RET_ERR;
}


int32 vpp_set_watermark0_charsize_and_num(struct vpp_device *p_vpp,uint8 w,uint8 h,uint8 num){
	
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_CHAR_SIZE_AND_NUM, w|(h<<8), num);
	}
	return RET_ERR;
}


int32 vpp_set_watermark0_idx(struct vpp_device *p_vpp,uint8 index_num,uint8 index){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_CHAR_IDX, index, index_num);
	}
	return RET_ERR;
}


int32 vpp_set_watermark1_size(struct vpp_device *p_vpp,uint8 w,uint8 h){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK1_PHOTO_SIZE, (h<<8)|w, 0);
	}
	return RET_ERR;
}


int32 vpp_set_watermark0_mode(struct vpp_device *p_vpp,uint8 mode){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_MODE, mode, 0);
	}
	return RET_ERR;
}

int32 vpp_set_watermark1_mode(struct vpp_device *p_vpp,uint8 mode){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK1_MODE, mode, 0);
	}
	return RET_ERR;
}

int32 vpp_set_watermark0_enable(struct vpp_device *p_vpp,uint8 enable){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK0_EN, enable, 0);
	}
	return RET_ERR;
}

int32 vpp_set_watermark1_enable(struct vpp_device *p_vpp,uint8 enable){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_WATERMARK1_EN, enable, 0);
	}
	return RET_ERR;
}

int32 vpp_set_motion_det_enable(struct vpp_device *p_vpp,uint8 enable){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_MOTION_DET_EN, enable, 0);
	}
	return RET_ERR;
}

int32 vpp_set_motion_calbuf(struct vpp_device *p_vpp,uint32 calbuf){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_MOTION_CALBUF, calbuf, 0);
	}
	return RET_ERR;
}

int32 vpp_set_motion_range(struct vpp_device *p_vpp,uint16 x_s,uint16 y_s,uint16 x_e,uint16 y_e){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_MOTION_RANGE, x_s|(y_s<<16), x_e|(y_e<<16));
	}
	return RET_ERR;
}

int32 vpp_set_motion_blk_threshold(struct vpp_device *p_vpp,uint8 threshold){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_MOTION_BLK_THRESHOLD, threshold, 0);
	}
	return RET_ERR;
}

int32 vpp_set_motion_frame_threshold(struct vpp_device *p_vpp,uint32 threshold){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_MOTION_FRAME_THRESHOLD, threshold, 0);
	}
	return RET_ERR;
}

int32 vpp_set_ifp_addr(struct vpp_device *p_vpp,uint32 addr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_IFP_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_ifp_en(struct vpp_device *p_vpp,uint8 enable){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_IFP_EN, enable, 0);
	}
	return RET_ERR;
}

int32 vpp_set_mode(struct vpp_device *p_vpp,uint8 mode){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_MODE, mode, 0);
	}
	return RET_ERR;
}

int32 vpp_set_video_size(struct vpp_device *p_vpp,uint32 w,uint32 h){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_SIZE, w, h);
	}
	return RET_ERR;
}

int32 vpp_set_ycbcr(struct vpp_device *p_vpp,uint8 ycbcr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_YCBCR, ycbcr, 0);
	}
	return RET_ERR;
}


int32 vpp_set_buf0_count(struct vpp_device *p_vpp,uint8 cnt){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF0_CNT, cnt, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf1_count(struct vpp_device *p_vpp,uint8 cnt){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF1_CNT, cnt, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf1_en(struct vpp_device *p_vpp,uint8 enable){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF1_EN, enable, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf1_shrink(struct vpp_device *p_vpp,uint8 half){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF1_SHRINK, half, 0);
	}
	return RET_ERR;
}

int32 vpp_set_input_interface(struct vpp_device *p_vpp,uint8 mode){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_INPUT_INTERFACE, mode, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf0_y_addr(struct vpp_device *p_vpp,uint32 addr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF0_Y_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf0_u_addr(struct vpp_device *p_vpp,uint32 addr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF0_U_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf0_v_addr(struct vpp_device *p_vpp,uint32 addr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF0_V_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf1_y_addr(struct vpp_device *p_vpp,uint32 addr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF1_Y_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf1_u_addr(struct vpp_device *p_vpp,uint32 addr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF1_U_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_buf1_v_addr(struct vpp_device *p_vpp,uint32 addr)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_BUF1_V_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_itp_y_addr(struct vpp_device *p_vpp,uint32 addr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_ITP_Y_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_itp_u_addr(struct vpp_device *p_vpp,uint32 addr){
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_ITP_U_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_itp_v_addr(struct vpp_device *p_vpp,uint32 addr)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_ITP_V_ADDR, addr, 0);
	}
	return RET_ERR;
}

int32 vpp_set_itp_linebuf(struct vpp_device *p_vpp,uint32 linebuf)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_SET_TAKE_PHOTO_LINEBUF, linebuf, 0);
	}
	return RET_ERR;
}

int32 vpp_set_itp_enable(struct vpp_device *p_vpp,uint8 en)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_ITP_EN, en, 0);
	}
	return RET_ERR;
}

int32 vpp_set_itp_auto_close(struct vpp_device *p_vpp,uint8 en)
{
	if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl) {
		return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->ioctl(p_vpp, VPP_IOCTL_CMD_ITP_AUTO_CLOSE, en, 0);
	}
	return RET_ERR;
}


int32 vpp_request_irq(struct vpp_device *p_vpp, uint32 irq_flags, vpp_irq_hdl irq_hdl, uint32 irq_data)
{
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->request_irq) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->request_irq(p_vpp, irq_flags, irq_hdl, irq_data);
    }
    return RET_ERR;
}

int32 vpp_release_irq(struct vpp_device *p_vpp, uint32 irq_flags)
{
    if (p_vpp && ((const struct vpp_hal_ops *)p_vpp->dev.ops)->release_irq) {
        return ((const struct vpp_hal_ops *)p_vpp->dev.ops)->release_irq(p_vpp, irq_flags);
    }
    return RET_ERR;
}


