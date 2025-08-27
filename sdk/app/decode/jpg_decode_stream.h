#ifndef __JPG_DECODE_STREAM_H__
#define __JPG_DECODE_STREAM_H__

enum jpg_decode_cmd_e
{
    JPG_DECODE_SET_CONFIG = CUSTOM_STREAM_CMD +1, //配置w、h、rotate
    JPG_DECODE_SET_IN_OUT_SIZE,
    JPG_DECODE_SET_STEP,
    JPG_DECODE_READY,
    JPG_DECODE_START,
};


#endif