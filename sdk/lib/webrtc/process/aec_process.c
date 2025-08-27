#include "aec_process.h"
#include "typesdef.h"
#include "osal/string.h"
#include "osal/task.h"
#include "custom_mem.h"

#ifndef MIN
#define  MIN(A, B)        ((A) < (B) ? (A) : (B))
#endif

void *aecmInst = NULL;

int aec_init(uint32_t sampleRate, int16_t nMode)
{	
    AecmConfig config;
    config.cngMode = AecmTrue;
    config.echoMode = 4;// 0, 1, 2, 3 (default), 4
    aecmInst = WebRtcAecm_Create();
    if(!aecmInst) {
        printf("\n******create aecm err\n");
        return -1;
    }
    WebRtcAecm_Init(aecmInst, sampleRate);
    WebRtcAecm_set_config(aecmInst, config);
	return 0;
}

int push_farbuf(short *data, unsigned int len, uint32_t sampleRate)
{
    uint32_t samples = MIN(160, sampleRate / 100);
    if (samples == 0)
        return -1;
    uint32_t nCount = (len / samples);
    int16_t *far_input = data;
    for (uint32_t i = 0; i < nCount; i++) {
        WebRtcAecm_BufferFarend(aecmInst, far_input, samples);
        far_input += samples;
    }
	return 0;
}

int aec_process(int16_t *near_frame, uint32_t inSampleCount, int16_t msInSndCardBuf, uint32_t sampleRate)
{
    if (!near_frame) return -1;   
    if (inSampleCount == 0) return -1;

    uint32_t samples = MIN(160, sampleRate / 100);
    if (samples == 0)
        return -1;
    const int maxSamples = 160;
    int16_t *near_input = near_frame;
    uint32_t nCount = (inSampleCount / samples);
    if (aecmInst == NULL) return -1;
    
    int16_t out_buffer[maxSamples];
    for (uint32_t i = 0; i < nCount; i++) {
        int nRet = WebRtcAecm_Process(aecmInst, near_input, NULL, out_buffer, samples, msInSndCardBuf);

        if (nRet != 0) {
            printf("failed in WebRtcAecm_Process\n");
            WebRtcAecm_Free(aecmInst);
            return -1;
        }
        memcpy(near_input, out_buffer, samples * sizeof(int16_t));
        near_input += samples;
    }
    return 1;
}


//char near_buf[1024],far_buf1[1024];
//void aec_test(void)
//{
//    FILE *near_fp = osal_fopen("near.pcm","rb");
//    FILE *far_fp = osal_fopen("far.pcm","rb");
//    FILE *output_fp = osal_fopen("out.pcm","wb");
//
//    int near_read,far_read;
//
////    char near_buf[1024],far_buf[1024];
//    AecmConfig config;
//    config.cngMode = AecmTrue;
//    config.echoMode = 4;// 0, 1, 2, 3 (default), 4
//    void *aecmInst = WebRtcAecm_Create();
//    printf("aecm:%X\n",aecmInst);
//    WebRtcAecm_Init(aecmInst, 8000);
//    WebRtcAecm_set_config(aecmInst, config);
//    int ret;
//    while(1)
//    {
//        near_read = osal_fread(near_buf,1,1016,near_fp); //下一次播放的数据
//        far_read = osal_fread(far_buf1,1,1016,far_fp);    //上一次
//        if(near_read == 0)
//        {
//            break;
//        }
//        ret = mcu_AECM(aecmInst,(int16_t *)near_buf, (int16_t *)far_buf1, 8000,near_read/2);
//        if(ret<0)
//        {
//            break;
//        }
//        osal_fwrite(near_buf,1,near_read,output_fp);
//
//    }
//    if(ret>0)
//    {
//        WebRtcAecm_Free(aecmInst);
//    }
//    osal_fclose(near_fp);
//    osal_fclose(far_fp);
//    osal_fclose(output_fp);
//    printf("\n*******press any key to exit. \n");
//}
//struct os_task aec_test_task;
//void aec_test_thread(void)
//{
//	printf("\n**********aec_test_thread*************\n");
//	OS_TASK_INIT("aec_test",&aec_test_task,aec_test,0,OS_TASK_PRIORITY_NORMAL,10240);
//}