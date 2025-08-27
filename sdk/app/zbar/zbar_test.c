#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "osal/task.h"
#include "osal/string.h"
#include "zbar.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "lib/heap/sysheap.h"
#if 0
#define TIME_FOR_KEEP      5000

struct  os_task handle_zbar_task;
//extern uint8 psram_yuv_buf[640*480];
//uint8 sram_for_zbar[640*120];
//extern uint8 psram_buf[IMAGE_H][IMAGE_W];
extern uint8 psram_ybuf_src[IMAGE_H*IMAGE_W+IMAGE_H*IMAGE_W/2];
extern volatile uint32 vs_isr;
extern int mask_type;

void yuv_data_deal(uint8_t *yuvsrc){
	uint32_t itk = 0;
	uint32_t jtk = 0;
	uint32_t *p;
	p = (uint32_t)yuvsrc;
	for(itk = 0;itk < 384;itk++){
		for(jtk = 0;jtk < 160;jtk++){
			p[itk*128+jtk] = p[(48+itk)*160+16+jtk]; 
		}
	}
}

void zbar_test(){
	struct vpp_device *vpp_test_gogle;
	struct dvp_device *dvp_test_gogle;
	uint8 *buf_fp;
	uint32 time_now;
	uint32 time_mask=0;
	uint8 count = 0;
	uint32 itk,jtk;
	int width = 512, height = 384;
	void *raw = psram_ybuf_src;//psram_yuv_buf;//+640*120;
	uint8* raw2;
	raw2 = psram_ybuf_src;
	zbar_symbol_type_t typ_mask;
	
	dvp_test_gogle = (struct dvp_device *)dev_get(HG_DVP_DEVID);
	vpp_test_gogle = (struct vpp_device *)dev_get(HG_VPP_DEVID);
	dvp_close(dvp_test_gogle);
	vpp_dis_uv_mode(vpp_test_gogle,1);
	vpp_set_buf0_y_addr(vpp_test_gogle,(uint32)psram_ybuf_src);
	time_now = os_jiffies();
	mask_type = ZBAR_NONE;
	typ_mask = ZBAR_NONE;
	dvp_open(dvp_test_gogle);
	vs_isr = 0;
	while(1){
		zbar_image_scanner_t *scanner = NULL;
		zbar_image_scanner_t *zbar_image_scanner_create ();
		while(vs_isr != 2){
//			yuv_data_deal(512,384);
		}

		vs_isr = 0;
		if(mask_type != ZBAR_NONE){
			if((os_jiffies()-time_mask) > TIME_FOR_KEEP)	
				mask_type = ZBAR_NONE;
		}
		
		if((mask_type != ZBAR_NONE) && (mask_type != ZBAR_QRCODE))
		{
			
		}

		dvp_close(dvp_test_gogle);
		count++;
		sys_dcache_clean_invalid_range((void *)psram_ybuf_src, 307200);
		yuv_data_deal(psram_ybuf_src);	
		scanner = zbar_image_scanner_create();
		/* configure the reader */
		zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);

		zbar_image_t *image = zbar_image_create();
		//printf("%s  %d\n",__func__,__LINE__);
		zbar_image_set_format(image, *(int*)"Y800");
		zbar_image_set_size(image, width, height);
		zbar_image_set_data(image, raw, width * height, zbar_image_free_data);
		
		/* scan the image for barcodes */
		int n = zbar_scan_image(scanner, image);
		
		/* extract results */
		const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
		if(symbol == NULL){
			typ_mask = ZBAR_NONE;
		}
		
		for(; symbol; symbol = zbar_symbol_next(symbol)) {
			/* do something useful with results */
			zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
			typ_mask = typ;
			const char *data = zbar_symbol_get_data(symbol);
			printf("decoded %s symbol \"%s\r\n",
				   zbar_get_symbol_name(typ), data);
		
		}
		if((typ_mask == ZBAR_EAN2)||(typ_mask == ZBAR_EAN5)||(typ_mask == ZBAR_EAN8)||(typ_mask == ZBAR_EAN13)||(typ_mask == ZBAR_QRCODE)||(typ_mask == ZBAR_CODE128)){
			mask_type = typ_mask;
			time_mask = os_jiffies();
		}
		
		_os_printf("zbar end:%d   %d\r\n",os_jiffies()-time_now,sysheap_freesize(&sram_heap));	
		time_now = os_jiffies();
	    zbar_image_destroy(image);
		_os_printf("%s %d\r\n",__func__,__LINE__);
	    zbar_image_scanner_destroy(scanner);
		_os_printf("%s %d\r\n",__func__,__LINE__);
		dvp_open(dvp_test_gogle);

	}
}

void zbar_thread_init(){
	OS_TASK_INIT("handle_zbar", &handle_zbar_task, zbar_test, NULL, OS_TASK_PRIORITY_NORMAL, 4096);
}

#endif
