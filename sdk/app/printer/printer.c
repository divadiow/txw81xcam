#include "sys_config.h"
#include "typesdef.h"
#include "dev.h"
#include "devid.h"
#include "hal/gpio.h"
#include "hal/dvp.h"
#include "hal/vpp.h"
#include "hal/adc.h"
#include "hal/spi.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "hal/pwm.h"
#include "hal/timer_device.h"
#include "osal/task.h"

#if PRINTER_EN

/*==================================打印机引脚定义==================================*/
#define  MOTO_SLEEP_PIN	     PB_7		    // LOW sleep , HIGH run

#define  PRINTER_DATA_PIN	 PB_13
#define  PRINTER_CLK_PIN	 PB_11
#define  PRINTER_LATCH_PIN	 PB_8
#define  PRINTER_TM_PIN	 	 PC_5
#define  PRINTER_PSENSOR_PIN PA_0
#define  PRINTER_STB_PIN	 PB_10
#define  PRINTER_EN_PIN	     PA_7

#define  PRINTER_DATA_PORT   GPIOB
#define  PRINTER_DATA_NUM    13
#define  PRINTER_CLK_PORT    GPIOB
#define  PRINTER_CLK_NUM     11
#define  PRINTER_STB_PORT    GPIOB
#define  PRINTER_STB_NUM     10

#define PRINTER_W            384
#define PRINTER_H            512


/*
	第一组PWM (3.59 s):	打开SPI_SEND_EN、关闭STB_PIN_PWM_EN
						printer_delay:    1,7			

	第二组PWM (4.62 s)： 关闭SPI_SEND_EN、打开STB_PIN_PWM_EN
						printer_delay：   1,17

			  (4.63 s):	打开SPI_SEND_EN、关闭STB_PIN_PWM_EN
						printer_delay：   1,53

	第三组PWM (6.16 s)： 关闭SPI_SEND_EN、打开STB_PIN_PWM_EN
						printer_delay:   21,57

			  (6.18 s): 打开SPI_SEND_EN、打开STB_PIN_PWM_EN
			  			printer_delay:   21,86
						
	第四组PWM（8.19 s）：打开SPI_SEND_EN、打开STB_PIN_PWM_EN
						printer_delay:   1,26
*/
#define PRINTER_PWM_1_EN 		 0
#define PRINTER_PWM_2_EN		 0
#define PRINTER_PWM_3_EN		 1
#define PRINTER_PWM_4_EN		 0

/*====================================第一组 PWM====================================*/
#if   PRINTER_PWM_1_EN

#define SPI_SEND_EN 		 1				//使用SPI时序发送数据，可控发送速率
#define STB_PIN_PWM_EN 		 1              //STB 使用PWM波形，使能置1否则置0

#define TOTAL_TICK           256            //打印每行调用timer7_noisr_func函数的次数

#define SPI_CLK_FREQ		 8000000		//SPI时钟频率8M ，CLK脚高电平稳定时间约为50-60ns
#define STB_PIN_PWM_PERIOD 	 1000           
#define STB_PIN_PWM_DUTY 	 900            //改变PWM的占空比可以改变加热时长
#define DELAY_NOHEAT		 1
#define DELAY_HEAT			 7


/*====================================第二组 PWM====================================*/
#elif PRINTER_PWM_2_EN

#define SPI_SEND_EN 		 1				//使用SPI时序发送数据，可控发送速率
#define STB_PIN_PWM_EN 		 1              //STB 使用PWM波形，使能置1否则置0

#define TOTAL_TICK           256            //打印每行调用timer7_noisr_func函数的次数

#if SPI_SEND_EN
#define SPI_CLK_FREQ		 8000000		//SPI时钟频率8M ，CLK脚高电平稳定时间约为50-60ns
#define STB_PIN_PWM_PERIOD 	 1000           
#define STB_PIN_PWM_DUTY 	 900            //改变PWM的占空比可以改变加热时长
#define DELAY_NOHEAT		 1
#define DELAY_HEAT			 53
#else
#define SPI_CLK_FREQ		 8000000		//SPI时钟频率8M ，CLK脚高电平稳定时间约为50-60ns
#define STB_PIN_PWM_PERIOD 	 1000           
#define STB_PIN_PWM_DUTY 	 900            //改变PWM的占空比可以改变加热时长
#define DELAY_NOHEAT		 1
#define DELAY_HEAT			 17
#endif

/*====================================第三组 PWM====================================*/
#elif PRINTER_PWM_3_EN

#define SPI_SEND_EN 		 1				//使用SPI时序发送数据，可控发送速率
#define STB_PIN_PWM_EN 		 0              //STB 使用PWM波形，使能置1否则置0
#define HEAT_MOTO_SEPARATION (1 && SPI_SEND_EN)

#define TOTAL_TICK           256            //打印每行调用timer7_noisr_func函数的次数

#if  SPI_SEND_EN
#define SPI_CLK_FREQ		 8000000		//SPI时钟频率8M ，CLK脚高电平稳定时间约为50-60ns
#define STB_PIN_PWM_PERIOD 	 1000           
#define STB_PIN_PWM_DUTY 	 600            //改变PWM的占空比可以改变加热时长
#define DELAY_NOHEAT		 1
#define DELAY_HEAT			 106
#else
#define SPI_CLK_FREQ		 8000000		//SPI时钟频率8M ，CLK脚高电平稳定时间约为50-60ns
#define STB_PIN_PWM_PERIOD 	 1000           
#define STB_PIN_PWM_DUTY 	 600            //改变PWM的占空比可以改变加热时长
#define DELAY_NOHEAT		 21
#define DELAY_HEAT			 57
#endif

/*====================================第四组 PWM====================================*/
#elif PRINTER_PWM_4_EN

#define SPI_SEND_EN 		 1				//该组暂时只能置1
#define STB_PIN_PWM_EN 		 0             //STB 使用PWM波形，使能置1否则置0

#define TOTAL_TICK           512            //打印每行调用timer7_noisr_func函数的次数

#if SPI_SEND_EN
#define SPI_CLK_FREQ		 8000000		//SPI时钟频率8M ，CLK脚高电平稳定时间约为50-60ns
#define STB_PIN_PWM_PERIOD 	 1000           
#define STB_PIN_PWM_DUTY 	 900            //改变PWM的占空比可以改变加热时长
#define DELAY_NOHEAT		 1
#define DELAY_HEAT			 26
#endif

#endif

struct hgpwm_v0 *moto_hgpwm;
struct hgadc_v0* hgadc_tm;
struct  timer_device* tmr7_dev_g;
struct os_task handle_printer_task;

extern const unsigned char printer_demo[196608];                                //384 x 512 只包含Y数据的图片
uint8 printer_buf[196608] __attribute__ ((aligned(4),section(".psram.src")));   //384 x 512
#if SPI_SEND_EN
struct spi_device *spi0_dev;
uint8 printer_buf_odd[48] __attribute__ ((aligned(4)));							//48*8 = 384
uint8 printer_buf_even[48] __attribute__ ((aligned(4)));						//48*8 = 384
#endif
const unsigned char *printer_picture_addr;
volatile uint32_t printer_h = 0;

typedef struct {
    __IO uint32_t MODE;
    __IO uint32_t OTYPE;
    __IO uint32_t OSPEEDL;
    __IO uint32_t OSPEEDH;
    __IO uint32_t PUPL;
    __IO uint32_t PUPH;
    __IO uint32_t PUDL;
    __IO uint32_t PUDH;
    __IO uint32_t IDAT;
    __IO uint32_t ODAT;
    __IO uint32_t BSR;
    __IO uint32_t RES0;
    __IO uint32_t AFRL;
    __IO uint32_t AFRH;
    __IO uint32_t TGL;
    __IO uint32_t IMK;
    __IO uint32_t RES1;
    __IO uint32_t RES2;
    __IO uint32_t RES3;
    __IO uint32_t DEBEN;
    __IO uint32_t AIOEN;
    __IO uint32_t PND;
    __IO uint32_t PNDCLR;
    __IO uint32_t TRG0;
    __IO uint32_t RES4;
    __IO uint32_t RES5;
    __IO uint32_t RES6;
    __IO uint32_t RES7;
    __IO uint32_t IEEN;
    __IO uint32_t IOFUNCOUTCON0;
    __IO uint32_t IOFUNCOUTCON1;
    __IO uint32_t IOFUNCOUTCON2;
    __IO uint32_t IOFUNCOUTCON3;
} GPIO_TypeDef;

#define GPIOA                   ((GPIO_TypeDef       *) HG_GPIOA_BASE)
#define GPIOB                   ((GPIO_TypeDef       *) HG_GPIOB_BASE)
#define GPIOC                   ((GPIO_TypeDef       *) HG_GPIOC_BASE)

static void printer_moto_delay(uint32_t n)
{
	volatile uint32_t i=500*n;
	while(i--)
	{
	}
}

static void printer_data_delay(uint32_t n)
{
	volatile uint32_t i=n;

	while(i--)
	{
	}
}

static void printer_delay(uint32_t n)
{
	volatile uint32_t i=20*n;
	while(i--)
	{
	}
}

void printer_moto_enable(uint8_t en)
{
	if(en)
	{
		gpio_set_val(MOTO_SLEEP_PIN,1);
		printer_moto_delay(20);
	}
	else
	{
		gpio_set_val(MOTO_SLEEP_PIN,0);
	}
}


int32_t get_printer_tm(){
	uint32 vol = 0;
	printer_delay(100);
	hgadc_tm = (struct hgadc_v0*)dev_get(HG_ADC0_DEVID);
	adc_open((struct adc_device *)(hgadc_tm));	
	adc_add_channel((struct adc_device *)hgadc_tm, PRINTER_TM_PIN);
	adc_get_value((struct adc_device *)hgadc_tm, PRINTER_TM_PIN, &vol);
	os_printf("temperature:%d\r\n",vol);
	return vol;
}

uint8_t is_paper_insert(){
	return gpio_get_val(PRINTER_PSENSOR_PIN);
}

void paper_io_init(){

	gpio_iomap_output(MOTO_SLEEP_PIN,GPIO_IOMAP_OUTPUT);
#if SPI_SEND_EN
	spi0_dev = (struct spi_device *)dev_get(HG_SPI0_DEVID);
	spi_ioctl(spi0_dev,SPI_CFG_SET_NONE_CS,1,0);
	spi_ioctl(spi0_dev,SPI_SET_LSB_FIRST,0,0);
	spi_ioctl(spi0_dev,SPI_KICK_DMA_EN,0,0);

	spi_open(spi0_dev,SPI_CLK_FREQ,SPI_MASTER_MODE,SPI_WIRE_SINGLE_MODE,SPI_CPOL_0_CPHA_0);
#else
	gpio_iomap_output(PRINTER_DATA_PIN,GPIO_IOMAP_OUTPUT);
	gpio_iomap_output(PRINTER_CLK_PIN,GPIO_IOMAP_OUTPUT);
#endif

	gpio_iomap_output(PRINTER_LATCH_PIN,GPIO_IOMAP_OUTPUT);
	gpio_set_val(PRINTER_LATCH_PIN,1);
	
	gpio_iomap_output(PRINTER_STB_PIN,GPIO_IOMAP_OUTPUT);
	gpio_iomap_output(PRINTER_EN_PIN,GPIO_IOMAP_OUTPUT);
	
	gpio_iomap_output(PRINTER_TM_PIN,GPIO_IOMAP_OUTPUT);
	

	gpio_set_mode(PRINTER_PSENSOR_PIN, GPIO_PULL_NONE, GPIO_PULL_LEVEL_NONE);
	gpio_iomap_input(PRINTER_PSENSOR_PIN,GPIO_IOMAP_INPUT);

	/////////////pwm init//////////////
	gpio_iomap_output(MOTO_AIN1_PIN,GPIO_IOMAP_OUTPUT);     //PWM_CHANNEL_0  a
	gpio_iomap_output(MOTO_AIN2_PIN,GPIO_IOMAP_OUTPUT);		//PWM_CHANNEL_1  c
	gpio_iomap_output(MOTO_BIN1_PIN,GPIO_IOMAP_OUTPUT);		//PWM_CHANNEL_2  b 
	gpio_iomap_output(MOTO_BIN2_PIN,GPIO_IOMAP_OUTPUT);		//PWM_CHANNEL_3  d

	moto_hgpwm    = (struct hgpwm_v0 *)dev_get(HG_PWM0_DEVID);	

	pwm_init((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_0, 1000, 1000); 
	pwm_ioctl((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_0, PWM_IOCTL_CMD_SET_PRESCALER, (uint32)0x7, 0);      //配置分频比
	pwm_start((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_0);	
	pwm_init((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_1, 1000, 1000);
	pwm_ioctl((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_1, PWM_IOCTL_CMD_SET_PRESCALER, (uint32)0x7, 0);      //配置分频比
	pwm_start((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_1);	
	pwm_init((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_2, 1000, 1000);
	pwm_ioctl((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_2, PWM_IOCTL_CMD_SET_PRESCALER, (uint32)0x7, 0);      //配置分频比
	pwm_start((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_2);	
	pwm_init((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_3, 1000, 1000);
	pwm_ioctl((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_3, PWM_IOCTL_CMD_SET_PRESCALER, (uint32)0x7, 0);      //配置分频比
	pwm_start((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_3);		
#if STB_PIN_PWM_EN
	pwm_init((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_4, STB_PIN_PWM_PERIOD, 0);
	pwm_start((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_4);
#endif

}

void moto_pwm_start()
{
#if (HEAT_MOTO_SEPARATION && PRINTER_PWM_3_EN)
	pwm_start((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_0);
	pwm_start((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_1);
	pwm_start((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_2);
	pwm_start((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_3);
#endif
}

void moto_pwm_stop()
{
#if (HEAT_MOTO_SEPARATION && PRINTER_PWM_3_EN)
	pwm_stop((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_0);
	pwm_stop((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_1);
	pwm_stop((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_2);
	pwm_stop((struct pwm_device *)moto_hgpwm, PWM_CHANNEL_3);
#endif
}

void paper_io_deinit(){
	struct hgadc_v0* hgadc_tm;
	hgadc_tm = (struct hgadc_v0*)dev_get(HG_ADC0_DEVID);
	adc_close((struct adc_device *)(hgadc_tm));
	adc_delete_channel((struct adc_device *)(hgadc_tm),PRINTER_TM_PIN);
	pwm_deinit((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_0);
	pwm_deinit((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_1);
	pwm_deinit((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_2);
	pwm_deinit((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_3);

#if STB_PIN_PWM_EN
	pwm_deinit((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_4);
#endif

#if SPI_SEND_EN
	spi_close(spi0_dev);
#endif
}

void printer_mote_run(){
	uint32_t flags;
	flags = disable_irq();

#if  PRINTER_PWM_1_EN
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_3,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 1086*24, 407*24);//MOTO_BIN2_PIN    1086 ==>13.9ms
	printer_delay(8285);    																				//3.46ms	
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_1,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 1086*24, 407*24);//MOTO_AIN2_PIN
	printer_delay(8285);
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_2,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 1086*24, 407*24);//MOTO_BIN1_PIN
	printer_delay(8285);
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_0,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 1086*24, 407*24);//MOTO_AIN1_PIN
#elif PRINTER_PWM_2_EN
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_3,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 1406*24, 527*24);//MOTO_BIN2_PIN    1406 ==>18.0ms
	printer_delay(10775);																					//4.5ms	
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_1,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 1406*24, 527*24);//MOTO_AIN2_PIN    
	printer_delay(10775);																						
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_2,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 1406*24, 527*24);//MOTO_BIN1_PIN    
	printer_delay(10775);																						
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_0,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 1406*24, 527*24);//MOTO_AIN1_PIN  
#elif PRINTER_PWM_3_EN
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_3,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 1875*24, 703*24);//MOTO_BIN2_PIN    1875 ==>24.0ms 4:1
	printer_delay(14367);																					//6ms	
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_1,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 1875*24, 703*24);//MOTO_AIN2_PIN    
	printer_delay(14367);																						
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_2,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 1875*24, 703*24);//MOTO_BIN1_PIN    
	printer_delay(14367);																						
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_0,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 1875*24, 703*24);//MOTO_AIN1_PIN 	
#elif PRINTER_PWM_4_EN
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_3,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 2500*24, 938*24);//MOTO_BIN2_PIN    2500 ==>32.0ms 4:1
	printer_delay(19200);																					//8ms	
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_1,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 2500*24, 938*24);//MOTO_AIN2_PIN    
	printer_delay(19200);																						
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_2,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 2500*24, 938*24);//MOTO_BIN1_PIN    
	printer_delay(19200);																						
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_0,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 2500*24, 938*24);//MOTO_AIN1_PIN 	

#endif
	enable_irq(flags);	
}

void printer_mote_stop(){
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_0,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 50, 0);
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_1,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 50, 0);
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_2,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 50, 0);
	pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_3,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, 50, 0);
}


bool timer7_noisr_func(){
	#define HOTTIME  900
	uint32_t itk = 0;
	uint32_t jtk = 0;
	static uint32_t i = 0;
	static uint32_t j = 0;
	static uint32_t tick = 0;
	static uint32_t hot_time = 0;
	static uint32_t grep_step = 0;
	static uint8_t half_s = 0;
	static uint8_t hot_add = 0;
	uint32 offset_h;
	uint8_t *pt;
	pt = printer_buf;
	if(printer_h == 0){
		tick = 0;
		printer_h = 1;
		grep_step = 0;
	}
	offset_h = (printer_h-1)*PRINTER_W;

#if SPI_SEND_EN

	if((tick%2) == 0){
#if STB_PIN_PWM_EN
		pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_4,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, STB_PIN_PWM_PERIOD, STB_PIN_PWM_DUTY);
#else
		PRINTER_STB_PORT->ODAT |= BIT(PRINTER_STB_NUM);
#endif

#if 1
		if(hot_time > HOTTIME)
			goto deal_end;
#endif

		if(half_s){

            //偶数点
			spi_ioctl((struct spi_device*)spi0_dev, SPI_KICK_DMA_TX, (uint32)&printer_buf_even, 48);

#if PRINTER_PWM_4_EN
			for(jtk = 0; jtk < 48; jtk++)
			{
				for(itk = 1 + i; itk < 8; itk+=4)
				{
					if((pt[offset_h+jtk*8+itk]) > 0 && (1)){
						pt[offset_h+jtk*8+itk]--;
						printer_buf_odd[jtk] |= (0x1 << (7-itk));
					}
				}
			}
			if(i == 2)
				i = 0; 
			else
				i = 2;
#else
			for(jtk = 0; jtk < 48; jtk++)
			{
				for(itk = 1 + i; itk < 8; itk+=2)
				{
					if((pt[offset_h+jtk*8+itk]) > 0 && (1)){
						pt[offset_h+jtk*8+itk]--;
						printer_buf_odd[jtk] |= (0x1 << (7-itk));
					}
				}
			}
#endif 

		} else {

			//奇数点
			spi_ioctl((struct spi_device*)spi0_dev, SPI_KICK_DMA_TX, (uint32)&printer_buf_odd, 48);

#if PRINTER_PWM_4_EN
			for(jtk = 0; jtk < 48; jtk++)
			{
				for(itk = 0 + j; itk < 8; itk+=4)
				{
					if((pt[offset_h+jtk*8+itk]) > 0 && (1)){
						pt[offset_h+jtk*8+itk]--;
						printer_buf_even[jtk] |= (0x1 << (7-itk));
					}
				}
			}
			if(j == 2)
				j = 0;
			else
				j = 2;
#else
			for(jtk = 0; jtk < 48; jtk++)
			{
				for(itk = 0 + j; itk < 8; itk+=2)
				{
					if((pt[offset_h+jtk*8+itk]) > 0 && (1)){
						pt[offset_h+jtk*8+itk]--;
						printer_buf_even[jtk] |= (0x1 << (7-itk));
					}
				}

			}

#endif
		}

		while(!spi_ioctl((struct spi_device*)spi0_dev, SPI_GET_DMA_PENDING, 0, 0));

		spi_ioctl((struct spi_device*)spi0_dev, SPI_CLEAR_DMA_PENDING, 1, 0);

	}
	else{
#if PRINTER_PWM_3_EN
#if !HEAT_MOTO_SEPARATION
	#if STB_PIN_PWM_EN
		pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_4,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, STB_PIN_PWM_PERIOD, 0);
	#else
		PRINTER_STB_PORT->ODAT &= ~BIT(PRINTER_STB_NUM);
	#endif
#endif
#else
	#if STB_PIN_PWM_EN
		pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_4,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, STB_PIN_PWM_PERIOD, 0);
	#else
		PRINTER_STB_PORT->ODAT &= ~BIT(PRINTER_STB_NUM);
	#endif
#endif

#if 1
		if(hot_time > HOTTIME)
			goto deal_end;	
#endif	

		if(half_s)
			memset(printer_buf_even, 0, 48);
		else
			memset(printer_buf_odd , 0, 48);


		gpio_set_val(PRINTER_LATCH_PIN,0);
		gpio_set_val(PRINTER_LATCH_PIN,1);
		
		half_s ^= BIT(0);
		hot_add++;
		if((hot_add%2) == 0){
			grep_step++;
		}
	}

#else

	if((tick%2) == 0){
#if STB_PIN_PWM_EN
		pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_4,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, STB_PIN_PWM_PERIOD, STB_PIN_PWM_DUTY);
#else
		PRINTER_STB_PORT->ODAT |= BIT(PRINTER_STB_NUM);
#endif

#if 1
		if(hot_time > HOTTIME)
			goto deal_end;
#endif



		if(half_s){
			for(itk = 0;itk < PRINTER_W;itk++){
				if(itk%2 == 0){                 //偶数点
					if((pt[offset_h+itk]) > 0 && (1)){
						pt[offset_h+itk]--;
						PRINTER_DATA_PORT->ODAT |= BIT(PRINTER_DATA_NUM);
					}else{
						PRINTER_DATA_PORT->ODAT &= ~BIT(PRINTER_DATA_NUM);
					}
				} else {			            //奇数点
					PRINTER_DATA_PORT->ODAT &= ~BIT(PRINTER_DATA_NUM);
				}
				PRINTER_CLK_PORT->ODAT |= BIT(PRINTER_CLK_NUM);
				__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();		//45 ns - 50 ns
				PRINTER_CLK_PORT->ODAT &= ~BIT(PRINTER_CLK_NUM);
			}
		}else{
			for(itk = 0;itk < PRINTER_W;itk++){
				if(itk%2 == 1){                 //奇数点
					if((pt[offset_h+itk]) > 0 && (1)){
						pt[offset_h+itk]--;
						PRINTER_DATA_PORT->ODAT |= BIT(PRINTER_DATA_NUM);
					}else{
						PRINTER_DATA_PORT->ODAT &= ~BIT(PRINTER_DATA_NUM);
					}
				} else {					    //偶数点
					PRINTER_DATA_PORT->ODAT &= ~BIT(PRINTER_DATA_NUM);
				}
				PRINTER_CLK_PORT->ODAT |= BIT(PRINTER_CLK_NUM);
				__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();		//45 ns - 50 ns
				PRINTER_CLK_PORT->ODAT &= ~BIT(PRINTER_CLK_NUM);
			}
		}
	}
	else{
#if STB_PIN_PWM_EN
		pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_4,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, STB_PIN_PWM_PERIOD, 0);
#else
		PRINTER_STB_PORT->ODAT &= ~BIT(PRINTER_STB_NUM);
#endif

#if 1
		if(hot_time > HOTTIME)
			goto deal_end;	
#endif	
	
		gpio_set_val(PRINTER_LATCH_PIN,0);
		gpio_set_val(PRINTER_LATCH_PIN,1);
		
		half_s ^= BIT(0);
		hot_add++;
		if((hot_add%2) == 0){
			grep_step++;
		}
	}

#endif

deal_end:
	if((tick % TOTAL_TICK) == 0){
		printer_h++;        
		hot_time = 0;
		grep_step = 0;
#if (HEAT_MOTO_SEPARATION && PRINTER_PWM_3_EN)
		#if STB_PIN_PWM_EN
				pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_4,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY, STB_PIN_PWM_PERIOD, 0);
		#else
				PRINTER_STB_PORT->ODAT &= ~BIT(PRINTER_STB_NUM);
		#endif
		moto_pwm_start();
		printer_delay(28734); //2x14367 16ms
		moto_pwm_stop();
#endif
		if(printer_h == PRINTER_H){
		#if STB_PIN_PWM_EN
			pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_4,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY,STB_PIN_PWM_PERIOD,0);
		#else
			PRINTER_STB_PORT->ODAT &= ~BIT(PRINTER_STB_NUM);
		#endif
			return 0;
		}
	}
	tick++;
	hot_time++;

	return 1;
}

/**
 * @brief 设置打印机的图片地址，赋值给全局变量printer_picture_addr
 * 
 * @param addr 传入的图片地址
 * @return true 
 * @return false 
 */
bool printer_set_picture_addr(const unsigned char* addr)
{
	if(addr == NULL) {
		os_printf("printer demo addr invaild!!! addr:%x\n",addr);
		return 1;
	}

	printer_picture_addr = addr;
	
	return 0;
};

/**
 * @brief 切换打印机与摄像头的使能
 * 
 * @param printer_en 如果使能打印机、关闭摄像头则置1，否则置0
 */
void printer_dvp_en_exchange(int printer_en)
{

#if DVP_EN
	struct dvp_device *dvp_device = (struct dvp_device*)dev_get(HG_DVP_DEVID);
	struct vpp_device *vpp_device = (struct vpp_device*)dev_get(HG_VPP_DEVID);
#endif

	os_printf("%s %d\r\n",__FUNCTION__,printer_en);

	//打印机使能
	if(printer_en)
	{
		gpio_set_val(PRINTER_EN_PIN,1);

#if DVP_EN
		gpio_iomap_output(PIN_DVP_PDN,GPIO_IOMAP_OUTPUT);
		gpio_set_val(PIN_DVP_PDN,1);
		dvp_close(dvp_device);
		vpp_close(vpp_device);

#endif
	}
	//DVP使能
	else {
		gpio_set_val(PRINTER_EN_PIN,0);

#if DVP_EN
		bool csi_ret;
		bool csi_cfg();
		bool csi_open();

		gpio_iomap_output(PIN_DVP_PDN,GPIO_IOMAP_OUTPUT);
		gpio_set_val(PIN_DVP_PDN,0);
#if SDH_I2C2_REUSE
		uint32 sdhost_i2c2_exchange(int sdh_stop_en);
		sdhost_i2c2_exchange(1);
#endif
		csi_ret = csi_cfg();
		if(csi_ret)
			csi_open();
		//vpp_close(vpp_device);
#if SDH_I2C2_REUSE
		sdhost_i2c2_exchange(0);
#endif

#endif
	}
}


void enhanceContrast(uint8_t *image) {
    int minLevel = 63; 
    int maxLevel = 0;  
    
    for (int i = 0; i < PRINTER_W * PRINTER_H; i++) {
        if (image[i] < minLevel) {
            minLevel = image[i];
        }
        if (image[i] > maxLevel) {
            maxLevel = image[i];
        }
    }
    
    int dynamicRange = maxLevel - minLevel;
    
    for (int i = 0; i < PRINTER_W * PRINTER_H; i++) {
        image[i] = (image[i] - minLevel) * 63 / dynamicRange;

        if (image[i] < 0) {
            image[i] = 0;
        }
        if (image[i] > 63) {
            image[i] = 63;
        }
    }
}

unsigned char restrain_int(int input) {
    if (input < 0x00) {
        return 0x00;
    }

    if (input > 0xFF) {
        return 0xFF;
    }

    return input;
}

//Floyd-Steinberg算法,该函数只适用于256灰阶图像
void dithering(uint8_t *image, int width, int height) {
    // 对每个像素执行误差扩散
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            int index = y * width + x;
            unsigned char oldPixel = image[index];
            unsigned char newPixel = (oldPixel < 128) ? 0 : 255; // 二值化

            // 计算误差
            int error = oldPixel - newPixel;

            // 更新当前像素
            image[index] = newPixel;

            // 误差扩散到相邻像素
            if(x < width - 1) {
                image[index + 1] = restrain_int(image[index + 1] + (error * 7 / 16));
            }
            if(y < height - 1) {
                image[index + width] = restrain_int(image[index + width] + (error * 5 / 16));
            }
            if(x < width - 1 && y < height - 1) {
                image[index + width + 1] = restrain_int(image[index + width + 1] + (error * 1 / 16));
            }
            if(x > 0 && y < height - 1) {
                image[index + width - 1] = restrain_int(image[index + width - 1] + (error * 3 / 16));
            }

        }
    }
}

extern volatile uint8  printer_action;
extern uint8_t vga_room[2][640*480+640*480/2];
bool printer_work(){
	uint32_t flags;
	uint8_t ret;
#if !SPI_SEND_EN
	uint32_t print_w = PRINTER_W;
#endif
	uint32_t itk = 0;
	uint32_t ktk = 0;
	uint32_t jtk = 0;
	int32_t tm;
	unsigned int grep_step = 0;
	os_sleep_ms(1000);

	while(1){
		
		if(printer_action == 0){
			os_sleep_ms(50);
			continue;
		}
		printer_action = 0;
		printer_h = 0;
		printf("-->Printer0 open!\r\n");
		paper_io_init();												//打印机IO初始化
		//gpio_set_val(PRINTER_EN_PIN,1);								//enable printer power
		printer_dvp_en_exchange(1);
		printer_delay(500); 
		printf("-->Printer0 open  finish!\r\n");
		if(is_paper_insert())											// check paper
		{
			//gpio_set_val(PRINTER_EN_PIN,0);							//disable printer power
			printer_dvp_en_exchange(0);
			printf("-->Printer error...no paper!!!\r\n");
			continue;
		}
		
		tm = get_printer_tm();											//check tm
		if(1 == tm) // hot
		{
			//gpio_set_val(PRINTER_EN_PIN,0);							//disable printer power
			printer_dvp_en_exchange(0);
			printf("-->Get the TM error...\r\n");
			continue;
		}
	
	
		memset(printer_buf,0,196608);									//384 x 512
#if 1
		dithering(vga_room, PRINTER_W, PRINTER_H);						//半色调散点特效
#endif
		//printer_set_picture_addr(printer_demo); 						
		printer_set_picture_addr(vga_room);
		for(itk = 0; itk < 512;itk++){								//384 x 512
			for(ktk = 0; ktk < 384;ktk++){
				grep_step = 0;
				for(jtk = 0;jtk < 64;jtk++){								//配置打印图片量化系数
					if(printer_picture_addr[itk*384+ktk] < (grep_step*4)/*255*/){ //64 4
						printer_buf[itk*384+383-ktk]++;
					}
					grep_step++;
				}
			}	
		}

		enhanceContrast(printer_buf);

	
#if SPI_SEND_EN
		memset(printer_buf_even, 0, 48);
		memset(printer_buf_odd, 0, 48);
	
		for(jtk = 0; jtk < 48; jtk++)
		{
			for(itk = 1; itk < 8; itk+=2)
			{
				if((printer_buf[jtk*8+itk]) > 0 && (1)){
					printer_buf[jtk*8+itk]--;
					printer_buf_odd[jtk] |= (1 << (7-itk));
				}
			}
		}
	
#endif
			
	
#if SPI_SEND_EN
		flags = disable_irq();	
		memset(printer_buf_even,0xFF,48);
		spi_ioctl((struct spi_device*)spi0_dev, SPI_KICK_DMA_TX, (uint32)&printer_buf_even, 48);
		while(!spi_ioctl((struct spi_device*)spi0_dev, SPI_GET_DMA_PENDING, 0, 0));
		spi_ioctl((struct spi_device*)spi0_dev, SPI_CLEAR_DMA_PENDING, 1, 0);
		enable_irq(flags);
		memset(printer_buf_even,0,48);
#else
		for(itk = 0;itk < print_w;itk++){
			gpio_set_val(PRINTER_DATA_PIN,1);
			gpio_set_val(PRINTER_CLK_PIN,1);
			gpio_set_val(PRINTER_CLK_PIN,0);
		}
#endif
		gpio_set_val(PRINTER_LATCH_PIN,0);
		gpio_set_val(PRINTER_LATCH_PIN,1);
	
	
		flags = disable_irq();	
	
#if STB_PIN_PWM_EN
		pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_4,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY,STB_PIN_PWM_PERIOD,STB_PIN_PWM_DUTY);
#else
		gpio_set_val(PRINTER_STB_PIN,1);	//预加热
#endif
			
		printer_delay(70000);				// 25ms

		enable_irq(flags);

	
#if STB_PIN_PWM_EN
		pwm_ioctl((struct pwm_device *)moto_hgpwm,PWM_CHANNEL_4,PWM_IOCTL_CMD_SET_PERIOD_DUTY_IMMEDIATELY,STB_PIN_PWM_PERIOD,0);
#else
		gpio_set_val(PRINTER_STB_PIN,0);
#endif
	
		mcu_watchdog_timeout(0);


		flags = disable_irq();	
		printer_moto_enable(1);
		printer_mote_run();
		moto_pwm_stop();
		while(1){
			printer_delay(DELAY_NOHEAT);	   // 2.39个delay = 1us  
			ret = timer7_noisr_func();
			if(ret == 0)
				break;
			printer_delay(DELAY_HEAT);
			ret = timer7_noisr_func();
			if(ret == 0)
				break;	
		}
		enable_irq(flags);

		mcu_watchdog_timeout(5);
		
		moto_pwm_start();
		printer_delay(1000000);
		moto_pwm_stop();
		//gpio_set_val(PRINTER_EN_PIN,0);	  //disable printer power

		paper_io_deinit();
		printer_dvp_en_exchange(0);
		printer_mote_stop();
	
		

		printf("printer   stop....\r\n");

	}

}

void printer_thread_init(){
	OS_TASK_INIT("handle_printer", &handle_printer_task, printer_work, NULL, OS_TASK_PRIORITY_NORMAL, 1024);
}

#endif