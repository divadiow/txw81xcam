#include <stdlib.h>
#include <stdio.h>
#include "noise_suppression_x.h"
#include "nsx_defines.h"
#include "typesdef.h"
#include "osal/string.h"
#include "osal/task.h"
#include "osal_file.h"
#include "custom_mem.h"

NsxHandle *nsInst = NULL;

int ns_init(uint32 samplerate)
{
	nsInst = WebRtcNsx_Create();
	if(nsInst == NULL)
	{
        printf("WebRtcNs_Create err\n");
        return -1;
	}
    if (WebRtcNsx_Init(nsInst, samplerate) == -1) {
        printf("WebRtcNs_Init err\n");
        return -1;
    }
	return 0;
}

void ns_process(int16 *inbuffer, uint32 samplerate, uint32 samplecount)
{
	int nBands = 1;
	uint32 samples = 80;
	uint32 frames = samplecount / samples;
	
    for (int i = 0; i < frames; i++)
    {
        WebRtcNsx_Process(nsInst, inbuffer, nBands, inbuffer);
        inbuffer += samples;
    }
}

//static int16_t read_data[240];
//void ns_test(void)
//{
//	os_sleep_ms(2000);
//    void *in_fp = osal_fopen("infile.pcm","rb");
//    void *out_fp = osal_fopen("outfile.pcm","wb");
//	if((!in_fp) || (!out_fp))
//		return;
//    int read_len = -1;
//	uint32 time = 0;
//	ns_init(8000);
//    while(1)
//    {
//		read_len = osal_fread(read_data, 2, 240, in_fp);
//		if(read_len < 480)
//			break;
//		time = os_jiffies();	
//		ns_process(read_data, 8000, 240);
//		printf("t:%d\n",(os_jiffies()-time));
//		osal_fwrite(read_data, 2, 240, out_fp);
//		os_sleep_ms(1);
//    }
//
//    printf("\n*******ns test finish********\n");
//	osal_fclose(in_fp);
//	osal_fclose(out_fp);
//}
//
//struct os_task ns_test_task;
//void ns_test_thread(void)
//{
//	printf("\n**********ns_test_thread*************\n");
//	OS_TASK_INIT("ns_test",&ns_test_task,ns_test,0,OS_TASK_PRIORITY_NORMAL,5120);
//}