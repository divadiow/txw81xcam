#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "string.h"
#include "osal_file.h"
#include "osal/task.h"
#include "osal/string.h"
#include "mp3_decode.h"

static uint8_t play_mode = 0;
char audio_filePath[10];

int32 demon_atcmd_play_mp3(const char *cmd, char *argv[], uint32 argc)
{
    if(argc < 2)
    {
        os_printf("%s argc too small:%d,enter the path and mode\n",__FUNCTION__,argc);
        return 0;
    }
	if(argv[0])
	{
		memset(audio_filePath,0,sizeof(audio_filePath));
		memcpy(audio_filePath,argv[0],strlen(argv[0]));
		play_mode = os_atoi(argv[1]);

        if(play_mode == 0) {
            set_mp3_decode_status(MP3_STOP);
        }
        else if(play_mode == 1) {
            mp3_decode_init(audio_filePath, NULL);

        }
	}
	os_printf("%s %s\n",__FUNCTION__,audio_filePath);
	return 0;
}

