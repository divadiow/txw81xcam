#include "agc_process.h"
#include "typesdef.h"
#include "osal/string.h"
#include "osal/task.h"
#include "osal_file.h"
#include "custom_mem.h"

#ifndef MIN
#define  MIN(A, B)        ((A) < (B) ? (A) : (B))
#endif

static void *agcInst = NULL;

int agc_init(int16_t agcMode, uint32_t sampleRate)
{
    agcInst = WebRtcAgc_Create();
    if (agcInst == NULL) return -1;

    int minLevel = 0;
    int maxLevel = 1024;

    WebRtcAgcConfig agcConfig;
    agcConfig.compressionGaindB = 50; // default 9 dB
    agcConfig.limiterEnable = 1; // default kAgcTrue (on)
    agcConfig.targetLevelDbfs = 12; // default 3 (-3 dBOv)

    int status = WebRtcAgc_Init(agcInst, minLevel, maxLevel, agcMode, sampleRate);
    if (status != 0) {
        printf("WebRtcAgc_Init fail\n");
        WebRtcAgc_Free(agcInst);
        return -1;
    }
    status = WebRtcAgc_set_config(agcInst, agcConfig);
    if (status != 0) {
        printf("WebRtcAgc_set_config fail\n");
        WebRtcAgc_Free(agcInst);
        return -1;
    }
    return 0;
}

int agc_process(int16_t *buffer, uint32_t sampleRate, size_t samplesCount) {
    if (!buffer) return -1;
    if (samplesCount == 0) return -1;

    size_t samples = MIN(160, sampleRate / 100);
    if (samples == 0) return -1;
    int16_t *input = buffer;
    size_t nTotal = (samplesCount / samples);
    
    size_t num_bands = 1;
    int inMicLevel, outMicLevel = -1;
    int16_t out_buffer[samples];
    int16_t *out16 = out_buffer;
    uint8_t saturationWarning = 1;                 //是否有溢出发生，增益放大以后的最大值超过了65536
    int16_t echo = 0;                                 //增益放大是否考虑回声影响
    for (int i = 0; i < nTotal; i++) {
        inMicLevel = 0;
        int nAgcRet = WebRtcAgc_Process(agcInst, (const int16_t *const *) &input, num_bands, samples,
                                        (int16_t *const *) &out16, inMicLevel, &outMicLevel, echo,
                                        &saturationWarning);
        if (nAgcRet != 0) {
            os_printf("failed in WebRtcAgc_Process\n");
            WebRtcAgc_Free(agcInst);
            return -1;
        }
        os_memcpy(input, out_buffer, samples * sizeof(int16_t));
        input += samples;
    }

    const size_t remainedSamples = samplesCount - nTotal * samples;
    if (remainedSamples > 0) {
        if (nTotal > 0) {
            input = input - samples + remainedSamples;
        }

        inMicLevel = 0;
        int nAgcRet = WebRtcAgc_Process(agcInst, (const int16_t *const *) &input, num_bands, samples,
                                        (int16_t *const *) &out16, inMicLevel, &outMicLevel, echo,
                                        &saturationWarning);

        if (nAgcRet != 0) {
            os_printf("failed in WebRtcAgc_Process during filtering the last chunk\n");
            WebRtcAgc_Free(agcInst);
            return -1;
        }
        os_memcpy(&input[samples-remainedSamples], &out_buffer[samples-remainedSamples], remainedSamples * sizeof(int16_t));
        input += samples;
    }

    return 1;
}

//static int16 read_data[240];
//void agc_test(void)
//{
//    void *in_fp = osal_fopen("infile.pcm","rb");
//    void *out_fp = osal_fopen("outfile.pcm","wb");
//
//	int read_len = -1;
//    while(1)
//    {
//		read_len = osal_fread(read_data, 2, 240, in_fp);
//		if(read_len < 480)
//			break;
//		agc_process(read_data, 8000, 240);
//		osal_fwrite(read_data, 2, 240, out_fp);
//    }
//	osal_fclose(in_fp);
//	osal_fclose(out_fp);
//	printf("\n*******agc test finish********\n");
//}
//struct os_task agc_test_task;
//void agc_test_thread(void)
//{
//	printf("\n**********agc_test_thread*************\n");
//	agc_init(kAgcModeAdaptiveDigital, 8000);
//	OS_TASK_INIT("agc_test",&agc_test_task,agc_test,0,OS_TASK_PRIORITY_NORMAL,2048);
//}