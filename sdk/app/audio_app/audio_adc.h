#ifndef __AUDIO_ADC_H
#define __AUDIO_ADC_H


struct audio_adc_s
{
	void *adc_msgq;
	void *audio_hardware_hdl;
	struct os_task     thread_hdl;
	void *queue;
};
int audio_adc_init();
#endif