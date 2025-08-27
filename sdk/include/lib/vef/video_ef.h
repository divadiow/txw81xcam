#ifndef _VIDEO_EF_H_
#define _VIDEO_EF_H_
#include "typesdef.h"

/*
    x轴缩放
    将一个yuv内存块缩放后放到另一个内存块中
*/
void sw_scaler_enlarge1_x(uint8 *iyuv, uint16 ph, uint16 pw,uint8 *oyuv, uint16 oh, uint16 ow);
void sw_scaler_shrink2_x(uint8* iyuv, uint16 ph, uint16 pw,uint8 *oyuv, uint16 oh, uint16 ow);

/*
	in是数据源，
	w是图像源宽度，
	h是图像源高度，
*/
void hexagon_ve1(uint8* in, uint8* tem, uint32 w, uint32 h);
void block_9(uint8* in, uint8* tem, uint32 w, uint32 h);
void block_4(uint8* in, uint8* tem, uint32 w, uint32 h);
void block_2_yinv(uint8* in, uint8* tem, uint32 w, uint32 h);
void block_2_xinv(uint8* in, uint8* tem, uint32 w, uint32 h);
void uv_offset(uint8* in, uint8* tmp, uint32 w, uint32 h, int32 uoff, int32 voff);
void ve_gray_img(uint8* in, uint8* tmp, uint32 w, uint32 h);



void vf_switch(uint32 sw);
void vf_set_addr(uint32 src, uint32 dst);

typedef struct _vf_struct {

	struct {
		uint32 open	    :1;
		uint32 self     :1;
		uint32 ve_sel   :8;
		uint32 revered  :22;
	}desp;
	uint32 p_src;
	uint32 p_dst;
} vf_cblk;


#endif
