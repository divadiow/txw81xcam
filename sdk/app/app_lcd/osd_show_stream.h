#ifndef __OSD_SHOW_STREAM_H
#define __OSD_SHOW_STREAM_H
#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "stream_frame.h"
#include "hal/lcdc.h"
struct osd_show_s
{
	struct lcdc_device *lcd_dev;
	stream *s;
	struct data_structure *last_show_data_s;
};

#endif