#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "hal/i2s.h"
#include "hal/i2c.h"
#include "osal/sleep.h"
#include  "hal_i2s_audio.h"
#include "osal/string.h"








#define __iis_sram_acton            __at_section(".iiscode")

__iis_sram_acton static void demo_sram(uint32 sys_con,uint32 con)
{
    /* 下面三句要放在SRAM上跑 */
    //enable iis0
    (*((volatile uint32 *)0x40004900)) |= BIT(0);
    

    SYSCTRL_REG_OPT(
        SYSCTRL->SYS_CON2 = sys_con;
    );

    //iis 1 set rx
    (*((volatile uint32 *)0x40004A00)) = con;
}


static void audio_8311_irq_handle(uint32 irq, uint32 irq_data) 
{
	audio_i2s_config *priv = (audio_i2s_config*)irq_data;
	void *buf;
	int buf_size;
	//speaker_user_action* speaker_act = (speaker_user_action*)priv->speaker_management->app_action;
    if (I2S_IRQ_FLAG_HALF == irq) 
	{
		buf = priv->set_buf(priv->priv_el,&priv->reg_node,&buf_size);
		
		if(buf)
		{
			priv->irq_func(priv->i2s_dev , buf, buf_size);
		}
		else
		{
			if(priv->play_empty_buf)
			{
				priv->irq_func(priv->i2s_dev , priv->play_empty_buf, priv->buf_size);
			}
		}

       // _os_printf("A");
    } 
	else if (I2S_IRQ_FLAG_FULL == irq) 
    {

		//半中断有配置新的buf,则将完成的录音帧通知应用层,如果为NULL,则代表没有配置新的buf,只能使用旧的buf
		if(priv->reg_node)
		{
			//buf_size大部分是无效值,如果是喇叭,get_buf则基本是空函数,如果是mic,get_buf则是返回已经录音完成的buf
			priv->get_buf(priv->priv_el,priv->current_node,&buf_size);
			priv->current_node = priv->reg_node;
			priv->reg_node = NULL;
		}

    }

}







//play_config是喇叭配置,mic_config麦克风配置,只要是非NULL,默认传入的参数是对的
int audio_i2s_install(audio_i2s_config *play_config,audio_i2s_config *mic_config)
{
	int res = RET_ERR;
	int duplex_en = 0;
	struct i2s_device *i2s_dev;
	audio_i2s_config *config;
	int i2s_devid;
	int		sample_freq = ~0;
	int		sample_bit  = ~0;
	int 	data_format = ~0;
	void *buf;
	int buf_size;
	if(!play_config && !mic_config)
	{
		goto audio_i2s_install_end;
	}
	//需要同步
	if(play_config  && mic_config)
	{
		duplex_en = 1;
		 *(unsigned int *)0x40020000=0x3fac87e4;
	    SYSCTRL_REG_OPT(
	        SYSCTRL->SYS_CON3 = (SYSCTRL->SYS_CON3 & ~(0x01 << 27)) | (1 << 27);
	    );
		extern unsigned int _iis_start;
		extern unsigned int _iis_end;
		unsigned int iis_start = (unsigned int)&_iis_start;
	    unsigned int iis_end = (unsigned int)&_iis_end;
		unsigned int iis_sram_start = (unsigned int)demo_sram;
		os_memcpy((void*)iis_sram_start,(void*)iis_start,iis_end-iis_start);

	}
	else
	{
		 *(unsigned int *)0x40020000=0x3fac87e4;
	    SYSCTRL_REG_OPT(
	        SYSCTRL->SYS_CON3 = (SYSCTRL->SYS_CON3 & ~(0x01 << 27));
	    );

	}

	//打开对应i2s,前提是i2s的id要设置对,否则就会出现异常,后续只是获取id设备,不验证是否为i2s设备,强行转换
	if(play_config && play_config->i2s_devid != ~0)
	{
		config = play_config;
		config->irq_func = (i2s_irq_func)i2s_write;
		i2s_devid = config->i2s_devid;
		i2s_dev = (struct i2s_device *)dev_get(i2s_devid);
		if(!i2s_dev)
		{
			res = I2S_DEV_NULL;
			goto audio_i2s_install_end;
		}

		if(!config->play_empty_buf)
		{
			res = I2S_PLAY_EMPTY_BUF_NULL;
			goto audio_i2s_install_end;
		}

		if(!config->set_buf && !config->get_buf)
		{
			res = I2S_REGISTER_FUNC_NULL;
			goto audio_i2s_install_end;
		}


		config->i2s_dev = i2s_dev;
		sample_freq = config->sample_freq;
		sample_bit  = config->sample_bit;
		data_format = config->data_fmt;
		i2s_open(i2s_dev , I2S_MODE_MASTER, sample_freq, sample_bit);
		//设置duplex_en
		i2s_ioctl(i2s_dev,I2S_IOCTL_CMD_SET_DUPLEX,duplex_en);
		i2s_ioctl(i2s_dev, I2S_IOCTL_CMD_SET_DATA_FMT, data_format);
		i2s_request_irq(i2s_dev , I2S_IRQ_FLAG_HALF | I2S_IRQ_FLAG_FULL, (i2s_irq_hdl)audio_8311_irq_handle, (uint32)config);
		config->irq_func(i2s_dev , config->play_empty_buf, config->buf_size);
	    

	}

	if(mic_config && mic_config->i2s_devid != ~0)
	{


		config = mic_config;
		config->irq_func = i2s_read;
		i2s_devid = config->i2s_devid;
		i2s_dev = (struct i2s_device *)dev_get(i2s_devid);

		if(!config->set_buf && !config->get_buf)
		{
			res = I2S_REGISTER_FUNC_NULL;
			goto audio_i2s_install_end;
		}

		if(!i2s_dev)
		{
			res = I2S_DEV_NULL;
			goto audio_i2s_install_end;
		}
		config->i2s_dev = i2s_dev;
		data_format = config->data_fmt;
		if(duplex_en == 1)
		{
			i2s_open(i2s_dev , I2S_MODE_MASTER, sample_freq, sample_bit);
		}
		else
		{
			i2s_open(i2s_dev , I2S_MODE_MASTER, config->sample_freq, config->sample_bit);
		}

		//设置duplex_en
		i2s_ioctl(i2s_dev,I2S_IOCTL_CMD_SET_DUPLEX,duplex_en);
		i2s_ioctl(i2s_dev, I2S_IOCTL_CMD_SET_DATA_FMT, data_format);
		i2s_request_irq(i2s_dev , I2S_IRQ_FLAG_HALF | I2S_IRQ_FLAG_FULL, (i2s_irq_hdl)audio_8311_irq_handle, (uint32)config);
		buf = config->set_buf(config->priv_el,&config->current_node,&buf_size);
		if(!buf)
		{
			res = I2S_MIC_FIRST_BUF_NULL;
			goto audio_i2s_install_end;
		}
		config->irq_func(i2s_dev, buf, buf_size);
	}

	//如果是同步,则运行同步操作
	if(duplex_en)
	{
		uint32 con = 0;
		uint32 sys_con = 0;
	
		__disable_irq();
		con  = (*((volatile uint32 *)0x40004900));
		con &= ~ BIT(8);
		con |=	 BIT(0);
		
		sys_con   = SYSCTRL->SYS_CON2;
		sys_con  &= ~ BIT(0);
		demo_sram(sys_con,con);
		__enable_irq();
	
	}

	res = RET_OK;
	audio_i2s_install_end:
	return res;
}

int audio_i2s_uninstall(audio_i2s_config *play_config,audio_i2s_config *mic_config)
{
	int res = RET_ERR;
	int i2s_devid;
	struct i2s_device *i2s_dev;
	audio_i2s_config *config;
	if(play_config && play_config->i2s_devid != ~0)
	{
		config = play_config;
		i2s_devid = config->i2s_devid;
		i2s_dev = (struct i2s_device *)dev_get(i2s_devid);
		if(!i2s_dev)
		{
			goto audio_i2s_uninstall_end;
		}
		i2s_close(i2s_dev);
	}

	if(mic_config && mic_config->i2s_devid != ~0)
	{
		config = mic_config;
		i2s_devid = config->i2s_devid;
		i2s_dev = (struct i2s_device *)dev_get(i2s_devid);
		if(!i2s_dev)
		{
			goto audio_i2s_uninstall_end;
		}
		i2s_close(i2s_dev);

	}
	res = I2S_OK;
	audio_i2s_uninstall_end:
	return res;

}






