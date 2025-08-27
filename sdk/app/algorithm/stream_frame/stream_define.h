#ifndef __STREAM_DEFINE_H
#define __STREAM_DEFINE_H

#ifndef BIT
#define BIT(a) (1UL << (a))
#endif


//yuv通用参数
struct yuv_arg_s
{
  uint32_t y_size;
  uint32_t y_off;
  uint32_t uv_off;
  uint32_t out_w;
  uint32_t out_h;
  //状态位,如果发现要求del,那么接收流就要尽快释放了,不要占用当前节点
  uint8_t *del;
};


//解码,yuv通用参数放前面,与yuv_arg_s保持一致
struct jpg_decode_arg_s
{
    struct yuv_arg_s yuv_arg;
    uint32_t rotate;
    uint32_t decode_w;  //解码图片的size
    uint32_t decode_h;
    uint32_t step_w;
    uint32_t step_h;
    uint32_t magic;

};


 
//用于定义各种data的类型枚举
enum {
  DATA_TYPE_ANY,
  DATA_TYPE_AUDIO_PDM,
  DATA_TYPE_AUDIO_I2S,
  DATA_TYPE_AUDIO_ADC,
  DATA_TYPE_AUDIO_DAC,
  DATA_TYPE_JPEG_VGA,
  DATA_TYPE_JPEG_720P,
  DATA_TYPE_YUV,
};



enum
{
  //这里是通用命令,定义一些通用的接口,代表所有都可以调用,获取或者设置都是一样的,需要明确各个参数
  JPG_DECODE_ARG = 0x01,      //opcode=0x01,arg=data_s,获取的是jpg解码相关参数,有解码后size,解码前size,以及step,参考jpg_decode_arg_s的结构
  YUV_ARG,                    //如果是yuv图片,可以通过这个获取相关yuv的参数(仅仅包含了yuv的必要参数),有y_size,uv_off,y_off,如果后续有需要可以扩展(比如旋转多少度)
  SET_LVGL_YUV_UPDATE_TIME,
  GET_LVGL_YUV_UPDATE_TIME,
  SET_LVGL_VIDEO_ARG,
  SCALE1_RESET_DPI,
  SET_MJPEG_RESOLUTION_PARM_CMD,
  RESET_MJPEG_DPI,
  RESET_MJPEG_FROM,
  PRC_MJPEG_KICK,
  PRC_REGISTER_ISR,
  STREAM_STOP_CMD,
  STREAM_ZBAR_FROM_SD,

  NEWAVI_PLAYER_LOCATE_FORWARD_INDEX,
  NEWAVI_PLAYER_LOCATE_REWIND_INDEX,
  NEWAVI_PLAYER_MAGIC,
  NEWAVI_PLAYER_STATUS,
  //这里是自定义的命令
  CUSTOM_STREAM_CMD = 0x100,
};
//定义各种流的名称,R开头代表接收的流,S开头是发送的流,SR代表既要接收又要发送的流
//名称不能相同

//没有专门发送和接收,只是流负责管理
#define MANAGE_STREAM_NAME "manage-stream"

//R
#define R_RECORD_AUDIO    "record-audio" //录像的音频
#define R_RECORD_JPEG    "record-jpeg" //录像的音频
#define R_RECORD_H264    "record-h264" //录像的音频
#define R_PHOTO_JPEG    "photo-jpeg" //录像的音频
#define R_ALK_JPEG   "alk-jpeg"
#define R_AUDIO_TEST   "audio-test"
#define R_SPEAKER     "speaker"
#define R_JPEG_TO_LCD "JPEG-LCD"
#define R_JPEG_DEMO "jpeg_demo"

#define R_USB_AUDIO_MIC "usb-audio-mic"


#define R_RTP_AUDIO       "rtp-audio"     //图传的音频
#define R_RTP_JPEG       "rtp-jpeg"     //图传的视频
#define R_LVGL_JPEG       "lvgl-jpeg"     //lvgl接收回放适配
#define R_LVGL_TAKEPHOTO  "lvgl_takephoto"



#define R_LOWPOERRESUME_EVENT   "LOWPOERRESUME_EVENT"
#define R_AT_SAVE_OSD   "AT_OSD"
#define R_AT_SAVE_AUDIO   "AT_AUDIO"
#define R_AT_SAVE_PHOTO   "AT_PHOTO"
#define R_AT_AVI_AUDIO    "AT_AVI_AUDIO" //录像的音频
#define R_AT_AVI_JPEG     "AT_AVI_JPEG" //录像的音频
#define R_SAVE_STREAM_DATA   "stream_data"
#define R_INTERMEDIATE_DATA   "R_Intermediate_data"
#define R_AVI_SAVE_VIDEO        "AVI_SAVE_VIDEO"
#define R_AVI_SAVE_AUDIO        "AVI_SAVE_AUDIO"


#define R_INTERCOM_AUDIO  "INTERCOM_SEND"
#define R_PSRAM_JPEG       "PSRM-jpeg"     //接收到psram的视频
#define R_USB_SPK         "usb_speaker"
#define R_OSD_ENCODE         "osd_encode"
#define R_OSD_SHOW          "osd_show"
#define R_VIDEO_P1            "VIDEO_P1"
#define R_VIDEO_P0            "VIDEO_P0"
#define R_JPG_TO_RGB            "JPG_TO_RGB"
#define R_YUV_TO_JPG            "YUV_TO_JPG"
#define R_ZBAR              "Zbar"

#define R_DEBUG_STREAM      "debug-stream"

#define R_SONIC_PROCESS     "r_sonic_process"

#define R_SPEECH_RECOGNITION "r_speech_recognition"

//S
#define S_PDM               "pdm"
#define S_ADC_AUDIO          "adc_audio"
#define S_MP3_AUDIO          "mp3_audio"
#define S_AMRNB_AUDIO       "amrnb_audio"
#define S_AMRWB_AUDIO       "amrwb_audio"
#define S_JPEG              "jpeg"
#define S_USB_JPEG          "usb-jpeg"
#define S_USB_JPEG_PSRAM    "usb-jpeg-psram"
#define S_WEBJPEG           "web-jpeg"
#define S_PLAYBACK          "playback-jpeg"
#define S_WEBAUDIO          "web-audio"
#define S_NET_JPEG          "net-jpeg"
#define S_USB_DUAL          "usb-dual"
#define S_INTERCOM_AUDIO    "INTERCOM_RECV"
#define S_INTERMEDIATE_DATA  "S_Intermediate_data"

#define S_USB_MIC           "usb_microphone"
#define S_LVGL_OSD           "lvgl_osd"
#define S_JPG_DECODE          "jpg_decode"
#define S_LVGL_JPG           "lvgl_jpg"
#define S_PREVIEW_SCALE3       "preview_scale3"
#define S_SCALE1_STREAM       "scale3_stream"
#define S_PREVIEW_ENCODE_QVGA  "preview_encode_qvga"
#define S_PREVIEW_ENCODE_QQVGA  "preview_encode_qqvga"
#define S_PREVIEW_ENCODE_VPP  "preview_encode_vpp"

#define S_SONIC_PROCESS     "s_sonic_process"
#define S_NEWPLAYER          "newplayer"
#define S_NEWAVI          "newavi"
#define S_ZBAR_FROM_SD          "zbar_read_sd"
#define S_LVGL_PHOTO          "lvgl_photo"



//SR
#define SR_OTHER_JPG          "other_jpg"
#define SR_OTHER_JPG_USB1          "other_jpg_usb1"
#define SR_OTHER_JPG_USB2          "other_jpg_usb2"

#define SR_VIDEO_USB          "video_usb"

#define SR_ZBAR_JPG          "zbar_parse"
//高16位是大类型(统一宏),低16位是细分类型
#define SET_DATA_TYPE(type1,type2) (type1<<16 | type2)
#define GET_DATA_TYPE1(type) (type>>16)
#define GET_DATA_TYPE2(type) (type&0xffff)




enum data_type1
{
  TYPE1_NONE,
  JPEG = 1,
  SOUND,
  YUV,
  H264,
};

//0保留
enum DATA_TYPE2_RECV_ALL
{
  RESEVER_VALUE,
  RECV_ALL = RESEVER_VALUE,
};


//定义mjpeg的类型,最好自己将不同类型归类
enum JPEG_data_type2
{
  JPEG_NONE = RESEVER_VALUE,
  JPEG_DVP_NODE,  //采用特殊的节点方式
  JPEG_FULL,      //jpeg的data是完整的图片,不采用节点方式
  JPEG_DVP_FULL, //采用整张图保存的方式(在空间足够下,采用正常图片形式,正常是带psram才会使用)
  JPEG_USB,
  JPEG_FILE,
  LVGL_JPEG,
  SET_JPEG_MSG,
  LVGL_JPEG_TO_RGB,
  JPEG_NORMAL_DATA, //正常模式的data,代表用普通方式去读取jpg就可以了
};


//定义声音的类型
enum SOUND_data_type2
{
  SOUND_ALL = RECV_ALL,
  SOUND_NONE,//不接收声音,源头不要产生这个声音就好了
  SOUND_MIC ,
  SOUND_FILE,
  SOUND_INTERCOM,
  SOUND_MP3,
  SOUND_AMRNB,
  SOUND_AMRWB,
};

enum YUV_data_type2
{
  YUV_P0      =  BIT(0),
  YUV_P1      =  BIT(1),
  YUV_OTHER   =  BIT(2),
};


//定义lvgl的video命令的类型
enum LVGL_VIDEO_data_type2
{
  CMD_LVGL_NONE = 0,
  CMD_LVGL_VIDEO_DEL,
};


enum
{
  CMD_VIDEO_PLAYER_NONE,
  CMD_VIDEO_PLAYER_EXIT,
};



//设置发送的命令,只有type1是在这里定义,type2由各个模块去定义(这个就不再有固定,因为自定义,所以在要自己在各个STREAM_SEND_CMD里面去处理识别)

#define SET_CMD_TYPE(type1,type2) (type1<<16 | type2)
#define GET_CMD_TYPE1(type) (type>>16)
#define GET_CMD_TYPE2(type) (type&0xffff)

enum cmd_type1
{
  CMD_NONE,
  CMD_AUDIO_DAC,
  CMD_AUDIO_DAC_MODIFY_HZ,  //采样率修改cmd
  CMD_JPEG_FILE,
  CMD_LVGL_VIDEO,              //lvgl 的video命令
  CMD_VIDEO_PLAYER,
};

#endif