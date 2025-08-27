#ifndef __VIDEO_APP_BBM_H__
#define __VIDEO_APP_BBM_H__

#include "sys_config.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "typesdef.h"
#include "stream_frame.h"

typedef struct
{
    uint32_t  timeinfo;
    uint32_t frame_len;
    uint8_t  state;             //0:未使用 1:填充完成，待使用 2:使用完成 3:正在使用
    uint8_t  *data;
}bbm_net_jpeg_frame;

extern bbm_net_jpeg_frame* bbm_get_frame();
extern void bbm_del_frame(bbm_net_jpeg_frame* get_f);

stream *bbm_net_jpeg_psram_stream_init();
void bbm_net_jpeg_psram_stream_deinit(stream *s);
void bbm_net_jpeg_init();

#endif