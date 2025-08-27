#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "stream_frame.h"
#include "utlist.h"
#include "osal/work.h"
#include "lib/lcd/lcd.h"
#include "dev/scale/hgscale.h"
#include "dev/jpg/hgjpg.h"
#include "app_lcd.h"
#include "decode/recv_jpg_yuv_demo.h"
#include "yuv_stream.h"
#include "custom_mem/custom_mem.h"

//创建一个线程,专门是处理lcd显示
struct osd_show_s
{
	struct lcdc_device *lcd_dev;
	stream *s;
	struct data_structure *last_show_data_s;
};



//如果说workqueue确定没有一些耗时的任务,这个也是可以放到workqueue去处理,节省线程的内存
void lcd_thread(void *d)
{
	struct app_lcd_s *lcd_s = (struct app_lcd_s *)d;
    struct lcdc_device *lcd_dev = lcd_s->lcd_dev;
    stream *p1_s = lcd_s->video_p1_s;
    stream *p0_s = lcd_s->video_p0_s;
    stream *s = lcd_s->osd_show_s;
	struct osd_show_s *osd_show = (struct osd_show_s *)s->priv;
	struct recv_jpg_yuv_s *yuv_p1 = (struct recv_jpg_yuv_s *)p1_s->priv;

    struct yuv_stream_s *yuv_p0 = (struct yuv_stream_s *)p0_s->priv;

    struct data_structure *osd_data_s;
    struct data_structure *video_p1_data_s;
    struct data_structure *video_p0_data_s;
    
    uint32_t p0_w=0,p0_h=0,p1_w=0,p1_h=0;
    struct yuv_arg_s *yuv_msg;
    uint32_t y_off = 0,uv_off = 0;


    //是否要启动显示,只要有osd或者video显示,都要启动硬件模块
    //如果是mcu屏,内部可以增加其他逻辑,判断显示是否有变化决定是否刷屏
    //如果是rgb屏,无论内容是否发生变化,都要刷新,否则会有问题
    uint8_t flag = 0;
    uint8_t p0_p1_enable = 0;
    while(!lcd_s->thread_exit)
    {
        //ready
        p0_p1_enable = 0;
        if(lcd_s->hardware_ready)
        {
            //显示osd
            {
                osd_data_s = recv_real_data(s);
                if(osd_data_s)
                {
                    //os_printf("%s:%d\tosd_data_s:%X\n",__FUNCTION__,__LINE__,osd_data_s);
                    if(osd_show->last_show_data_s)
                    {
                        free_data(osd_show->last_show_data_s);
                        osd_show->last_show_data_s = NULL;
                    }           
                    osd_show->last_show_data_s = osd_data_s;
                }
                else
                {
                    osd_data_s = osd_show->last_show_data_s;
                }
                //os_printf("data_s len:%d\t%X\n",get_stream_real_data_len(osd_data_s),osd_data_s);
                if(osd_data_s)
                {
                    lcdc_set_osd_dma_addr(lcd_dev,(uint32)get_stream_real_data(osd_data_s));
                    lcdc_set_osd_dma_len(lcd_dev,get_stream_real_data_len(osd_data_s));
                    lcdc_set_osd_en(lcd_dev,1);
                    //os_printf("encode osd len:%d\n",get_stream_real_data_len(osd_data_s));
                    flag++;
                }
                else
                {
                    lcdc_set_osd_en(lcd_dev,0);
                }
            }




            //显示video p1
            //这里是video_p1的显示
            {
                lcd_thread_get_video_p1_again:
                video_p1_data_s = recv_real_data(p1_s);
                
                //有新的yuv数据要显示
                if(video_p1_data_s)
                {
                    //os_printf("%s:%d\tvideo_p1_data_s:%X\n",__FUNCTION__,__LINE__,video_p1_data_s);
                    //移除原来的数据
                    if(yuv_p1->last_data_s)
                    {
                        free_data(yuv_p1->last_data_s);
                    }
                    yuv_p1->last_data_s = video_p1_data_s;
                }
                else if(yuv_p1->last_data_s)
                {
                    video_p1_data_s = yuv_p1->last_data_s;
                }

                //判断一下显示时间,如果过时的,就不显示了
                if(video_p1_data_s)
                {
                    uint32_t show_time =stream_self_cmd_func(p1_s,GET_LVGL_YUV_UPDATE_TIME,(uint32_t)0);
                    if(show_time > get_stream_data_timestamp(video_p1_data_s))
                    {
                        free_data(yuv_p1->last_data_s);
                        yuv_p1->last_data_s = NULL;
                        goto lcd_thread_get_video_p1_again;
                    }
                    
                }


                if(video_p1_data_s)
                {
                    yuv_msg = (struct yuv_arg_s *)stream_self_cmd_func(video_p1_data_s->s,YUV_ARG,(uint32_t)video_p1_data_s);

                    //错误的图片,可能不需要显示或者发送错误
                    if(yuv_msg)
                    {

                        //需要删除
                        if(yuv_msg->del && *yuv_msg->del)
                        {
                            free_data(video_p1_data_s);
                            yuv_p1->last_data_s = NULL;
                            os_printf("%s:%d p1 del:%X\n",__FUNCTION__,__LINE__,video_p1_data_s);
                            goto lcd_thread_get_video_p1_again;
                        }

                        p1_w = yuv_msg->out_w;
                        p1_h = yuv_msg->out_h;
                        uint8_t *yuv_buf = get_stream_real_data(video_p1_data_s);
                        if(lcd_s->rotate == LCD_ROTATE_180)
                        {
                            y_off = p1_w*(p1_h-1);
                            uv_off= ((p1_w/2+3)/4)*4 * (p1_h/2-1);
                        }
                        lcdc_set_p1_rotate_y_src_addr(lcd_dev,(uint32)yuv_buf+y_off/**/);
                        lcdc_set_p1_rotate_u_src_addr(lcd_dev,(uint32)yuv_buf+yuv_msg->y_size+uv_off/**/);
                        lcdc_set_p1_rotate_v_src_addr(lcd_dev,(uint32)yuv_buf+yuv_msg->y_size+yuv_msg->y_size/4+uv_off/**/);
                        p0_p1_enable |= BIT(1);
                    }
                    else
                    {
                        os_printf("%s:%d\tlvgl video yuv err\n",__FUNCTION__,__LINE__);
                        os_printf("%s:%d\tvideo_p1_data_s:%X\tname:%s\n",__FUNCTION__,__LINE__,video_p1_data_s,video_p1_data_s->s->name);
                        free_data(yuv_p1->last_data_s);
                        yuv_p1->last_data_s = NULL;
                    }

                }
            }


            //强行显示一下p0的数据
            {
lcd_thread_get_video_p0_again:
                video_p0_data_s = recv_real_data(p0_s);                
                //有新的yuv数据要显示
                if(video_p0_data_s)
                {
                    //os_printf("%s:%d\tvideo_p1_data_s:%X\n",__FUNCTION__,__LINE__,video_p1_data_s);
                    //移除原来的数据
                    if(yuv_p0->last_data_s)
                    {
                        free_data(yuv_p0->last_data_s);
                    }
                    yuv_p0->last_data_s = video_p0_data_s;
                }
                else if(yuv_p0->last_data_s)
                {
                    video_p0_data_s = yuv_p0->last_data_s;
                }


                //判断一下显示时间,如果过时的,就不显示了
                if(video_p0_data_s)
                {
                    uint32_t show_time =stream_self_cmd_func(p0_s,GET_LVGL_YUV_UPDATE_TIME,(uint32_t)0);
                    if(show_time > get_stream_data_timestamp(video_p0_data_s))
                    {
                        free_data(video_p0_data_s);
                        yuv_p0->last_data_s = NULL;
                        goto lcd_thread_get_video_p0_again;
                    }
                }

                if(video_p0_data_s)
                {
                    yuv_msg = (struct yuv_arg_s *)stream_self_cmd_func(video_p0_data_s->s,YUV_ARG,(uint32_t)video_p0_data_s);
                    if(yuv_msg)
                    {
                        //需要删除
                        if(yuv_msg->del && *yuv_msg->del)
                        {
                            free_data(video_p0_data_s);
                            yuv_p0->last_data_s = NULL;
                            os_printf("%s:%d p0 del:%X\n",__FUNCTION__,__LINE__,video_p0_data_s);
                            goto lcd_thread_get_video_p0_again;
                        }
                        uint8_t *p_buf = (uint8_t*)get_stream_real_data(video_p0_data_s);
                        p0_w = yuv_msg->out_w;
                        p0_h = yuv_msg->out_h;
                        if(lcd_s->rotate == LCD_ROTATE_180)
                        {
                            y_off = p0_w*(p0_h-1);
                            uv_off= ((p0_w/2+3)/4)*4 * (p0_h/2-1);
                        }

                        lcdc_set_p0_rotate_y_src_addr(lcd_dev,(uint32)p_buf+y_off);
                        lcdc_set_p0_rotate_u_src_addr(lcd_dev,(uint32)p_buf+yuv_msg->y_size+uv_off);
                        lcdc_set_p0_rotate_v_src_addr(lcd_dev,(uint32)p_buf+yuv_msg->y_size+yuv_msg->y_size/4+uv_off);

                        p0_p1_enable |= BIT(0);
                    }

                }

            }

            //打开video
            if(p0_p1_enable)
            {
                //检查一下line_buf是否已经申请了,申请空间就是w*line_num的空间(w应该是和屏旋转有关)
                if(!lcd_s->line_buf)
                {
                    uint16_t rotate_w;
                    rotate_w = lcd_s->screen_w;
                    lcd_s->line_buf = (void*)custom_malloc(lcd_s->line_buf_num*rotate_w*2);
                    //这里理论要申请到空间,如果申请失败,重新申请一下(否则要就额外处理资源数据)
                    while(!lcd_s->line_buf)
                    {
                        os_sleep_ms(1);
                        lcd_s->line_buf = (void*)custom_malloc(lcd_s->line_buf_num*rotate_w*2);
                    }
                    lcdc_set_rotate_linebuf_y_addr(lcd_dev,(uint32)lcd_s->line_buf );
	                lcdc_set_rotate_linebuf_u_addr(lcd_dev,(uint32)lcd_s->line_buf +rotate_w*lcd_s->line_buf_num);
	                lcdc_set_rotate_linebuf_v_addr(lcd_dev,(uint32)lcd_s->line_buf +rotate_w*lcd_s->line_buf_num+(rotate_w/2)*lcd_s->line_buf_num);
                }
                lcdc_set_rotate_p0p1_size(lcd_dev,p0_w,p0_h,p1_w,p1_h);
                lcdc_set_rotate_mirror(lcd_dev,0,lcd_s->video_rotate);
                lcdc_set_p0p1_enable(lcd_dev,p0_p1_enable&BIT(0),p0_p1_enable&BIT(1));
                lcdc_set_video_en(lcd_dev,1);
                flag++;
            }
            else
            {
                //如果不需要显示video,则空间释放吧
                if(lcd_s->line_buf)
                {
                    custom_free(lcd_s->line_buf);
                    lcd_s->line_buf = NULL;

                }
                lcdc_set_video_en(lcd_dev,0);
            }
            
        }


        //lcd模块已经启动,hardware_ready清0
        if(flag)
        {
            //os_printf("%s:%d\n",__FUNCTION__,__LINE__);
            lcd_s->hardware_ready = 0; 

            lcdc_set_start_run(lcd_dev);
            flag = 0;
            
        }
        else
        {
            //不需要显示,则休眠1ms重新检查
            //这里暂时没有用信号量唤醒,主要考虑到osd和video都有可能需要显示,要考虑如何监听osd和video的数据才可以用信号量
            //这里用sleep,会占用一点点cpu
            os_sleep_ms(1);
        }
    }


    free_data(yuv_p0->last_data_s);
    yuv_p0->last_data_s = NULL;

    free_data(yuv_p1->last_data_s);
    yuv_p1->last_data_s = NULL;

    do
    {
        video_p0_data_s = recv_real_data(p0_s);  
        free_data(video_p0_data_s); 
        os_printf("video_p0_data_s:%X\n",video_p0_data_s);
        
    }while(video_p0_data_s);

    do
    {
        video_p1_data_s = recv_real_data(p1_s);  
        free_data(video_p1_data_s); 
        os_printf("video_p1_data_s:%X\n",video_p1_data_s);
        
    }while(video_p1_data_s);
    //相当于退出lcd的线程
    lcd_s->thread_exit = 0;
    lcd_s->thread_hdl = NULL;
    return;
}