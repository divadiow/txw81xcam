#ifndef _LLM_API_H_
#define _LLM_API_H_

#include "basic_include.h"

#ifdef __cplusplus
extern "C" {
#endif

void *llm_global_init(void);
int32 llm_global_deinit(void *llm);

//CHAT
void *llm_chat_init(void *llm, char *llm_name);
int32 llm_chat_deinit(void *session);
int32 llm_chat_send(void *session, char *role, char *question, uint32 question_len);
int32 llm_chat_recv(void *session, char *buff, uint32 buff_size);
int32 llm_chat_config(void *session, void *chat_cfg, uint32 cfg_size);
int32 llm_chat_stop(void *session);

//STT
void *llm_stt_init(void *llm, char *llm_name);
int32 llm_stt_deinit(void *session);
int32 llm_stt_send(void *session, char *audio_buff, uint16 audio_len, uint8 is_end);
int32 llm_stt_recv(void *session, char *buff, uint32 buff_size);
int32 llm_stt_config(void *session, void *stt_cfg, uint32 cfg_size);
int32 llm_stt_stop(void *session);

//TTS
void *llm_tts_init(void *llm, char *llm_name);
int32 llm_tts_deinit(void *session);
int32 llm_tts_send(void *session, char *text, uint32 text_len);
int32 llm_tts_recv(void *session, void *ptr, uint32 size);
int32 llm_tts_config(void *session, void *tts_cfg, uint8 cfg_size);
int32 llm_tts_stop(void *session);

#ifdef __cplusplus
}
#endif

#endif

