#ifndef __OSD_ENCODE_STREAM_H
#define __OSD_ENCODE_STREAM_H
#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "stream_frame.h"
#include "hal/lcdc.h"

enum
{
	OSD_HARDWARE_REAY_CMD = CUSTOM_STREAM_CMD+1,
	OSD_HARDWARE_ENCODE_SET_LEN,
	OSD_HARDWARE_ENCODE_GET_LEN,
	OSD_HARDWARE_ENCODE_BUF,
	OSD_HARDWARE_DEV,
	OSD_HARDWARE_STREAM_RESET,
	OSD_HARDWARE_STREAM_STOP,
};


#endif