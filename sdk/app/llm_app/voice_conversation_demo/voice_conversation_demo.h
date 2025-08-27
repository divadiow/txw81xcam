#ifndef __VOICE_CONVERSATION_DEMO_H
#define __VOICE_CONVERSATION_DEMO_H

#include "llm/llm_api.h"
#include "llm/DouBao/DouBao.h"

#include "syscfg.h"
#include "keyWork.h"
#include "keyScan.h"
#include "mp3_decode.h"
#include "audio_dac.h"
#include "stream_define.h"
#include "stream_frame.h"

#define STT_RECV_SIZE   1024
#define CHAT_RECV_SIZE  1024

struct llm_demo_manage {
    uint8   key_transfer;       //按键
    uint8   auadc_model_start;  //麦克风采集
    void    *llm;
    void    *chat_session;
    void    *stt_session;
    void    *tts_session;
};

extern struct llm_demo_manage llm_mgr;

#endif
