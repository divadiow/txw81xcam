#include "sys_config.h"
#include "typesdef.h"
#include "adkey.h"
#include "keyScan.h"
 
#include "dev/adc/hgadc_v0.h"
#include "hal/gpio.h"
#include "osal/string.h"


static void key_adkey_init(key_channel_t *key,uint8_t enable)
{
    adkey_t *adkey = (adkey_t *)key->priv;
    struct hgadc_v0 *adc = (struct hgadc_v0*)dev_get(HG_ADC0_DEVID);

    
    if(enable)
    {
        adc_open((struct adc_device *)adc);	
        gpio_set_mode(adkey->pin,adkey->pull,adkey->pull_level);
        adc_add_channel((struct adc_device *)adc, adkey->pin);	
        os_printf("%s:%d\n",__FUNCTION__,__LINE__);
        adkey->priv = (void*)adc;
        key->enable = 1;
    }
    else
    {
        adc_open((struct adc_device *)adc);	
        adc_delete_channel((struct adc_device *)adc, adkey->pin);
        os_printf("%s:%d\n",__FUNCTION__,__LINE__);
        adkey->priv = (void*)adc;
        key->enable = 0;
    }


}

static uint8 key_adkey_scan(key_channel_t *key)
{
    adkey_t *adkey = (adkey_t *)key->priv;
    uint32 vol;
    struct adkey_scan_code *key_scan = (struct adkey_scan_code*)key->key_table;
    adc_get_value((struct adc_device *)adkey->priv, adkey->pin, &vol);
	//os_printf("vol:%d\t%X\n",vol,adkey->pin);
    //记录当前adc的值,用与发送到应用层,至于应用层是否需要,由应用层去管理
    key->extern_value = vol;
    for(;;)
    {
        if(vol>=key_scan->adc)
        {
            key_scan++;
        }
        else
        {
            key_scan--;
            break;
        }
    }
    //printf("key_scan->key:%d\tvol:%d\n",key_scan->key,vol);
    return key_scan->key;
}








/*********************************************************
 *                      adkey的参数配置
 * 
    default: 4080
    up:536
    down:0
    left:1682
    right:1100
    press:2200
    A:3915
    B:3345
    C:2800
    D:NULL
默认开发板先检查每一个按键的值,然后大概每一个ad-100填到下表
************************************************************/
#if 0
static const struct adkey_scan_code adkey_table[] = 
{
	{0,     AD_DOWN},
	{400,   AD_UP},
	{1000,  AD_RIGHT},
	{1500,  AD_LEFT},
    {2100,  AD_PRESS},
	{2700,  AD_C},
	{3200,  AD_B},
    {3800,  AD_A},
    {4000,  KEY_NONE},
    {4096,  KEY_NONE},
};
#else

static const struct adkey_scan_code adkey_table[] = 
{
	{0,     AD_A},
	{900,  AD_B},
	{1800,  AD_C},
	{2800,  AD_LEFT},
    //{3900,  AD_PRESS},
    {3950,  KEY_NONE},
    {4096,  KEY_NONE},
};

#endif

static const keys_t adkey_arg = 
{
    .period_long     = 500,
    .period_repeat   = 1000,
    .period_dither = 80,
};


static adkey_t adkey= {
  .priv = NULL,
  .pin  = PA_1,
  .pull = GPIO_PULL_NONE,
  .pull_level = GPIO_PULL_LEVEL_NONE,
};



//外部调用
key_channel_t adkey_key2 = 
{
  .init       = key_adkey_init,
  .scan       = key_adkey_scan,
  .prepare    = NULL,
  .priv       = (void*)&adkey,
  .key_arg    = &adkey_arg,//按键的参数,可能不同的类型按键,参数不一样
  .key_table  = &adkey_table,
};



