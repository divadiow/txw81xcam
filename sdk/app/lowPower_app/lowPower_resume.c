/*********************************************************************************************
 * 本文件仅仅用于低功耗休眠恢复的一些demo,不是完整的,更多的应用休眠和恢复要根据实际应用逻辑和硬件
 * 去实现,demo有部分说明
 ********************************************************************************************/

#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "osal/task.h"
#include "audio_app/audio_adc.h"
#include "audio_app/audio_dac.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "hal/jpeg.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "osal/msgqueue.h"
#include "osal/sleep.h"
#include "osal/semaphore.h"
#if LOWPOWER_DEMO == 1
#ifdef CONFIG_SLEEP
#include "lib/lmac/lmac_dsleep.h"
#include "lib/common/dsleepdata.h"
#endif
#include "lib/net/eloop/eloop.h"

#include "stream_frame.h"
#include "app_lcd/osd_encode_stream.h"

#ifdef CONFIG_SLEEP

//休眠定时器起来,如果需要添加io唤醒,需要自行修改
//默认用了保活方式,所以需要连接路由器成功后才会休眠
void sleep_test(uint32_t time)
{
    uint32 sleep_type = 1;
    struct system_sleep_param args;
	memset(&args,0,sizeof(args));
	args.sleep_ms = time;
    //args.wkup_io_sel[0] = PA_13;//LP Module PA_8
    args.wkup_io_edge = 0x00;//0: Rising; 1:Falling Edge
    args.wkup_io_en = 0x00;//IO0 En
    os_printf("%s:%d\tenter sleep\n",__FUNCTION__,__LINE__);
    system_sleep(sleep_type, &args);
}



static void enter_lowpower_timer(struct event *ei, void *d)
{
    uint32_t time = (uint32_t)d;
    os_printf("%s:%d\ttime:%d\n",__FUNCTION__,__LINE__,time);
    //进入休眠,休眠500ms起来
    sleep_test(time);
}



//恢复顺序根据硬件以及程序逻辑去适配(比如io复用之类都要考虑)
void lowPower_resume_task(void *d)
{
    //uint32_t flags;
    struct os_msgqueue *msgq = (struct os_msgqueue *)d;
    uint32_t res = os_msgq_get(msgq,-1);
    //收到信号量,恢复对应的硬件
    if(res)
    {

        //usb u盘的恢复
        #if USB_EN
        #ifdef RT_USBH
                hg_usbh_register(HG_USB_HOST_CONTROLLER_DEVID);
        #endif
        #endif

        //lcd相关恢复,由于恢复与儿童相机的原框架不匹配,这里只是对lcd简单恢复,没有任何应用恢复
        #if LVGL_STREAM_ENABLE == 1 && LCD_EN == 1
        uint16_t w,h;
        uint16_t screen_w,screen_h;
        uint8_t rotate,video_rotate;
        stream *s1 = get_stream_available(R_OSD_ENCODE);
        stream *s2 = get_stream_available(R_OSD_SHOW);
        extern void lcd_driver_reinit();
        extern void lcd_hardware_init(uint16_t *w,uint16_t *h,uint8_t *rotate,uint16_t *screen_w,uint16_t *screen_h,uint8_t *video_rotate);
        lcd_hardware_init(&w,&h,&rotate,&screen_w,&screen_h,&video_rotate);
        lcd_driver_reinit();
        stream_self_cmd_func(s1, OSD_HARDWARE_STREAM_RESET, 0);
        close_stream(s1);
        close_stream(s2);
        lvgl_hdl_resume();
        #endif
        

        //这里是dvp恢复
        #if JPG_EN
            jpg_cfg(HG_JPG0_DEVID,VPP_DATA0);
        #endif
        #if DVP_EN
			bool csi_cfg();
			bool csi_open();
            csi_cfg();
            csi_open();
        #endif

        //恢复sd
		#if SDH_EN && FS_EN
			extern bool fatfs_register();
			fatfs_register();
		#endif
    }


    //释放空间
    os_msgq_del(msgq);
    os_free(msgq);
}

static uint8_t suspend_flag = 0;
static struct os_msgqueue *msgq = NULL;
//开始进入低功耗模式,首先将自己应用层的资源释放,创建一个高优先级的线程,主要用于低功耗唤醒后,可以第一时间快速恢复外部硬件
void lowPower_start()
{
    if(suspend_flag)
    {
       return; 
    }
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);

    //lcd的休眠,如果硬件有特殊,需要额外处理
    #if LVGL_STREAM_ENABLE == 1 && LCD_EN == 1
    lvgl_hdl_suspend();
    wait_lcd_exit();
    #endif


    //u盘恢复
    #if USB_EN
    #ifdef RT_USBH
        hg_usbh_unregister(HG_USB_HOST_CONTROLLER_DEVID);
    #endif
    #endif



    //卸载文件系统
	#if SDH_EN && FS_EN
    fatfs_unregister();
	#endif


    //开始主动关闭硬件以及硬件模块,外围硬件需要自己去关闭
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    #if DVP_EN
        struct vpp_device *vpp = (struct vpp_device *)dev_get(HG_VPP_DEVID);
        struct dvp_device *dvp= (struct dvp_device *)dev_get(HG_DVP_DEVID);
        dvp_close(dvp);
        vpp_close(vpp);
    #endif
    os_printf("%s:%d\n",__FUNCTION__,__LINE__);
    #if JPG_EN
        jpg_clear_init_flag(0);
    #endif
    msgq = (struct os_msgqueue *)os_malloc(sizeof(struct os_msgqueue));
    memset(msgq,0,sizeof(struct os_msgqueue));
    os_msgq_init(msgq,1);
    //创建线程任务
    suspend_flag = 1;
    os_task_create("resume", lowPower_resume_task, (void*)msgq, OS_TASK_PRIORITY_ABOVE_NORMAL, 0, NULL, 1024);
    //os_msgq_put(msgq,msgq,0);
}

void lowPower_task_wakeup()
{
    suspend_flag = 0;
    os_msgq_put(msgq,msgq,0);
}



//应用层的休眠唤醒回调函数,所有操作在上面几个函数实现,不同应用实现不一样,所以上面的函数根据自己应用作改动
int32 app_sleepcb(uint16_t type, struct sys_sleepcb_param *args, void *priv)
{
	uint8_t action = args->action;
	uint8_t step = args->step;
    uint8_t reason = args->resume.wkreason;
    switch(action)
    {
        case SYS_SLEEPCB_ACTION_SUSPEND:
            if(step == SYS_SLEEPCB_APP)
            {
				lowPower_start();
            }
            break;
        case SYS_SLEEPCB_ACTION_RESUME:
            if(step == SYS_SLEEPCB_APP)
            {
                //注意这里需要判断一下唤醒的原因,由于这里是demo,所有的原因都会唤醒
                //if(reason == DSLEEP_WK_REASON_WK_DATA)
                {
                    lowPower_task_wakeup();
                }
                
            }
            break;
    }
    return RET_OK;
}

//这个返回1代表收到需要的数据,会唤醒,原因是DSLEEP_WK_REASON_WK_DATA
//返回0就不是匹配数据,会继续休眠

//这里demo只是检测到tcp就唤醒,如果自己的应用,还需要判断数据是否匹配等
//还有就是tcp即使不是数据包,可能也要唤醒,需要维持tcp的连接
int32 user_check_data(void *usr_wkdet_priv, uint8 *data, uint32 len)
{
    //tcp的长度会超过10
    if(len >= 10)
    {
        //判断一下是否为tcp,简单去处理了,tcp就回应一个心跳包,实际应用可能还要判断一下是否为自己需要的ip
        //应用层就是唤醒后就去写卡  dvp  录像之类
        if(data[9] == 0x06)
        {
            return 1;
        }
    }
    return 0;
}

//应用层低功耗初始化
void app_lowpower_init()
{
    sys_register_sleepcb(app_sleepcb,NULL);
    dsleep_set_usr_wkdet_cb(NULL, user_check_data);

    //定时休眠
    eloop_add_timer(5000,EVENT_F_ENABLED,enter_lowpower_timer,(void*)5000);
}




static int32 app_sleepcb_test(uint16_t type, struct sys_sleepcb_param *args, void *priv)
{
	uint8_t action = args->action;
	uint8_t step = args->step;
    uint8_t reason = args->resume.wkreason;
    switch(action)
    {
        //低功耗休眠的时候会调用的接口,这里处理完毕后,尽量保证自己的线程不要再运行或者不要运行一些外设网络的东西
        case SYS_SLEEPCB_ACTION_SUSPEND:
            if(step == SYS_SLEEPCB_APP)
            {
                os_printf("##########################enter %s:%d\n",__FUNCTION__,__LINE__);
            }
            break;

        //低功耗恢复,这里恢复可以是别原因,APP只会调用一次才对
        case SYS_SLEEPCB_ACTION_RESUME:
            if(step == SYS_SLEEPCB_APP)
            {
                //注意这里需要判断一下唤醒的原因,由于这里是demo,所有的原因都会唤醒
                //if(reason == DSLEEP_WK_REASON_WK_DATA)
                {
                    os_printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$exit %s:%d\n",__FUNCTION__,__LINE__);
                }
                
            }
            break;
    }
    return RET_OK;
}



/********************************************************************
 * type:
 typedef enum {
    SYSTEM_SLEEP_TYPE_SRAM_WIFI      = 1,
    SYSTEM_SLEEP_TYPE_WIFI_DSLEEP    = 2,
    SYSTEM_SLEEP_TYPE_SRAM_ONLY      = 3,
    SYSTEM_SLEEP_TYPE_LIGHT_SLEEP    = 4,
    SYSTEM_SLEEP_TYPE_RTCC           = 5,
    SYSTEM_SLEEP_TYPE_MAX            = 6,
} SYSTEM_SLEEP_TYPE;

time:代表休眠多久时间后唤醒
 * ******************************************************************/
 //测试的时候可以将type设置为3(SYSTEM_SLEEP_TYPE_SRAM_ONLY),不需要连接wifi
void enter_sleep_test(uint32_t type,uint32_t time)
{
    uint32 sleep_type = type;
    struct system_sleep_param args;
	memset(&args,0,sizeof(args));
	args.sleep_ms = time;
    //args.wkup_io_sel[0] = PA_13;//LP Module PA_8
    args.wkup_io_edge = 0x00;//0: Rising; 1:Falling Edge
    args.wkup_io_en = 0x00;//IO0 En
    os_printf("%s:%d\tenter sleep\n",__FUNCTION__,__LINE__);
    system_sleep(sleep_type, &args);
}


//内部测试低功耗使用
void lowpoer_sleep_test_init()
{
    sys_register_sleepcb(app_sleepcb_test,NULL);
}
#endif
#endif