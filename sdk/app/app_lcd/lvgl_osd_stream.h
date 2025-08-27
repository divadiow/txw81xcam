#ifndef __LVGL_OSD_STREAM_H
#define __LVGL_OSD_STREAM_H
#include "stream_frame.h"
typedef void (*osd_finish_cb)(void *self);
typedef void (*osd_free_cb)(void *self,void *data);
struct encode_data_s_callback
{
	osd_finish_cb finish_cb;
	osd_free_cb   free_cb;
	void *user_data;
};
#endif