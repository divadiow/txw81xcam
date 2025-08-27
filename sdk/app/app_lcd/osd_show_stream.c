#include "osd_show_stream.h"
#include "custom_mem/custom_mem.h"
#include "osal/string.h"
//这里只是为了给osd创建流,实际内部没有做什么,这个流取到的数据应该是已经压缩过的
//接收到就可以配置硬件寄存器了

static int opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
			s->priv = priv;
			enable_stream(s,1);
		}
		break;
		case STREAM_OPEN_FAIL:
		break;
		default:
			//默认都返回成功
		break;
	}
	return res;
}
stream *osd_show_stream(const char *name)
{
	struct osd_show_s *show = (struct osd_show_s*)custom_zalloc_psram(sizeof(struct osd_show_s));
	struct lcdc_device *lcd_dev;	
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);
	show->lcd_dev = lcd_dev;
	stream *s = open_stream_available(name,0,8,opcode_func,show);
	return s;
}