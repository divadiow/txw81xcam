#ifndef __RECV_JPG_YUV_DEMO_H
#define __RECV_JPG_YUV_DEMO_H
#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "stream_frame.h"
#include "utlist.h"
#include "osal/work.h"
#include "jpg_decode_stream.h"
struct recv_jpg_yuv_s
{
    struct os_work work;
    stream *s;
    struct data_structure *last_data_s;
    uint32_t show_photo_time_start;
    uint8_t enable;  //使能
};

void update_yuv_stream_time(uint32_t time);
#endif