#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"

#include "hal/pdm.h"
#include <stdio.h>
#include "string.h"
#include "pdm_audio.h"

struct audio_priv
{
	struct hgpdm_v0*   pdm_hdl;
	void *current_node;
	void *reg_node;
	//buf暂定open后,就不改变
	int buf_size;
	set_buf set_buf;
	get_buf get_buf;

	//私有结构元素
	void *priv_el;
	
};

static void audio_irq(uint32 irq, uint32 irq_data) 
{
    
  //半中断,切换下一个buf,如果不存在,使用当前的
  struct audio_priv *priv = (struct audio_priv *)irq_data;
  void *buf;
  if(irq == PDM_IRQ_FLAG_DMA_HF)
  {
	buf = priv->set_buf(priv->priv_el,&priv->reg_node);
	//如果reg_node为NULL,则音频录音buf还是原来的buf
	if(priv->reg_node)
	{
		pdm_read((struct pdm_device *)priv->pdm_hdl, buf, priv->buf_size);
	}
  }
  else if(irq == PDM_IRQ_FLAG_DMA_OV)
  {
	  //半中断有配置新的buf,则将完成的录音帧通知应用层,如果为NULL,则代表没有配置新的buf,只能使用旧的buf
	  if(priv->reg_node)
	  {
		  priv->get_buf(priv->priv_el,priv->current_node);
		  priv->current_node = priv->reg_node;
		  priv->reg_node = NULL;
	  }

  }
    
}


void  *pdm_audio_open(enum pdm_sample_freq freq,enum pdm_channel channel)
{
	int ret = 0;
	int res = RET_OK;
	struct audio_priv *priv = NULL;
	
	priv = (struct audio_priv*)malloc(sizeof(struct audio_priv));
	if(!priv)
	{
		ret = -1;
		goto audio_open_err;
	}
	memset(priv,0,sizeof(struct audio_priv));
	priv->pdm_hdl = (struct hgpdm_v0*)dev_get(HG_PDM0_DEVID);
	if(!priv->pdm_hdl)
	{
		ret = -2;
		goto audio_open_err;
	}

	//启动pdm
    res = pdm_open((struct pdm_device*)priv->pdm_hdl, freq, channel);
	if(res != RET_OK)
	{
	ret = -3;
	goto audio_open_err;

	}
    pdm_request_irq((struct pdm_device*)priv->pdm_hdl, PDM_IRQ_FLAG_DMA_OV, (pdm_irq_hdl)audio_irq, (uint32_t)priv);
	pdm_request_irq((struct pdm_device*)priv->pdm_hdl, PDM_IRQ_FLAG_DMA_HF, (pdm_irq_hdl)audio_irq, (uint32_t)priv);  
	return priv;

	audio_open_err:
	if(priv)
	{
		free(priv);
	}
	return NULL;
}

void pdm_audio_register(void *audio_hdl,void *priv_el,int play_size,set_buf audio_set_buf,get_buf audio_get_buf)
{
	struct audio_priv *priv = (struct audio_priv*)audio_hdl;
	priv->buf_size = play_size;
	priv->set_buf = audio_set_buf;
	priv->get_buf = audio_get_buf;
	priv->priv_el = priv_el;
	
}

int pdm_audio_start(void *audio_hdl)
{
	int ret = 0;
	int res = 0;
	void *buf;
	struct audio_priv *priv = (struct audio_priv*)audio_hdl;
	buf = priv->set_buf(priv->priv_el,&priv->current_node);

	if(!buf)
	{
		ret = -1;
		goto audio_run_err;
	}
    res = pdm_read((struct pdm_device *)priv->pdm_hdl, buf, priv->buf_size);
	//pdm参数输入错误
	if(res < 0)
	{
		ret = -2;
		goto audio_run_err;
	}

	audio_run_err:
	return res;


}

void pdm_audio_close(void *audio_hdl)
{
	struct audio_priv *priv;
	if(audio_hdl)
	{
		priv = (struct audio_priv*)audio_hdl;
		pdm_close((struct pdm_device*)priv->pdm_hdl);
		free(priv);
	}
}

