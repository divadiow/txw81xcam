#include "voice_conversation_demo.h"

#ifdef LLM_DEMO

/* Please add your server information */
#define Chat_URL        "https://xxx"
#define ARK_API_KEY     "xxx-xxx-xxx"
#define ENDPOINT_ID     "xxx"

#define STT_URL         "wss://xxx"
#define STT_APPID       "xxx"
#define STT_TOKEN       "xxx"
#define STT_CLUSTER     "xxx"

#define TTS_URL         "wss://xxx"
#define TTS_APPID       "xxx"
#define TTS_TOKEN       "xxx"
#define TTS_CLUSTER     "xxx"

struct llm_demo_manage llm_mgr = {
    .key_transfer       = 0,
    .auadc_model_start  = 0,
};

static int32 opcode_func(stream *s,void *priv,int opcode)
{
	int32 res = 0;
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
			break;
	}
	return res;
}

uint32_t llm_intercom_push_key(struct key_callback_list_s *callback_list,uint32_t keyvalue,uint32_t extern_value)
{
	if( (keyvalue>>8) != AD_PRESS)
		return 0;
	uint32 key_val = (keyvalue & 0xff);
	if((key_val == KEY_EVENT_LDOWN) || (key_val == KEY_EVENT_REPEAT)) {
        if (llm_mgr.key_transfer == 0) {
            llm_mgr.key_transfer = 1;
            //清除上次的资源
            llm_chat_stop(llm_mgr.chat_session);
            llm_stt_stop(llm_mgr.stt_session);
            llm_tts_stop(llm_mgr.tts_session);

            if (get_audio_dac_set_filter_type() != SOUND_NONE)
			    audio_dac_set_filter_type(SOUND_NONE);
		    //触发采集
            llm_mgr.auadc_model_start = 1;
        }
	}
	else if((key_val == KEY_EVENT_LUP)) {
        llm_mgr.auadc_model_start = 0;
        llm_mgr.key_transfer = 0;
    	if(get_audio_dac_set_filter_type() != SOUND_FILE)
		    audio_dac_set_filter_type(SOUND_FILE);
	}
	return 0;
}

int32 llm_mp3_read_func(void *ptr, uint32_t size)
{
	int32 read_len = 0;
	while (read_len == 0) {
		read_len = llm_tts_recv(llm_mgr.tts_session, ptr, size);
		os_sleep_ms(10);
	}
	return read_len;
}

sysevt_hdl_res sysevt_llm_event(uint32 event_id, uint32 data, uint32 priv)
{
    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_LLM, SYSEVT_LLM_CHAT_FAIL):
            os_printf("Text conversation failed!!\r\n");
            break;
        case SYS_EVENT(SYS_EVENT_LLM, SYSEVT_LLM_STT_FAIL):
            os_printf("STT failed!!\r\n");
            break;
        case SYS_EVENT(SYS_EVENT_LLM, SYSEVT_LLM_TTS_FAIL):
            os_printf("TTS failed!!\r\n");
            break;
        default:
            os_printf("no this event(%x)...\r\n");
    }
    return SYSEVT_CONTINUE;
}

void llm_main_demo(void)
{
    int32 tx_result     = 0;
    int32 stt_rx_len    = 0;
    int32 chat_rx_len   = 0;
	stream* s = NULL;
	struct data_structure *data_s = NULL;
	char *copy_buf = NULL;
	uint8 *data = NULL;
    uint32 data_len = 0;
	uint32 copy_offset = 0;
	uint8 last_sta = 0;
	
	os_printf("Waiting for network connection...");
    while (!sys_status.wifi_connected || !sys_status.dhcpc_done) {
        _os_printf(".");
        os_sleep(1);
    }
    os_printf("Network connected!\r\n");
	
	//microphone
	s = open_stream_available(R_SPEECH_RECOGNITION,0,8,opcode_func,NULL);
    if(!s)
    {
        os_printf("open speech_recognition stream err!\r\n");
        return;
    }

    //user rx buff
    char *stt_recv = os_malloc(STT_RECV_SIZE);
    char *chat_recv = os_malloc(CHAT_RECV_SIZE);
    if (!stt_recv || !chat_recv) {
        os_printf("user rx buff malloc fail!\r\n");
        goto cleanup;
    }

    //llm init
    llm_mgr.llm = llm_global_init();
    if (llm_mgr.llm == NULL) {
        os_printf("llm_global_init fail!\r\n");
        goto cleanup;
    }
    llm_mgr.chat_session  = llm_chat_init(llm_mgr.llm, "doubao_chat");
    llm_mgr.stt_session   = llm_stt_init(llm_mgr.llm, "doubao_stt");
    llm_mgr.tts_session   = llm_tts_init(llm_mgr.llm, "doubao_tts");
    if (!llm_mgr.chat_session || !llm_mgr.stt_session || !llm_mgr.tts_session) {
        os_printf("session init fail!\r\n");
        goto cleanup;
    }

    Doubao_CHAT_Cfg chat_cfg = {
        .url = Chat_URL,
        .ark_api_key = ARK_API_KEY,
        .endpoint_id = ENDPOINT_ID,
        .max_tokens = 4096,
    };
    Doubao_STT_Cfg stt_cfg = {
        .url = STT_URL,
        .app_appid = STT_APPID,
        .app_token = STT_TOKEN,
        .app_cluster = STT_CLUSTER,
        .user_uid = "41494922",
        .audio_format = "raw",
        .audio_codec = "raw",
        .audio_rate = 8000,
        .audio_bits = 16,
    };
    Doubao_TTS_Cfg tts_cfg = {
        .url = TTS_URL,
        .app_appid = TTS_APPID,
        .app_token = TTS_TOKEN,
        .app_cluster = TTS_CLUSTER,
        .user_uid = "41494922",
        .audio_voice_type = "BV700_streaming",
        .audio_rate = 8000,
        .audio_encoding = "mp3",
        .audio_compression_rate = 1,
        .audio_speed_ratio = 1.0,
        .audio_volume_ratio = 2.0,
        .audio_pitch_ratio = 1.0,
        .audio_emotion = "comfort",
        .audio_language = "cn",
    };

    llm_chat_config(llm_mgr.chat_session, (void *)&chat_cfg, sizeof(chat_cfg));
    llm_stt_config(llm_mgr.stt_session, (void *)&stt_cfg, sizeof(stt_cfg));
    llm_tts_config(llm_mgr.tts_session, (void *)&tts_cfg, sizeof(tts_cfg));
    
    //Player init
    mp3_decode_init(NULL, llm_mp3_read_func);
    //Key init
    add_keycallback(llm_intercom_push_key, NULL);

    //Monitor LLM events
    sys_event_take(SYS_EVENT(SYS_EVENT_LLM, 0), sysevt_llm_event, 0);

    while (1) {
		data_s = recv_real_data(s);
        if (data_s) {
			data = get_stream_real_data(data_s);
            data_len = get_stream_real_data_len(data_s);
            if (llm_mgr.auadc_model_start) {
				if (!copy_buf) {
					copy_buf = os_malloc_psram(data_len*8*2*2);
				}
				os_memcpy(copy_buf+copy_offset, data, data_len);
				copy_offset += data_len;
				if (copy_offset >= data_len*8*2*2) {
					llm_stt_send(llm_mgr.stt_session, copy_buf, copy_offset, 0);
                    os_free_psram(copy_buf);
					copy_buf = NULL;
					copy_offset = 0;
				}
            } else if ((last_sta) && (!llm_mgr.auadc_model_start)) {
				if (!copy_buf) {
					copy_buf = os_malloc_psram(data_len*8*2*2);
				}
				os_memcpy(copy_buf+copy_offset, data, data_len);
				copy_offset += data_len;
                llm_stt_send(llm_mgr.stt_session, copy_buf, copy_offset, 1);
                os_free_psram(copy_buf);
				copy_buf = NULL;
				copy_offset = 0;
            }
		    last_sta = llm_mgr.auadc_model_start;  
            free_data(data_s);
            data_s = NULL;
		}
		
        stt_rx_len = llm_stt_recv(llm_mgr.stt_session, stt_recv, STT_RECV_SIZE);
        if (stt_rx_len > 0) {
            do {
                tx_result = llm_chat_send(llm_mgr.chat_session, "You are a helpful assistant.", stt_recv, stt_rx_len);
                os_sleep_ms(1);
            } while (tx_result == RET_ERR);
        }
        chat_rx_len = llm_chat_recv(llm_mgr.chat_session, chat_recv, CHAT_RECV_SIZE);
        if (chat_rx_len > 0) {
            do {
                tx_result = llm_tts_send(llm_mgr.tts_session, chat_recv, chat_rx_len);
                os_sleep_ms(1);
            } while (tx_result == RET_ERR);
        }
        os_sleep_ms(1);
    }

cleanup:
    if (stt_recv) os_free(stt_recv);
    if (chat_recv) os_free(chat_recv);
    llm_global_deinit(llm_mgr.llm);
    llm_chat_deinit(llm_mgr.chat_session);
    llm_stt_deinit(llm_mgr.stt_session);
    llm_tts_deinit(llm_mgr.tts_session);
}

struct os_task llm_task_demo;
void llm_demo(void)
{
    OS_TASK_INIT("LLM_DEMO", &llm_task_demo, llm_main_demo, NULL, OS_TASK_PRIORITY_NORMAL, 1024);
}

#endif
