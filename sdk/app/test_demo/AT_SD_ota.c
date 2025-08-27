#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "osal/task.h"
#include "osal_file.h"
#include "lib/ota/fw.h"
#include "tx_platform.h"
#include "dev/csi/hgdvp.h"

#define SD_CACHE_SIZE   (4*1024)
int32 demo_atcmd_sd_ota(const char *cmd, char *argv[], uint32 argc)
{
    if(argc < 1)
    {
        os_printf("argv is too less\n");
        return 0;
    }

#if DVP_EN	
		//dvp_close();
		void *dvp = (void *)dev_get(HG_DVP_DEVID);
		if(dvp)
		{
			dvp_close(dvp);
		}
#endif

    uint8_t *cache_buf = NULL;
    const char *sd_ota_filename = argv[0];
    void *fp = osal_fopen(sd_ota_filename,"r");
    if(!fp)
    {
        os_printf("%s file not exist\n",sd_ota_filename);
        goto demo_atcmd_sd_ota_end;
    }
    uint32_t filesize = osal_fsize(fp);
    uint32_t filesize_tmp = filesize;
    uint32_t readsize = SD_CACHE_SIZE;
    uint32_t ota_offset = 0;
    cache_buf = (uint8_t*)os_malloc(SD_CACHE_SIZE);
    if(!cache_buf)
    {
        goto demo_atcmd_sd_ota_end;
    }
    os_printf("filesize:%d\t%X\n",filesize,cache_buf);


    while(filesize)
    {
        if(filesize < SD_CACHE_SIZE)
        {
            readsize = filesize;
        }

        osal_fread(cache_buf,readsize,1,fp);
        libota_write_fw(filesize_tmp,ota_offset,cache_buf,readsize);
        filesize -= readsize;
        ota_offset += readsize;
    }


demo_atcmd_sd_ota_end:
    if(fp)
    {
        osal_fclose(fp);
    }

    if(cache_buf)
    {
        os_free(cache_buf);
    }
    return 0;
}