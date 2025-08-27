#ifndef __AMR_APP_H
#define __AMR_APP_H

#define AMRNB_MAGIC_NUMBER "#!AMR\n"
#define AMRWB_MAGIC_NUMBER "#!AMR-WB\n"

#define AMRNB_TO_PCM_SIZE   (160*8) //一定是160的倍数
#define AMRWB_TO_PCM_SIZE   (320*4) //一定是160的倍数
enum
{
    AMR_NOT_RUN, //代表可能创建了线程,但没有实际运行(可能是创建线程失败或者被其他高优先级任务一直卡住)
    AMR_PLAY,
    AMR_PAUSE,
    AMR_STOP,
};
struct amr_extern_read;

typedef uint32_t (*_amr_read)(struct amr_extern_read *extern_read,void *ptr,uint32_t offset,uint32_t size);
typedef void (*_amr_close)(struct amr_extern_read *extern_read,void *data);
typedef void (*_amr_seek)(struct amr_extern_read *extern_read,uint32_t offset);
//读取mp3的接口,外部先将fp创建,然后注册read和close的函数
struct amr_extern_read
{
    void *fp;
    uint32_t readsize;
    _amr_read amr_read;
    _amr_close amr_close;
    _amr_seek amr_seek;
    uint8_t *sd_buf;
    uint8_t status; //用于控制是否继续编码或者是否为暂停、播放、退出几个状态,只是由外部去控制
    uint8_t inside_status;  //内部状态,由内部去修改状态
};

uint8_t get_amrnb_decode_status();
uint8_t set_amrnb_decode_status(uint8_t status);
void amr_decode_thread(void *d);


#endif