#include "vad_process.h"
#include "typesdef.h"
#include "osal/string.h"
#include "osal/task.h"
#include "osal_file.h"
#include "custom_mem.h"

#ifndef MIN
#define  MIN(A, B)        ((A) < (B) ? (A) : (B))
#endif

#ifndef MAX
#define  MAX(A, B)        ((A) > (B) ? (A) : (B))
#endif

void *vadInst =NULL;

int vad_init(int16_t vad_mode)
{
    vadInst = WebRtcVad_Create();
    if (vadInst == NULL) return -1;
    int status = WebRtcVad_Init(vadInst);
    if (status != 0)
    {
        printf("WebRtcVad_Init fail\n");
        WebRtcVad_Free(vadInst);
        return -1;
    }
    status = WebRtcVad_set_mode(vadInst, vad_mode);
    if (status != 0)
    {
        printf("WebRtcVad_set_mode fail\n");
        WebRtcVad_Free(vadInst);
        return -1;
    }
    return 0;
}

int vad_process(int16_t *buffer, uint32_t sampleRate, size_t samplesCount, int per_ms_frames)
{
    if (!buffer) return -1;
    if (samplesCount == 0) return -1;
    // kValidRates : 8000, 16000, 32000, 48000
    // 10, 20 or 30 ms frames
    per_ms_frames = MAX(MIN(30, per_ms_frames), 10);
    size_t samples = sampleRate * per_ms_frames / 1000;
    if (samples == 0) return -1;
    int16_t *input = buffer;
    size_t nTotal = (samplesCount / samples);
    uint32_t totalVadRet = 0;

    for (int i = 0; i < nTotal; i++)
    {
        int keep_weight = 0;
        int nVadRet = WebRtcVad_Process(vadInst, sampleRate, input, samples, keep_weight);
        if (nVadRet == -1)
        {
            printf("failed in WebRtcVad_Process\n");
            WebRtcVad_Free(vadInst);
            return -1;
        }
        else
        {
            totalVadRet += nVadRet;
        }
        input += samples;
    }
    return totalVadRet;
}

//static int16 read_data[240];
//void vad_test(void)
//{
//    void *in_fp = osal_fopen("infile.pcm","rb");
//    void *out_fp = osal_fopen("outfile.pcm","wb");
//
//	int read_len = -1;
//    int32 vad_ret = 0;
//	
//    while(1)
//    {
//		read_len = osal_fread(read_data, 2, 240, in_fp);
//		if(read_len < 480)
//			break;
//		vad_ret = vad_process(read_data, 8000, 240, 30);
//        if(vad_ret != 0)
//		    osal_fwrite(read_data, 2, 240, out_fp);
//    }
//	osal_fclose(in_fp);
//	osal_fclose(out_fp);
//	printf("\n*******vad test finish********\n");
//}
//struct os_task vad_test_task;
//void vad_test_thread(void)
//{
//	printf("\n**********vad_test_thread*************\n");
//	vad_init(kVadNormal);
//	OS_TASK_INIT("vad_test",&vad_test_task,vad_test,0,OS_TASK_PRIORITY_NORMAL,2048);
//}