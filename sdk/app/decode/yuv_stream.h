#ifndef __YUV_STREAM_H
#define __YUV_STREAM_H
#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "stream_frame.h"
#include "utlist.h"
#include "osal/work.h"
#include "jpg_decode_stream.h"
struct yuv_stream_s
{
    struct data_structure *last_data_s;
    uint32_t show_yuv_time_start;
    uint8_t enable;  //使能
};

stream *yuv_stream(const char *name);
#endif