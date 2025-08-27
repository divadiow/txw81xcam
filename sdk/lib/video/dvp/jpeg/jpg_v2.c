#include "sys_config.h"
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "devid.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "hal/jpeg.h"
#include "dev/scale/hgscale.h"
#include "osal/semaphore.h"
#include "openDML.h"
#include "osal/mutex.h"
#include "custom_mem/custom_mem.h" 
//#include "media.h"
//#include "lib/common/common.h"
#include "stream_frame.h"
#include "utlist.h"
#include "jpgdef.h"
#if PRC_EN
#include "hal/prc.h"
#endif
#include "osal/task.h"
extern struct os_mutex m2m1_mutex;
extern void soft_to_jpg_cfg(uint32 w,uint32 h);



/* soft to jpg run*/
static volatile uint8  stjr_itk=0;
#if PRC_EN
static uint32 sw_jpg_w, sw_jpg_h;
static uint32 stjr_linebuf;
static uint32 stjr_yuvbuf;
static volatile uint32 stjr_tick;
static struct prc_device *stjr_prc_dev;
static struct jpg_device *stjr_jpeg_dev;
#endif
extern volatile uint8_t vfx_open;
#if JPG_EN


static uint8_t *global_jpg0_buf = NULL;
static uint8_t *global_jpg1_buf = NULL;



extern struct dvp_device *dvp_test;
extern uint8_t qc_mode;
extern Vpp_stream photo_msg;
extern uint8 *yuvbuf;
extern FIL fp_jpg;


struct list_head* get_frame(uint8 jpgid);


uint8 *psram_jpg0_buf;
uint8 *psram_jpg1_buf;

struct os_semaphore *jpg_sema_pd = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define JPEG_FRAME_NUM     2


volatile struct list_head free_tab[JPG_NUM];			//空闲列表，存放空间节点

volatile struct list_head *jpg_p[JPG_NUM];			//jpg模块的节点指针
volatile struct list_head *usr_p[JPG_NUM];			//应用的节点指针

volatile jpeg_frame jpg_frame[JPG_NUM][JPEG_FRAME_NUM];			//最大frame数量
volatile struct list_head* jpg_f_p[JPG_NUM];			//当前jpg所使用的frame

volatile mjpeg_node jpg0_node_src[JPG0_NODE]; 
volatile mjpeg_node jpg1_node_src[JPG1_NODE];

volatile uint32 outbuff_isr[JPG_NUM] = {0x0,0x0};
volatile int8 default_qt[JPG_NUM] = {0xf, 0xf};
struct jpg_device *jpeg_dev_global[JPG_NUM];

volatile uint8 psram_example_buf[38548] __attribute__ ((aligned(4),section(".psram.src")));
extern uint8 psram_ybuf_src[IMAGE_H*IMAGE_W+IMAGE_H*IMAGE_W/2];

volatile int8_t g_dqtable_index = DQT_DEF;

/**@brief
 * 将传入的frame根节点下的buf池都送回空间池（free_tab）中
 */
bool free_get_node_list(volatile struct list_head *head,volatile struct list_head *free){
	if(list_empty((struct list_head *)head)){
		return 0;
	}	

	list_splice_init((struct list_head *)head,(struct list_head *)free);
	return 1;
}


/**@brief 
 * 从初始化的frame根节点中抽取其中一个空闲的根节点进行使用，并标记frame的使用状态为正在使用
 * @param 是否开启抢占模式，如果开启，则将上一帧已完成的帧节点删掉，并返回
 */


volatile struct list_head* get_new_frame_head(uint8 jpgnum,int grab){
	uint8 frame_num = 0;
	for(frame_num = 0;frame_num < JPEG_FRAME_NUM;frame_num++){
		if(jpg_frame[jpgnum][frame_num].usable == 0){
			jpg_frame[jpgnum][frame_num].usable = 1;
			return &jpg_frame[jpgnum][frame_num].list;
		}

	}

	if(grab)								//是否开启抢占模式,开启抢占后，肯定有frame返回，上一帧没使用的frame肯定usable为2
	{
		
		for(frame_num = 0;frame_num < JPEG_FRAME_NUM;frame_num++){
			if(jpg_frame[jpgnum][frame_num].usable == 2){
				jpg_frame[jpgnum][frame_num].usable = 1;
				free_get_node_list(&jpg_frame[jpgnum][frame_num].list,&free_tab[jpgnum]);
				return &jpg_frame[jpgnum][frame_num].list;
			}
		}
	}
	
	return 0;
}



/**@brief 
 * 获取当前frame根节点中保存成功的frame，返回根节点以供链表查询
 */

extern struct list_head *get_node(volatile struct list_head *head,volatile struct list_head *del);
extern uint32 get_addr(volatile struct list_head *list);



void set_frame_ready(jpeg_frame *jf){
	jf->usable = 2;
}


/**@brief 
 * 将传入的frame根节点状态调整为空闲，以供下次重新获取
 */
void del_frame(uint8 jpg_num,volatile struct list_head* frame_list)
{


	jpeg_frame* fl;
	uint32_t flags;
	if(list_empty((struct list_head *)frame_list) != TRUE){
		//NVIC_DisableIRQ(MJPEG01_IRQn);
	
		flags = disable_irq();
		free_get_node_list(frame_list,&free_tab[jpg_num]);
		fl = list_entry((struct list_head *)frame_list,jpeg_frame,list);
		fl->usable = 0;
		//NVIC_EnableIRQ(MJPEG01_IRQn);
		enable_irq(flags);
		return;		

	}else{
		//NVIC_DisableIRQ(MJPEG01_IRQn);
		flags = disable_irq();
		fl = list_entry((struct list_head *)frame_list,jpeg_frame,list);
		fl->usable = 0;
		enable_irq(flags);
		//NVIC_EnableIRQ(MJPEG01_IRQn);
		return;
	}


}


/**@brief 
 * 初始化free_tab池，将mjpeg_node的节点都放到空闲池中，供frame后面提取使用
 * @param ftb 空闲池头指针
 * @param jpn mjpeg_node节点源
 * @param use_num  mjpeg_node的数量，即多少个mjpeg_node放到空闲池
 * @param addr   mjpeg_node的总buf起始地址
 * @param buf_len  每个mjpeg_node所关联到的数据量
 */
void free_table_init(uint8 jpgnum,volatile struct list_head *ftb, volatile mjpeg_node* jpn,int use_num,uint32 addr,uint32 buf_len){
	int itk;
	for(itk = 0;itk < use_num;itk++){
		if(jpgnum == 0)
			jpn->buf_addr = (uint8*)(addr + itk*(buf_len+JPG0_TAIL_RESERVER)+JPG0_HEAD_RESERVER);
		else
			jpn->buf_addr = (uint8*)(addr + itk*(buf_len+JPG1_TAIL_RESERVER)+JPG1_HEAD_RESERVER);

		list_add_tail((struct list_head *)&jpn->list,(struct list_head *)ftb); 
		jpn++;
	}
}


/**@brief 
 * 从空间池中提取一个节点，放到队列中，并将此节点作为返回值返回
 */
struct list_head *get_node(volatile struct list_head *head,volatile struct list_head *del){
	if(list_empty((struct list_head *)del)){
		return 0;
	}

	list_move((struct list_head *)del->next,(struct list_head *)head);

	return head->next;				//返回最新的位置
}


int get_node_count(volatile struct list_head *head)
{
	int count = 0;
	struct list_head *first = (struct list_head *)head;
	while(head->next != first)
	{
		head = head->next;
		count++;
	}

	return count;				//返回最新的位置
}



/**@brief 
 * 从当前使用的节点回放到消息池中
 */
bool put_node(volatile struct list_head *head,volatile struct list_head *del){
	//portINIT_CRITICAL();
	if(list_empty((struct list_head *)del)){
		return 0;
	}
	uint32_t flags;
	flags = disable_irq();
	//NVIC_DisableIRQ(MJPEG01_IRQn);
	list_move(del->next,(struct list_head *)head);
	enable_irq(flags);
	//NVIC_EnableIRQ(MJPEG01_IRQn);

	return 1;
}

/**@brief 
 * 获取当前jpeg节点buf地址进行返回
 */
uint32 get_addr(volatile struct list_head *list){
	mjpeg_node* mjn;
	mjn = list_entry((struct list_head *)list,mjpeg_node,list);
	return (uint32)mjn->buf_addr;
}

void jpg_DQT_updata(struct jpg_device *p_jpg, uint8 upOdown, uint8 diff)
{
	uint32 *ptable;
	static int8 pdqt_tab = DQT_DEF;
	if (upOdown == 1)
	{
		pdqt_tab -= diff;
		if (pdqt_tab < 0)
			pdqt_tab = 0;
	}
	else
	{
		pdqt_tab += diff;
		if (pdqt_tab > DQT_MAX_INDEX)
			pdqt_tab = DQT_MAX_INDEX;
	}
	g_dqtable_index = pdqt_tab;
	ptable = (uint32 *)quality_tab[pdqt_tab];
	jpg_updata_dqt(p_jpg, ptable);
}

int jpg_quality_pidCtrl(int diff, int p, int i, int d)
{
	static int diff_prev = 0;
	static int diff_sum = 0;
	int res = 0;
	res = p * diff + i * diff_sum + d * (diff - diff_prev);
	diff_sum += diff;
	diff_prev = diff;
	res = res>>16;
	res = LIMITING(res, 120, -120);
	diff_sum = LIMITING(diff_sum, 2000, -2000);
	//_os_printf("diff_sum:%d\n",diff_sum);
	return res;
}

extern volatile uint32 hs_isr;
volatile uint8 rtp_speed = 0; // 1:降低    2:提升

uint8 jpg_quality_tidy(uint8 jpgnum, uint32 len, uint8 *dqt_index_diff)
{
	uint8 updata_dqt = 0;
	if (qc_mode)
	{
		if (len >= 15 * 1024)
		{
			default_qt[jpgnum]++;
			if (default_qt[jpgnum] == 0x10)
			{
				default_qt[jpgnum] = 0x8;
				updata_dqt = 2;
			}
		}
		else if (len <= 5 * 1024)
		{
			default_qt[jpgnum]--;
			if (default_qt[jpgnum] == 0)
			{
				default_qt[jpgnum] = 8;
				updata_dqt = 1;
			}
		}
		return updata_dqt;
	}
#if 0
	if(len >= 25*1024){
		default_qt[jpgnum]++;
		if(default_qt[jpgnum] == 0x10){
			if(g_dqtable_index < DQT_MAX_INDEX) {
				default_qt[jpgnum] = 0x8;
				updata_dqt = 2;
			}
			else
				default_qt[jpgnum] = 0xf;
		}
	}else if(len <= 15*1024){
		default_qt[jpgnum]--;	
		if(default_qt[jpgnum] == 0){
			if(g_dqtable_index > 0) {
				default_qt[jpgnum] = 8;	
				updata_dqt = 1;
			}
			else
				default_qt[jpgnum] = 0x1;
		}	
	}
	*dqt_index_diff = 1;
#elif 1
	int jpg_len_diff = len - TARGET_JPG_LEN;
	int res = jpg_quality_pidCtrl(jpg_len_diff, QUALITY_CTRL_P, QUALITY_CTRL_I, QUALITY_CTRL_D);
	uint8 qt_diff = os_abs(res) % 0x10;
	uint8 qt_index_diff = os_abs(res) / 0x10;
	*dqt_index_diff = qt_index_diff;
	if (res > 0)
	{
		if (qt_diff + default_qt[jpgnum] > 0xf)
		{
			if ((g_dqtable_index + qt_index_diff + 1 + (qt_diff - (0xf - default_qt[jpgnum]))/8) > DQT_MAX_INDEX)
			{
				*dqt_index_diff = DQT_MAX_INDEX - g_dqtable_index;
				default_qt[jpgnum] = 0xf;
			}
			else
			{
				default_qt[jpgnum] = 0x8 + (qt_diff - (0xf - default_qt[jpgnum]))%8;
				*dqt_index_diff = qt_index_diff + 1 + (qt_diff - (0xf - default_qt[jpgnum]))/8;
			}
		}
		else
			default_qt[jpgnum] += qt_diff;
		if ((*dqt_index_diff) >= 1 && g_dqtable_index < DQT_MAX_INDEX)
		{
			*dqt_index_diff = ((*dqt_index_diff) > (DQT_MAX_INDEX - g_dqtable_index)) ? (DQT_MAX_INDEX - g_dqtable_index) : (*dqt_index_diff);
			updata_dqt = 2;
		}
	}
	else if(res < 0)
	{
		if (default_qt[jpgnum] - qt_diff < 0)
		{
			if ((g_dqtable_index - (qt_index_diff + 1 + (qt_diff-default_qt[jpgnum])/8)) < 0)
			{
				*dqt_index_diff = g_dqtable_index;
				default_qt[jpgnum] = 0;
			}
			else
			{
				default_qt[jpgnum] = 0x8 - (qt_diff - default_qt[jpgnum])%8; //(qt_diff-default_qt[jpgnum]) maybe > 0x8
				*dqt_index_diff = qt_index_diff + 1 + (qt_diff-default_qt[jpgnum])/8;
			}
		}
		else
			default_qt[jpgnum] -= qt_diff;
		if ((*dqt_index_diff) >= 1 && g_dqtable_index > 0)
		{
			*dqt_index_diff = ((*dqt_index_diff) > (g_dqtable_index)) ? g_dqtable_index : (*dqt_index_diff);
			updata_dqt = 1;
		}
	}
	if(updata_dqt == 0)
		*dqt_index_diff = 0;
	// _os_printf("pid res:%d %d %d %d %d %d\n", jpg_len_diff, res, qt_diff, qt_index_diff, *dqt_index_diff, updata_dqt);
#else

	if (len >= 35 * 1024)
	{
		default_qt++;
		if (default_qt == 0x10)
		{
			default_qt = 0x8;
			updata_dqt = 2;
		}
	}
	else
	{
		if (rtp_speed == 1)
		{
			default_qt++;
			if (default_qt == 0x10)
			{
				default_qt = 0x8;
				updata_dqt = 2;
			}
		}
		else if (rtp_speed == 2)
		{
			default_qt--;
			if (default_qt == 0)
			{
				default_qt = 8;
				updata_dqt = 1;
			}
		}
		rtp_speed = 0;
	}
#endif
	return updata_dqt;
}



//获取实际内容buf的长度
int get_jpg_node_len(uint8 jpg_num)
{
	if(jpg_num == 0)
	{
		return JPG0_BUF_LEN-JPG0_HEAD_RESERVER;
	}
	else
	{
		return JPG1_BUF_LEN-JPG1_HEAD_RESERVER;
	}
}



uint32 get_jpg_node_len_new(void *get_f)
{
	jpeg_frame* jpg_frame = (jpeg_frame*)get_f;
	if(jpg_frame->jpg_num == 0)
		return JPG0_BUF_LEN-JPG0_HEAD_RESERVER;
	else
		return JPG1_BUF_LEN-JPG1_HEAD_RESERVER;
}

extern volatile uint8 scale_take_photo;


#if 0
extern uint8 video_psram_mem[616320];
extern uint8 video_psram_mem1[616320];

void location_reset(uint8_t *inbuf,uint8_t *outbuf,int x,int y,int w){
	int xn,yn;
	uint8_t yd,ud,vd;
	//float angle;
	//angle = (30*3.14)/180;
	y = y*-1;
	xn = (x * 866 - y * 499)/1000;
	yn = (x * 499 + y * 866)/1000;
	yn = yn*-1;
	
	if(xn < 0){
		return;
	}

	if(xn > 640){
		return;
	}

	if(yn < 0){
		return;
	}

	//yd = inbuf[x+y*w];
	//ud = inbuf[307200+x/2+(y/2)*320];
	//vd = inbuf[384000+x/2+(y/2)*320];

	
	outbuf[xn+yn*w]               = inbuf[x+y*w];
	outbuf[307200+xn/2+(yn/2)*320]= inbuf[307200+x/2+(y/2)*320];
	outbuf[384000+xn/2+(yn/2)*320]= inbuf[384000+x/2+(y/2)*320];
	//if(xn+1 < 640)
	{
		if((unsigned char)outbuf[xn+1+yn*w] == 0x80){
			outbuf[xn+1+(int)yn*w]                    = inbuf[x+y*w];
			//if((xn+1)%2 == 0){
			//	outbuf[307200+xn/2+1+(yn/2)*320]	   = ud;
			//	outbuf[384000+xn/2+1+(yn/2)*320]= vd;
			//}
			
		}
	}
	
}
#endif

#if 0
//认证或者用户学习使用
void jpeg_user(uint32* pixel_itp)				//应用层
{
	//#define TAKE_PHOTO_HIGH    1080//1080//2560//4320
	//#define TAKE_PHOTO_WIDTH   1920//1920//3840//7680
	uint16_t itp_w,itp_h;
	uint8_t  continous_spon;
	struct scale_device *scale_dev;
	struct lcdc_device *lcd_dev;
	struct vpp_device *vpp_dev;
	uint8  jpg_chose;
	uint32 pingpang = 0;
	uint32 count = 0;
	uint32 flen;
	uint32 itk;
	struct list_head* get_f;
	mjpeg_node* mjn;
	jpeg_frame* jpf;
	uint8 printf_dat = 0;
	struct jpg_device *jpeg_dev;
	jpeg_dev = (struct jpg_device *)dev_get(HG_JPG0_DEVID);	
	scale_dev = (struct scale_device *)dev_get(HG_SCALE1_DEVID);
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
	itp_w = pixel_itp[0]&0xffff;
	itp_h = (pixel_itp[0]>>16)&0xffff;
	continous_spon = pixel_itp[1];
	scale_close(scale_dev);
	jpg_close(jpeg_dev);
	os_sleep_ms(100);
	jpg_cfg(HG_JPG0_DEVID,SCALER_DATA);
	_os_printf("itp_w:%d itp_h:%d   continous_spon:%d\r\n",itp_w,itp_h,continous_spon);
	photo_msg.out0_w = itp_w;
	photo_msg.out0_h = itp_h;
	jpg_start(HG_JPG0_DEVID);
	if(itp_h <= 720){
		scale_from_vpp_to_jpg(scale_dev,yuvbuf,photo_msg.in_w,photo_msg.in_h,itp_w,itp_h);
	}

	while(1){
		if(itp_h > 720){
			if(m2m1_mutex.magic == 0xa8b4c2d5)
				os_mutex_lock(&m2m1_mutex,osWaitForever);
			
			lcdc_video_enable_auto_ks(lcd_dev,1);
			scale_from_soft_to_jpg(scale_dev,psram_ybuf_src,photo_msg.in_w,photo_msg.in_h,itp_w,itp_h);
			scale_take_photo = 0;
			while(scale_take_photo == 0){
				os_sleep_ms(2);
			}

			if(m2m1_mutex.magic == 0xa8b4c2d5)
				os_mutex_unlock(&m2m1_mutex);
			
			os_sleep_ms(10);
			lcdc_video_enable_auto_ks(lcd_dev,0);
			vpp_open(vpp_dev);	
		}

		
		get_f = get_frame(HG_JPG0_DEVID);
		jpg_chose = 0;

		if(get_f){
			fatfs_create_jpg();
			_os_printf("usr(%d)====>get_f:%x\r\n",jpg_chose,(uint32)get_f);
			count++;
			usr_p[jpg_chose] = get_f;
			
			jpf = list_entry(get_f,jpeg_frame,list);
			flen = jpf->frame_len;
			
			_os_printf("flen_write:%d\r\n",flen);
			while(list_empty((struct list_head *)usr_p[jpg_chose]) != TRUE)
			{
				mjn = list_entry((struct list_head *)usr_p[jpg_chose]->next,mjpeg_node,list);
				
				//_os_printf("mjn:%x\r\n",mjn);
				/*
					此处使用当前mjn，获取节点对应的buf数据位置，对数据进行处理
				*/
				if(jpg_chose){
					if(flen > JPG1_BUF_LEN){
						//fatfs_write_data(mjn->buf_addr,JPG1_BUF_LEN);
						osal_fwrite(mjn->buf_addr,1,JPG1_BUF_LEN,&fp_jpg);
						flen -= get_jpg_node_len(jpg_chose);
					}
					else{
						//for(itk = 0;itk < flen+JPG1_HEAD_RESERVER;itk++){
						//	_os_printf("%02x ",mjn->buf_addr[itk-JPG1_HEAD_RESERVER]);
						//}
						//fatfs_write_data(mjn->buf_addr,flen);
						osal_fwrite(mjn->buf_addr,1,flen,&fp_jpg);
						flen = 0;
					}
				}else{
					if(flen > JPG0_BUF_LEN){
						//for(itk = 0;itk < JPG0_BUF_LEN;itk++){
						//	_os_printf("%02x ",mjn->buf_addr[itk-JPG0_HEAD_RESERVER]);
						//}
						//fatfs_write_data(mjn->buf_addr,JPG0_BUF_LEN);
						osal_fwrite(mjn->buf_addr,1,JPG0_BUF_LEN,&fp_jpg);
						flen -= get_jpg_node_len(jpg_chose);
					}
					else{
						//for(itk = 0;itk < flen+JPG0_HEAD_RESERVER;itk++){
						//	_os_printf("%02x ",mjn->buf_addr[itk-JPG0_HEAD_RESERVER]);
						//}
						//fatfs_write_data(mjn->buf_addr,flen);
						osal_fwrite(mjn->buf_addr,1,flen,&fp_jpg);
						flen = 0;
					}
				}								
				put_node(&free_tab[jpg_chose],usr_p[jpg_chose]);				//节点归还到free_tab,边处理边给下一帧释放空间
			}
			_os_printf("flen_write_end:%d\r\n",flen);
			del_frame(jpg_chose,usr_p[jpg_chose]);							//删除frame
			fatfs_close_jpg();
			if(count == continous_spon)
				break;
		}
	}
	csi_kernel_task_del(csi_kernel_task_get_cur());
}
#endif
extern void take_photo_thread(void *d);
uint32_t takephoto_arg[3] = {0};
void take_photo_thread_init(uint16_t w,uint16_t h,uint8_t continuous_spot){
	takephoto_arg[0] = (w&0xffff) | ((h<<16)&0xffff0000);
	takephoto_arg[1] = continuous_spot;
	takephoto_arg[2] = 1;
	os_task_create("take_photo_thread", take_photo_thread, (void*)&takephoto_arg, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
}

uint32_t get_takephoto_thread_status()
{
	return takephoto_arg[2];
}




void jpg_room_init(uint8 jpgnum){
	//INIT_LIST_HEAD(&jpg_node);
	uint8 *buf; 

	if(jpgnum == 0)
	{

		if(global_jpg0_buf)
		{
			os_printf("0please custom_free jpg_buf when custom_malloc again\n");
			return ;
		}

		#ifdef PSRAM_HEAP
		buf = (uint8*)custom_malloc_psram(JPG0_NODE*(JPG0_BUF_LEN+JPG0_TAIL_RESERVER));
		#else
		buf = (uint8*)custom_malloc(JPG0_NODE*(JPG0_BUF_LEN+JPG0_TAIL_RESERVER));
		#endif
		global_jpg0_buf = buf;	
		//sys_dcache_clean_invalid_range((void *)buf, JPG0_NODE*(JPG0_BUF_LEN+JPG0_TAIL_RESERVER));	
	}
	else
	{

		if(global_jpg1_buf)
		{
			os_printf("1please custom_free jpg_buf when custom_malloc again\n");
			return ;
		}
		#ifdef PSRAM_HEAP
		buf = (uint8*)custom_malloc_psram(JPG1_NODE*(JPG1_BUF_LEN+JPG1_TAIL_RESERVER));
		#else
		buf = (uint8*)custom_malloc(JPG1_NODE*(JPG1_BUF_LEN+JPG1_TAIL_RESERVER));
		#endif
		global_jpg1_buf = buf;
		//sys_dcache_clean_invalid_range((void *)buf, JPG1_NODE*(JPG1_BUF_LEN+JPG1_TAIL_RESERVER));
	}

	while(!buf)
	{
		os_printf("%s:%d\n",__FUNCTION__,__LINE__);
		os_sleep_ms(1000);
	}
	
	INIT_LIST_HEAD((struct list_head *)&jpg_frame[jpgnum][0].list);
	INIT_LIST_HEAD((struct list_head *)&jpg_frame[jpgnum][1].list);
	INIT_LIST_HEAD((struct list_head *)&free_tab[jpgnum]);

	if(jpgnum == 0)
		free_table_init(jpgnum,&free_tab[jpgnum],(mjpeg_node*)&jpg0_node_src,JPG0_NODE,(uint32)buf,JPG0_BUF_LEN);
	else
		free_table_init(jpgnum,&free_tab[jpgnum],(mjpeg_node*)&jpg1_node_src,JPG1_NODE,(uint32)buf,JPG1_BUF_LEN);
	jpg_frame[jpgnum][0].usable = 0;
	jpg_frame[jpgnum][1].usable = 0;




	jpg_frame[jpgnum][0].jpg_num = jpgnum;
	jpg_frame[jpgnum][1].jpg_num = jpgnum;


	jpg_f_p[jpgnum] = get_new_frame_head(jpgnum,1);								//预先分配第一frame，待帧结束的时候再分配下一frame			
	jpg_p[jpgnum] = jpg_f_p[jpgnum];
}


uint8_t jpg_room_deinit(uint8 jpgnum)
{
	if(jpgnum == 0)
	{
		if(global_jpg0_buf)
		{
			#ifndef PSRAM_HEAP
			custom_free(global_jpg0_buf);
			#else
			custom_free_psram(global_jpg0_buf);
			#endif
			global_jpg0_buf = NULL;
		}
	}
	else if(jpgnum == 1)
	{
		if(global_jpg1_buf)
		{
			#ifndef PSRAM_HEAP
			custom_free(global_jpg1_buf);
			#else
			custom_free_psram(global_jpg1_buf);
			#endif
			global_jpg1_buf = NULL;
		}
	}

	return TRUE;
}


/*********************************
			获取分辨率
0:无效
1:vga(640x480)
2:720p(1280x720)
*********************************/
uint8 get_camera_dpi()
{
	return 1;
}


void jpg_outbuff_full_isr(uint32 irq_flag,uint32 irq_data,uint32 param1,uint32 param2){
	uint8 jpg_chose;
	uint32 addr;
	struct jpg_device *p_jpg = (struct jpg_device *)irq_data;
	if(p_jpg == jpeg_dev_global[0]){
		jpg_chose = 0;
	}else{
		jpg_chose = 1;
	}
	
	
	if(outbuff_isr[jpg_chose] != 0xff)
		outbuff_isr[jpg_chose]++;
	else
		return;


	
	jpg_p[jpg_chose] = get_node(jpg_p[jpg_chose],&free_tab[jpg_chose]);
	if(jpg_p[jpg_chose] == NULL){
		outbuff_isr[jpg_chose] = 0xff;
		return;
	}

//	if((jpg_addr_num%2) == 0){
//		data_cmp_u32(jpg_addr0,psram_example_buf+1284*jpg_addr_num,1284);
//	}else{	
//		data_cmp_u32(jpg_addr1,psram_example_buf+1284*jpg_addr_num,1284);
//	}

//	jpg_addr_num++;

	
	addr = get_addr(jpg_p[jpg_chose]);
	jpg_set_addr(p_jpg,addr,get_jpg_node_len(jpg_chose));
	sys_dcache_clean_invalid_range((uint32_t *)addr, get_jpg_node_len(jpg_chose));
	//os_printf("OF-%x \r\n",addr);
//	if((jpg_addr_num%2) == 1){
//		jpg_addr0 = addr;
//	}else{
//		jpg_addr1 = addr;
//	}
}

void jpg_buf_err(uint32 irq_flag,uint32 irq_data,uint32 param1,uint32 param2){
	uint8 jpg_chose;
	struct scale_device *scale_dev;
	uint32 addr;
	struct jpg_device *p_jpg = (struct jpg_device *)irq_data;
	scale_dev = (struct scale_device *)dev_get(HG_SCALE1_DEVID);
	if(p_jpg == jpeg_dev_global[0]){
		jpg_chose = 0;
	}else{
		jpg_chose = 1;
	}	
	jpg_close(p_jpg);
	if(jpg_chose == 0)
		scale_close(scale_dev);
//	dvp_close(dvp_test);	
	_os_printf("(%d)?\r\n",jpg_chose);
	del_frame(jpg_chose,jpg_f_p[jpg_chose]);
	
	jpg_f_p[jpg_chose] = get_new_frame_head(jpg_chose,1);
	jpg_p[jpg_chose] = jpg_f_p[jpg_chose];
	outbuff_isr[jpg_chose] = 0;

	jpg_p[jpg_chose] = get_node(jpg_p[jpg_chose],&free_tab[jpg_chose]);							//预分配的两个buf地址，帧结束的时候要去掉一个预分配节点
	if(jpg_p[jpg_chose] == NULL){
		_os_printf("need more node for new frame start1\r\n");
		while(1);
	}
	//p_jpg->DMA_TADR0 = get_addr(jpg_p);
	addr = get_addr(jpg_p[jpg_chose]);
	//jpg_set_addr0(p_jpg,addr);
	jpg_set_addr(p_jpg,addr,get_jpg_node_len(jpg_chose));
	sys_dcache_clean_invalid_range((uint32_t *)addr, get_jpg_node_len(jpg_chose));
	jpg_p[jpg_chose] = get_node(jpg_p[jpg_chose],&free_tab[jpg_chose]);
	if(jpg_p[jpg_chose] == NULL){
		_os_printf("need more node for new frame start2\r\n");
		while(1);		
	}	
	//p_jpg->DMA_TADR1 = get_addr(jpg_p);	
	addr = get_addr(jpg_p[jpg_chose]);
	//jpg_set_addr1(p_jpg,addr);
	jpg_set_addr(p_jpg,addr,get_jpg_node_len(jpg_chose));
	sys_dcache_clean_invalid_range((uint32_t *)addr, get_jpg_node_len(jpg_chose));
	jpg_open(p_jpg);
	if(jpg_chose == 0)
		scale_open(scale_dev);	




//	dvp_open(dvp_test);

	
}

void jpg_done_isr(uint32 irq_flag,uint32 irq_data,uint32 param1,uint32 param2){
	uint8 jpg_chose;
	struct jpg_device *p_jpg = (struct jpg_device *)irq_data;

	uint32 addr;
	jpeg_frame* jf;
	uint8 out_buf_num_err = 0;
	uint8 updata_dqt = 0;
	uint8 default_qt_last;
	uint32 jpg_len; 
	
	uint8 dqt_index_diff = 0;
	
	if(p_jpg == jpeg_dev_global[0]){
		jpg_chose = 0;
	}else{
		jpg_chose = 1;
	}	
	
	
	jpg_len = param1;//jpg_get_len(p_jpg);
	//os_printf("len(%d)=>:%d ",jpg_chose,jpg_len);
	//if(outbuff_isr !=  jpg_get_outbuf_num(p_jpg))
	if(param2||(outbuff_isr[jpg_chose] == 255)){
		os_printf("jpg done len err:%d\r\n",outbuff_isr[jpg_chose]);
		out_buf_num_err = 1;
	}
	default_qt_last = default_qt[jpg_chose];
//	os_printf("jpglen:%d %d %d\n", jpg_len, default_qt[jpg_chose], g_dqtable_index);
	updata_dqt = jpg_quality_tidy(jpg_chose, jpg_len, &dqt_index_diff);
	if (default_qt_last != default_qt[jpg_chose])
	{
		jpg_set_qt(p_jpg, default_qt[jpg_chose]);
	}
//	os_printf("quality res:%d %s%d\n", default_qt[jpg_chose], ((updata_dqt>1)?"+":"-"),dqt_index_diff);
	if (get_node_count(&free_tab[jpg_chose]) < 2)
	{
		outbuff_isr[jpg_chose] = 0xff;
	}
/**/
	if(jpg_len < outbuff_isr[jpg_chose] * get_jpg_node_len(jpg_chose)){
		os_printf("BUF len err:%d  %d\r\n",jpg_len,outbuff_isr[jpg_chose] * get_jpg_node_len(jpg_chose));
		del_frame(jpg_chose,jpg_f_p[jpg_chose]);
	}else if(outbuff_isr[jpg_chose] == 0xff){
		os_printf("outbuff_isr err,no room\r\n");
		del_frame(jpg_chose,jpg_f_p[jpg_chose]);
	}else if(out_buf_num_err){
		os_printf("out_buf_num_err happen\r\n");
		del_frame(jpg_chose,jpg_f_p[jpg_chose]);
	}
	else{
		//帧OK
			
		jf = list_entry((struct list_head *)jpg_f_p[jpg_chose],jpeg_frame,list);
		jf->frame_len = jpg_len;
		
		//因为有预分配机制，所以要先去掉最后一个节点
		put_node(&free_tab[jpg_chose],jpg_p[jpg_chose]->prev);
		set_frame_ready(jf);	
		//os_printf("jf_ready:%x jpg_chose:%d\r\n",jf,jpg_chose);
	}

	jpg_f_p[jpg_chose] = get_new_frame_head(jpg_chose,1);
	//os_printf("jpg_f_p[jpg_chose]:%X\n",jpg_f_p[jpg_chose]);
	jpg_p[jpg_chose] = jpg_f_p[jpg_chose];
	outbuff_isr[jpg_chose] = 0;
	//再设置DMA地址
	jpg_p[jpg_chose] = get_node(jpg_p[jpg_chose],&free_tab[jpg_chose]);							//预分配的两个buf地址，帧结束的时候要去掉一个预分配节点
	if(jpg_p[jpg_chose] == NULL){
		_os_printf("need more node for new frame start1\r\n");
		while(1);
	}
	//p_jpg->DMA_TADR0 = get_addr(jpg_p);
	addr = get_addr(jpg_p[jpg_chose]);
	jpg_set_addr(p_jpg,addr,get_jpg_node_len(jpg_chose));
	sys_dcache_clean_invalid_range((uint32_t *)addr, get_jpg_node_len(jpg_chose));
	jpg_p[jpg_chose] = get_node(jpg_p[jpg_chose],&free_tab[jpg_chose]);
	if(jpg_p[jpg_chose] == NULL){
		_os_printf("need more node for new frame start2\r\n");
		while(1);		
	}	
	//os_printf("addr:%x \r\n",addr);
	addr = get_addr(jpg_p[jpg_chose]);
	jpg_set_addr(p_jpg,addr,get_jpg_node_len(jpg_chose));
	sys_dcache_clean_invalid_range((uint32_t *)addr, get_jpg_node_len(jpg_chose));
	if (updata_dqt)
		jpg_DQT_updata(p_jpg, updata_dqt, dqt_index_diff);
}

volatile uint8_t pixel_done = 0;
#if PRC_EN
void jpg_pixel_done(uint32 irq_flag,uint32 irq_data,uint32 param1,uint32 param2){
	pixel_done = 1;
	//为了旧版本兼容,暂时用宏隔离
	#if DVP_EN
	if(!vfx_open) {
		return;
	} 
	#else
	if(0)
	{

	}
	#endif 
	else {
		if(stjr_itk < 30) {
			hw_memcpy((void*)stjr_linebuf,(void*)(stjr_yuvbuf+stjr_itk*16* sw_jpg_w), sw_jpg_w*16);
			prc_set_yaddr(stjr_prc_dev,(uint32)stjr_linebuf);
			hw_memcpy((void*)(stjr_linebuf+ sw_jpg_w*16),(void*)(stjr_yuvbuf+ sw_jpg_w*sw_jpg_h+stjr_itk*4* sw_jpg_w), sw_jpg_w*4);
			prc_set_uaddr(stjr_prc_dev,(uint32)(stjr_linebuf+ sw_jpg_w*16));
			hw_memcpy((void*)(stjr_linebuf+ sw_jpg_w*16+4*sw_jpg_w),(void*)(stjr_yuvbuf+ sw_jpg_w*sw_jpg_h+ sw_jpg_w*sw_jpg_h/4+stjr_itk*4* sw_jpg_w), sw_jpg_w*4);
			prc_set_vaddr(stjr_prc_dev,(uint32)(stjr_linebuf+ sw_jpg_w*16+4* sw_jpg_w));
			jpg_soft_kick(stjr_jpeg_dev,1);
			stjr_itk++;
		} else {
			os_sema_up(jpg_sema_pd);
			stjr_itk = 0;
		}
	}

}
#endif

void jpg_config_addr(struct jpg_device *p_jpg,uint8 jpgnum){
	uint32 addr;
	jpg_p[jpgnum] = get_node(jpg_p[jpgnum],&free_tab[jpgnum]);							//预分配的两个buf地址，帧结束的时候要去掉一个预分配节点
	addr = get_addr(jpg_p[jpgnum]);
	jpg_set_addr(p_jpg,addr,get_jpg_node_len(jpgnum));
	sys_dcache_clean_invalid_range((uint32_t *)addr, get_jpg_node_len(jpgnum));
	jpg_p[jpgnum] = get_node(jpg_p[jpgnum],&free_tab[jpgnum]);
	addr = get_addr(jpg_p[jpgnum]);
	jpg_set_addr(p_jpg,addr,get_jpg_node_len(jpgnum));	
	sys_dcache_clean_invalid_range((uint32_t *)addr, get_jpg_node_len(jpgnum));
}

void jpg_isr_init(struct jpg_device *p_jpg){
	jpg_request_irq(p_jpg,(jpg_irq_hdl )&jpg_outbuff_full_isr,JPG_IRQ_FLAG_JPG_BUF_FULL,p_jpg);
	jpg_request_irq(p_jpg,(jpg_irq_hdl )&jpg_buf_err,JPG_IRQ_FLAG_ERROR,p_jpg);		
	jpg_request_irq(p_jpg,(jpg_irq_hdl )&jpg_done_isr,JPG_IRQ_FLAG_JPG_DONE,p_jpg);
#if	PRC_EN
	jpg_request_irq(p_jpg,(jpg_irq_hdl )&jpg_pixel_done,JPG_IRQ_FLAG_PIXEL_DONE,p_jpg);
#endif
	NVIC_EnableIRQ(MJPEG01_IRQn);
}


//专门为从yuv在psram,然后编码成jpg的模式
//利用prc模块寄存器但不属于prc功能
//为了兼容,仅仅对JPG_IRQ_FLAG_PIXEL_DONE的done的参数修改,其他不变
typedef int32_t (*jpg_pixel_done_prc)(uint32 irq_flag,uint32 irq_data,uint32 param1,uint32 param2);
void jpg_isr_init_prc(struct jpg_device *p_jpg,jpg_pixel_done_prc done_prc,void *param){
	jpg_request_irq(p_jpg,(jpg_irq_hdl )&jpg_outbuff_full_isr,JPG_IRQ_FLAG_JPG_BUF_FULL,p_jpg);
	jpg_request_irq(p_jpg,(jpg_irq_hdl )&jpg_buf_err,JPG_IRQ_FLAG_ERROR,p_jpg);		
	jpg_request_irq(p_jpg,(jpg_irq_hdl )&jpg_done_isr,JPG_IRQ_FLAG_JPG_DONE,p_jpg);
	jpg_request_irq(p_jpg,done_prc,JPG_IRQ_FLAG_PIXEL_DONE,param);
	NVIC_EnableIRQ(MJPEG01_IRQn);
}

//不需要JPG_IRQ_FLAG_PIXEL_DONE中断
void jpg_isr_init_vpp(struct jpg_device *p_jpg){
	jpg_request_irq(p_jpg,(jpg_irq_hdl )&jpg_outbuff_full_isr,JPG_IRQ_FLAG_JPG_BUF_FULL,p_jpg);
	jpg_request_irq(p_jpg,(jpg_irq_hdl )&jpg_buf_err,JPG_IRQ_FLAG_ERROR,p_jpg);		
	jpg_request_irq(p_jpg,(jpg_irq_hdl )&jpg_done_isr,JPG_IRQ_FLAG_JPG_DONE,p_jpg);
	jpg_request_irq(p_jpg,(jpg_irq_hdl)NULL,JPG_IRQ_FLAG_PIXEL_DONE,NULL);
	NVIC_EnableIRQ(MJPEG01_IRQn);
}
 




void jpg_cfg_prc(uint8 jpgid,enum JPG_SRC_FROM src_from,jpg_pixel_done_prc done_prc,void *dev_id){
	uint8 jpg_chose;
	//uint32 itk,jtk;
	_os_printf("JPG start:%d\r\n",src_from);
	struct jpg_device *jpeg_dev;
	if(jpgid == HG_JPG0_DEVID)
		jpg_chose = 0;
	else
		jpg_chose = 1;	
	
	jpeg_dev = (struct jpg_device *)dev_get(jpgid);	
	jpeg_dev_global[jpg_chose] = jpeg_dev;

	//jpg table init
	jpg_init(jpeg_dev,DQT_DEF,default_qt[jpg_chose]);
	
	jpg_set_data_from(jpeg_dev,src_from);
	jpg_set_hw_check(jpeg_dev,1);
	jpg_isr_init_prc(jpeg_dev,done_prc,dev_id);
	jpg_set_init_finish(jpeg_dev,1);
}

void jpg_cfg(uint8 jpgid,enum JPG_SRC_FROM src_from){
	uint8 jpg_chose;
	//uint32 itk,jtk;
	_os_printf("JPG start\r\n");
	struct jpg_device *jpeg_dev;
	if(jpgid == HG_JPG0_DEVID)
		jpg_chose = 0;
	else
		jpg_chose = 1;	
	
	jpeg_dev = (struct jpg_device *)dev_get(jpgid);	
	jpeg_dev_global[jpg_chose] = jpeg_dev;

	//jpg table init
	jpg_init(jpeg_dev,DQT_DEF,default_qt[jpg_chose]);
	
	jpg_set_data_from(jpeg_dev,src_from);
	jpg_set_hw_check(jpeg_dev,1);
	jpg_isr_init(jpeg_dev);
	jpg_set_init_finish(jpeg_dev,1);
}

void jpg_cfg_vpp(uint8 jpgid,enum JPG_SRC_FROM src_from){
	uint8 jpg_chose;
	//uint32 itk,jtk;
	_os_printf("JPG start\r\n");
	struct jpg_device *jpeg_dev;
	if(jpgid == HG_JPG0_DEVID)
		jpg_chose = 0;
	else
		jpg_chose = 1;	
	
	jpeg_dev = (struct jpg_device *)dev_get(jpgid);	
	jpeg_dev_global[jpg_chose] = jpeg_dev;

	//jpg table init
	jpg_init(jpeg_dev,DQT_DEF,default_qt[jpg_chose]);
	
	jpg_set_data_from(jpeg_dev,src_from);
	jpg_set_hw_check(jpeg_dev,1);
	jpg_isr_init_vpp(jpeg_dev);
	jpg_set_init_finish(jpeg_dev,1);
}


static int opcode_func(stream *s,void *priv,int opcode)
{
	int res = 0;
	//_os_printf("%s:%d\topcode:%d\n",__FUNCTION__,__LINE__,opcode);
	switch(opcode)
	{
		case STREAM_OPEN_ENTER:
		break;
		case STREAM_OPEN_EXIT:
		{
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

uint32_t get_jpg_default_dpi(uint8_t id)
{
	if(id == 0)
	{
		return (photo_msg.out0_w<<16 | photo_msg.out0_h);
	}
	else
	{
		return (photo_msg.out1_w<<16 | photo_msg.out1_h);
	}
}

extern uint8_t vga_room[2][640*480+640*480/2];
//uint8_t soft_linebuf[16*640+8*640]__attribute__ ((aligned(4)));
void soft_to_jpg_cfg(uint32 w,uint32 h){	
	photo_msg.out0_h = h;
	photo_msg.out0_w = w;
	jpg_cfg(HG_JPG0_DEVID,SOFT_DATA);
	if(jpg_sema_pd == NULL)
	{
		jpg_sema_pd = os_malloc(sizeof(struct os_semaphore));
		os_sema_init(jpg_sema_pd, 0);
	}
	else
	{
		os_sema_del(jpg_sema_pd);
		os_sema_init(jpg_sema_pd, 0);
	}

}

#if PRC_EN
void soft_to_jpg_run(uint32 yuvbuf,uint32 linebuf,uint32 w,uint32 h){

	stjr_prc_dev = (struct prc_device*)dev_get(HG_PRC_DEVID);
	stjr_jpeg_dev = (struct jpg_device *)dev_get(HG_JPG0_DEVID);	
	stjr_linebuf = linebuf;	
	stjr_yuvbuf = yuvbuf;
	sw_jpg_w = w;
	sw_jpg_h = h;
	stjr_itk = 0;
	stjr_tick = os_jiffies();
	hw_memcpy((void*)(stjr_linebuf),(void*)(stjr_yuvbuf+stjr_itk*16* sw_jpg_w), sw_jpg_w*16);
	prc_set_yaddr(stjr_prc_dev,(uint32)stjr_linebuf);
	hw_memcpy((void*)(stjr_linebuf+ sw_jpg_w*16),(void*)(stjr_yuvbuf+ sw_jpg_w*sw_jpg_h+stjr_itk*4* sw_jpg_w), sw_jpg_w*4);
	prc_set_uaddr(stjr_prc_dev,(uint32)(stjr_linebuf+ sw_jpg_w*16));
	hw_memcpy((void*)(stjr_linebuf+ sw_jpg_w*16+4*sw_jpg_w),(void*)(stjr_yuvbuf+ sw_jpg_w*sw_jpg_h+ sw_jpg_w*sw_jpg_h/4+stjr_itk*4* sw_jpg_w), sw_jpg_w*4);
	prc_set_vaddr(stjr_prc_dev,(uint32)(stjr_linebuf+ sw_jpg_w*16+4* sw_jpg_w));
	stjr_itk++;
	pixel_done = 0;
	jpg_soft_kick(stjr_jpeg_dev,1);
	os_sema_down(jpg_sema_pd, 50);
}


extern void yuv_add_time_watermark(uint8_t *src,uint32_t w,uint32_t h,uint8_t cw,uint8_t ch,uint32_t x_s,uint32_t y_s,uint32_t year,uint8_t month,uint8_t day,uint8_t hour,uint8_t min,uint8_t sec);
extern uint8_t *vfx_linebuf;
void start_jpeg();
void soft_to_jpg_thread(){
	struct data_structure *get_f = NULL;
    struct stream_jpeg_data_s *dest_list;
    struct stream_jpeg_data_s *dest_list_tmp;
	stream *s = NULL;
	uint32_t node_len;
	uint32_t flen;
	uint8_t *jpeg_buf_addr = NULL;
	os_sleep_ms(1000);
	s = open_stream_available(R_RTP_JPEG,0,8,opcode_func,NULL);
	
	soft_to_jpg_cfg(640,480);
	
	if(vfx_linebuf == NULL)
		vfx_linebuf = os_malloc(16*640+8*640);
	if(vfx_linebuf == NULL){
		os_printf("error....vfx malloc...\r\n");
	}
	vfx_open = 1;
	start_jpeg();
	while(1){
		get_f = recv_real_data(s);
		if(get_f){
			dest_list = (struct stream_jpeg_data_s *)GET_DATA_BUF(get_f);
			dest_list_tmp = dest_list;
			flen = get_stream_real_data_len(get_f);
			node_len = GET_NODE_LEN(get_f);
			jpeg_buf_addr = (uint8_t *)GET_JPG_BUF(get_f);
			free_data(get_f);
			get_f = NULL;

		}else{
			os_sleep_ms(15);
			yuv_add_time_watermark((uint8_t *)vga_room,640,480,12,32,400,440,2001,12,1,13,1,59);
			soft_to_jpg_run((uint32)vga_room,(uint32)vfx_linebuf,640,480);
		}
	}
}

void softjpg_thread_init(){
	os_task_create("handle_softjpg", soft_to_jpg_thread, (void*)NULL, OS_TASK_PRIORITY_NORMAL, 0, NULL, 1024);
}
#endif

#if 0

void jpg_soft_run_demo(){
	struct data_structure *get_f = NULL;
    struct stream_jpeg_data_s *dest_list;
    struct stream_jpeg_data_s *dest_list_tmp;
	struct stream_jpeg_data_s *el,*tmp;
	stream *s = NULL;
	uint32_t node_len;
	uint32_t flen;
	uint32_t num = 0;
	uint8_t *jpeg_buf_addr = NULL;
	void *fp;
	os_sleep_ms(1000);
	s = open_stream_available(R_RTP_JPEG,0,8,opcode_func,NULL);
	soft_to_jpg_cfg(640,480);
	start_jpeg();
	while(1){
		get_f = recv_real_data(s);
		if(get_f){
			os_printf("JPG gen\r\n");
			dest_list = (struct stream_jpeg_data_s *)GET_DATA_BUF(get_f);
			dest_list_tmp = dest_list;
			flen = get_stream_real_data_len(get_f);
			node_len = GET_NODE_LEN(get_f);
			jpeg_buf_addr = (uint8_t *)GET_JPG_BUF(get_f);

			os_printf("jpeg_buf_addr:%x  len:%d\r\n",jpeg_buf_addr,node_len);
			free_data(get_f);
			get_f = NULL;

		}else{
			os_sleep_ms(15);
			soft_to_jpg_run(decode_vga,640,480);
		}
	}

}
#endif


//jpg重新配置,如果设备处于休眠状态,则可能需要等待休眠结束才可以重新启动mjpeg
void jpg_recfg(uint8 jpg_num)
{
	uint32_t jpg_id;
	uint32_t count = 0;
	jpg_id = jpg_num + HG_JPG0_DEVID;
	struct jpg_device *jpeg_dev;
	jpeg_dev = (struct jpg_device *)dev_get(jpg_id);	
	//第二bit代表jpg初始化完成
	while(!(jpg_get_init_flag(jpeg_dev)) && count++<3000)
	{
		os_sleep_ms(1);
	}
	os_printf("jpg_is_online(jpeg_dev):%X\n",jpg_is_online(jpeg_dev));
	jpg_room_init(jpg_num);
	jpg_config_addr(jpeg_dev,jpg_num);
}



void jpg_start(uint8 jpg_num){
	struct jpg_device *jpeg_dev;
	uint32_t jpgid;
	jpgid = jpg_num + HG_JPG0_DEVID;
	jpeg_dev = (struct jpg_device *)dev_get(jpgid);	

	//jpg csr config
	if(jpgid == HG_JPG0_DEVID){
		#if BBM_DEMO
			jpg_set_size(jpeg_dev,photo_msg.out1_h,photo_msg.out1_w);
		#else
			jpg_set_size(jpeg_dev,photo_msg.out0_h,photo_msg.out0_w);
		#endif
	}
	else{
		jpg_set_size(jpeg_dev,photo_msg.out0_h,photo_msg.out0_w);
	}
	jpg_open(jpeg_dev);	
	os_printf("mj1 open\r\n");
}

//w和h 设置0,代表和源头分辨率一致
void set_jpg_w_h(uint8 jpg_num,uint16_t w,uint16_t h)
{
	if(jpg_num == 0)
	{
		if(w == 0 && h == 0)
		{
			photo_msg.out0_w = photo_msg.in_w;
			photo_msg.out0_h = photo_msg.in_h;
		}
		else
		{
			photo_msg.out0_w = w;
			photo_msg.out0_h = h;
		}

	}
	else
	{
		if(w == 0 && h == 0)
		{
			photo_msg.out1_w = photo_msg.in_w;
			photo_msg.out1_h = photo_msg.in_h;
		}
		else
		{
		 	photo_msg.out1_w = w;
		 	photo_msg.out1_h = h;
		}

	}
}



void jpg_start_dpi(uint8 jpg_num,uint16_t w,uint16_t h){
	struct jpg_device *jpeg_dev;
	uint32_t jpgid;
	jpgid = jpg_num + HG_JPG0_DEVID;
	jpeg_dev = (struct jpg_device *)dev_get(jpgid);	

	if(jpg_num == 0)
	{
		photo_msg.out0_w = w;
		photo_msg.out0_h = h;
	}
	else
	{
		photo_msg.out1_w = w;
		photo_msg.out1_h = h;
	}

	os_printf("w:%d\th:%d\n",w,h);
	//jpg csr config
	if(jpeg_dev)
	{
		jpg_set_size(jpeg_dev,h,w);
		jpg_open(jpeg_dev);	
	}

	os_printf("mj1 open\r\n");
}

void jpg_stop(uint32_t jpg_num)
{
	uint32_t jpg_id;
	jpg_id = jpg_num + HG_JPG0_DEVID;
	struct jpg_device *jpeg_dev;
	jpeg_dev = (struct jpg_device *)dev_get(jpg_id);	
	jpg_close(jpeg_dev);
	jpg_room_deinit(jpg_num);
}

void jpg_clear_init_flag(uint32_t jpg_num)
{
	uint32_t jpg_id;
	struct jpg_device *jpeg_dev;
	jpg_id = jpg_num + HG_JPG0_DEVID;
	jpeg_dev = (struct jpg_device *)dev_get(jpg_id);
	jpg_set_init_finish(jpeg_dev,0);
}



void del_jpeg_frame(void *d)
{
	jpeg_frame* get_f = (jpeg_frame*)d;
	del_frame(get_f->jpg_num,(struct list_head*)get_f);

}

struct list_head* get_frame(uint8 jpgid)
{
	uint8 frame_num = 0;
	uint8 jpg_chose;
	if(jpgid == HG_JPG0_DEVID)
		jpg_chose = 0;
	else
		jpg_chose = 1;
	
	for(frame_num = 0;frame_num < JPEG_FRAME_NUM;frame_num++){
		if(jpg_frame[jpg_chose][frame_num].usable == 2){
			jpg_frame[jpg_chose][frame_num].usable = 1;
			return (struct list_head *)&jpg_frame[jpg_chose][frame_num].list;
		}
	}

	return NULL;	
}

uint32 get_jpeg_len(void *d)
{
	uint32 flen;
	struct list_head* get_f = (struct list_head*)d;
	jpeg_frame* jpf;
	jpf = list_entry(get_f,jpeg_frame,list);
	flen = jpf->frame_len;
	//_os_printf("%s:%d\tflen:%d\n",__FUNCTION__,__LINE__,flen);

	return flen;
}



void *get_jpeg_first_buf(void *d)
{
	struct list_head* get_f = (struct list_head*)d;
	mjpeg_node* mjn;
	mjn = list_entry(get_f->next,mjpeg_node,list);
	if(mjn)
	{
		return mjn->buf_addr;
	}
	return NULL;
} 
 
int del_jpeg_first_node(void *d)
{	
	jpeg_frame* get_f = (jpeg_frame*)d;
	put_node(&free_tab[get_f->jpg_num],(struct list_head*)get_f);
	
	return 0;	
}
 
void *get_jpeg_node_buf(void *d)
{
	struct list_head* get_f = (struct list_head*)d;
	mjpeg_node* mjn;
	mjn = list_entry(get_f,mjpeg_node,list);
	if(mjn)
	{
		return mjn->buf_addr;
	}
	return NULL;
}


//删除节点,是实际节点
void del_jpeg_node(void *d,uint32_t jpg_num)
{
	uint32_t flags;
	struct list_head* get_f = (struct list_head *)d;
	flags = disable_irq();
	list_add_tail(get_f,(struct list_head*)&free_tab[jpg_num]);
	enable_irq(flags);
}






#endif
