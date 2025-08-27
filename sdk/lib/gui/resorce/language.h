#ifndef _LANGUAGE_H_
#define _LANGUAGE_H_

#include <stdio.h>
#include "typesdef.h"

enum
{
    SOUND_STR,
    ISO_STR,
    RECORD_STR,
    TAKEPHOTO_STR,
    SETTING_STR,
    FORMAT_STR,
    WIFI_STR,
    HZ_STR,
    CYCLE_STR,
    USB_STR,
    BATTERY_STR,
    LANGUAGE_STR,
    EXIT_STR,
    LANGUAGE_EN_STR,
    LANGUAGE_CN_STR,
    YES_STR,
    NO_STR, 
    OPEN_STR,
    CLOSE_STR,
    NEXT_STR,
    CONTINUOUS_STR,
    SPIN_STR,
    PHOTO_STR,
    PLAYBACK_PHOTO_STR,
    PLAYBACK_REC_STR,
    PLAY_STR,
    NEXT_REC_STR,
    GAME_STR,
    LARGER_STR,
    RAHMEN_STR,
    USBDEV_STR,
    UDISK_STR,
    UVC_STR,
    VFX_STR,
    PRT_STR,
    LANGUAGE_STR_MAX,
};


enum
{
    English,
    Chinese,
    Language_MAX,
};


//extern uint8_t *language_switch[Language_MAX];
extern const uint8_t language_switch[Language_MAX][LANGUAGE_STR_MAX+1][128];
extern uint8_t language_cur;

#endif