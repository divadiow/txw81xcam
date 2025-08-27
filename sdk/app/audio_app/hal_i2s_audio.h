#ifndef __AUDIO_8311_H
#define __AUDIO_8311_H
#include "hal/i2s.h"

#define PLAY_MODE	0
#define MIC_MODE	1


typedef void *(*audio_i2s_set_buf)(void *priv_el,void *el_point,int *buf_size);
typedef void (*audio_i2s_get_buf)(void *priv_el,void *el_point, int *buf_size);
typedef int32 (*i2s_irq_func)(struct i2s_device *i2s, void* buf, uint32 len);


enum
{
	I2S_OK,
	I2S_CONFIG_NULL,//
	I2S_DEV_NULL,//
	I2S_PLAY_EMPTY_BUF_NULL,
	I2S_REGISTER_FUNC_NULL,
	I2S_MIC_FIRST_BUF_NULL,
}I2S_ERR;



//在有mic和喇叭的时候,sample_freq与sample_bit应该要一致,所以只有一个是有效值,默认是以喇叭的配置为准
typedef struct audio_i2s_config
{
	//无需用户配置
	struct i2s_device *i2s_dev;
	void 	*current_node;
	void 	*reg_node;
	i2s_irq_func irq_func;

	//用户配置
	uint8	type;	//0:喇叭   1:麦克风
	int		i2s_devid;
	int		sample_freq;
	int		sample_bit;	
	int		data_fmt;			//I2S_DATA_FMT_I2S   I2S_DATA_FMT_MSB  I2S_DATA_FMT_LSB   I2S_DATA_FMT_PCM
	void 	*play_empty_buf;	//作为喇叭的时候,需要配置,size与buf_size一致,可以是malloc也可以是固定,如果是malloc,需要自己去free
	int 	 buf_size;			
	audio_i2s_set_buf set_buf;
	audio_i2s_get_buf get_buf;
	void *priv_el;
	
}audio_i2s_config;








int audio_8311_init(int i2c_devid) ;
int audio_i2s_install(audio_i2s_config *play_config,audio_i2s_config *mic_config);
int audio_i2s_uninstall(audio_i2s_config *play_config,audio_i2s_config *mic_config);



#endif
