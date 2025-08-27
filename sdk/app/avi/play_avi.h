#ifndef __PLAY_AVI_H
#define __PLAY_AVI_H
#include "stream_frame.h"
enum
{
    PLAY_AVI_CMD_NONE,
    PLAY_AVI_CMD_PLAY,
    PLAY_AVI_CMD_PAUSE,
    PLAY_AVI_CMD_1FPS_PAUSE,
    PLAY_AVI_CMD_STOP,
};
stream* creat_play_avi_stream(const char *name,char *filepath,char *jpeg_recv_name,char *audio_recv_name);
uint32_t pause_1FPS_avi_stream(stream* s);
uint32_t start_play_avi_thread(stream *s);
uint32_t pause_avi_stream(stream* s);
uint32_t play_avi_stream(stream* s);
uint32_t stop_avi_stream(stream* s);

#endif