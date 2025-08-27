#ifndef _DOUBAO_H_
#define _DOUBAO_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char    *url;
    char    *ark_api_key;
    char    *endpoint_id;
    uint16  max_tokens;
} Doubao_CHAT_Cfg;

typedef struct {
    char    *url;
    char    *app_appid;
    char    *app_token;
    char    *app_cluster;
    char    *user_uid;
    char    *audio_format;
    char    *audio_codec;
    uint16   audio_rate;
    uint16   audio_bits;
} Doubao_STT_Cfg;

typedef struct {
    char    *url;
    char    *app_appid;
    char    *app_token;
    char    *app_cluster;
    char    *user_uid;
    char    *audio_voice_type;
    uint16   audio_rate;
    uint16   audio_compression_rate;
    char    *audio_encoding;
    float   audio_speed_ratio;
    float   audio_volume_ratio;
    float   audio_pitch_ratio;
    char    *audio_emotion;
    char    *audio_language;
} Doubao_TTS_Cfg;

#ifdef __cplusplus
}
#endif

#endif

