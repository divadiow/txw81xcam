#include "lwip\sockets.h"
#include "lwip\netif.h"
#include "lwip\dns.h"
#include "lwip\api.h"
#include "lwip\tcp.h"

#include "rtsp_common.h"
#include "osal/string.h"
#include "stream_define.h"
#include "video_app/video_app.h"
#include "stream_frame.h"
#include "log.h"
extern stream *new_video_app_stream(const char *name);;
extern void spook_send_thread_stream(struct rtsp_priv *r);
extern uint32_t creat_stream(struct rtsp_priv *r,const char *video_name,const char *audio_name);
extern uint32_t destory_stream(struct rtsp_priv *r);



void rtsp_live_thread(void *d)
{
	struct rtsp_priv *r = (struct rtsp_priv*)d;
	os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	spook_send_thread_stream(r);
}


//创建实时预览的的线程
void live_rtsp_creat(struct rtsp_source *source,void *priv)
{
	uint32_t res;
	//start_jpeg();


	source->priv = (struct rtsp_priv*)os_zalloc(sizeof(struct rtsp_priv));
	struct rtsp_priv *r = (struct rtsp_priv*)source->priv;
	if(source->priv)
	{	
		r->live_node = &source->live_node;
		//如果没有打开,用默认方式,如果打开了屏,请在屏端开启数据发送到R_RTP_JPEG
		#if !(LCD_EN == 1 && LVGL_STREAM_ENABLE == 1)
			#if DVP_EN
			//为了兼容旧版本
			r->priv = (void*)new_video_app_stream(S_JPEG);
			if(r->priv)
			{
				streamSrc_bind_streamDest((stream*)r->priv,R_RTP_JPEG);
				enable_stream((stream*)r->priv,1);
			}
			#endif
		#endif

		#if AUDIO_EN == 1
		res = creat_stream(r,R_RTP_JPEG,R_RTP_AUDIO);
		#else
		res = creat_stream(r,R_RTP_JPEG,NULL);
		#endif
		if(!res)
		{
			OS_TASK_INIT("live_rtsp", &source->handle, rtsp_live_thread, r, OS_TASK_PRIORITY_NORMAL, 1024);
		}
	}
	else
	{
		os_printf("%s not enough space\n",__FUNCTION__);
	}
	

	return;
}


//创建实时预览的的线程
void live_rtsp_destory(struct rtsp_source *source)
{
	//stop_jpeg();
	void *tmp = source->handle.hdl;
	struct rtsp_priv *r = source->priv;
	if(r)
	{
		if(r->priv)
		{
			os_printf("r->priv:%X\n",r->priv);
			close_stream((stream*)r->priv);
		}
		destory_stream(r);
		os_free(source->priv);
		source->priv = NULL;
	}
	if(tmp)
	{
		os_task_del(&source->handle);
	}
	
}
static int live_get_sdp( struct session *s, char *dest, int *len,
				char *path )
{
	struct rtsp_session *ls = (struct rtsp_session *)s->private;
	int i = 0;
	int t = 0;
	char *addr = "IP4 0.0.0.0";
	os_printf("%s:%d\tpath:%s\n",__FUNCTION__,__LINE__,path);

	if( s->ep[0] && s->ep[0]->trans_type == RTP_TRANS_UDP )
		addr = s->ep[0]->trans.udp.sdp_addr;

	i = snprintf( dest, *len,"v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=Test\r\na=type:broadcast\r\nt=0 0\r\nc=IN %s\r\n", addr );

		os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	os_printf("%s source->track[t]:%X\trtp:%X\n",ls->source->track[t],ls->source->track[t].rtp);
	for( t = 0; t < MAX_TRACKS && ls->source->track[t].rtp; ++t )
	{
		int port;

		if( s->ep[t] && s->ep[t]->trans_type == RTP_TRANS_UDP )
			port = s->ep[t]->trans.udp.sdp_port;
		else
			port = 0;


		if(ls->source->track[t].rtp->type == 0)
		{
			i += ls->source->track[t].rtp->get_sdp( dest + i, *len - i,96 + t, port,ls->source->track[t].rtp->private );
		}
		else
		{
			i += ls->source->track[t].rtp->get_sdp( dest + i, *len - i,96 + t, port,NULL);
		}
		
		if( port == 0 ) // XXX What's a better way to do this?
			i += sprintf( dest + i, "a=control:track%d\r\n", t );
	}
	os_printf("%s:%d\n",__FUNCTION__,__LINE__);
	*len = i;
	return t;
}

static struct session *live_rtsp_open( char *path, void *d )
{
	//默认的,各自模式可以各自去修改
	struct session *sess = rtsp_open(path,d);
	if(sess)
	{
		sess->get_sdp = live_get_sdp;
	}
	return sess;
}

static int live_set_path( const char *path, void *d )
{
  	spook_log (SL_DEBUG, "live set_path %s", path);

	/*if( num_tokens == 2 )*/
	{
		new_rtsp_location( path, NULL, NULL, NULL,
				live_rtsp_open, d );
		return 0;
	}
	spook_log( SL_ERR, "rtsp-handler: syntax: Path <path> [<realm> <username> <password>]" );
	return -1;
}

void live_rtsp_init(const rtp_name *rtsp)
{
	struct rtsp_source *source;
	source = rtsp_start_block();
	live_set_path( rtsp->path, source );
	set_video_track( (char*)rtsp->jpg_name, source );
	#if AUDIO_EN == 1
	set_audio_track( (char*)rtsp->audio_name, source );
	#endif
	register_live_fn(source,live_rtsp_creat,live_rtsp_destory);
	rtsp_end_block(source);
	return ;
}