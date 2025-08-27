#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "devid.h"
#include "hal/gpio.h"
#include "hal/spi.h"
#include "hal/timer_device.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/csi/hgdvp.h"
#include "dev/prc/hgprc.h"
#include "hal/pwm.h"
#include "hal/prc.h"
#include "osal/semaphore.h"
#include "custom_mem/custom_mem.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "osal/task.h"

#if PRC_EN
#define IIC_CLK 120000UL
extern SNSER snser;



#define RESET_HIGH()		{gpio_set_val(rsn,1);}
#define RESET_LOW()		    {gpio_set_val(rsn,0);}


struct os_task dvp_manual_handle;
extern volatile uint32 outbuff_isr;
volatile uint8 prc_buf_num = 0;

struct	spi_device*   spi0_dev_g; 
struct	prc_device*   prc_dev_g; 

//uint8  spi_dat_buf[2][480*16];    //spi0 & spi1的buf缓存
uint8  *spi_dat_buf[2];

uint8  prc_yuv_line[240*16+240*16/2]__attribute__ ((aligned(4)));
uint8 psram_spi_sensor[3][240*320+240*320/2] __attribute__ ((aligned(16),section(".psram.src")));

uint32 read_len  = 0;
uint32 read_app_len  = 0;
uint32 last_len_num  = 0;
extern struct i2c_device *iic_test;
uint32 kick_en = 1;
uint8 spi_to_jpg_open = 0;
extern Vpp_stream photo_msg;
volatile uint8 prc_to_mjpg = 0;

_Sensor_Adpt_ * sensorAutoCheck(struct i2c_device *p_iic,uint8 *init_buf);
extern _Sensor_Ident_ *devSensorInit;
const _Sensor_Ident_ null_init2={0x00,0x00,0x00,0x00,0x00,0x00};



void prc_deal_irq(uint32 irq, uint32 irq_data);
void spi0_read_irq(uint32 irq, uint32 irq_data);





static const _Sensor_Ident_ *devSensorInitTable_spi[] = {

#if DEV_SENSOR_BF30A2
	&bf30a2_init,
#endif

	NULL,
};

static const _Sensor_Adpt_ *devSensorOPTable_spi[] = {

#if DEV_SENSOR_BF30A2
	&bf30a2_cmd,
#endif

};

void prc_module_init(){
	prc_dev_g = (struct prc_device*)dev_get(HG_PRC_DEVID);
	prc_init(prc_dev_g);
	prc_set_width(prc_dev_g,240);
	prc_set_yuv_mode(prc_dev_g,0);
	
	
	prc_set_yaddr(prc_dev_g,(uint32)prc_yuv_line);
	prc_set_uaddr(prc_dev_g,(uint32)prc_yuv_line+240*16);
	prc_set_vaddr(prc_dev_g,(uint32)prc_yuv_line+240*16+240*16/4);
	
	prc_request_irq(prc_dev_g,PRC_ISR,(prc_irq_hdl)prc_deal_irq,(uint32)prc_dev_g);
}

uint8 spi_sensor_to_mjpeg_is_run(){
	return prc_to_mjpg;
}

uint8 spi_to_jpg_cfg(uint8 enable){
	if(enable){
		prc_to_mjpg = 1;
		jpg_cfg(HG_JPG0_DEVID,PRC_DATA);
	}else{
		prc_to_mjpg = 0;
	}
	return prc_to_mjpg;
}



void spi_sensor_cfg(){
	spi0_dev_g = (struct spi_device*)dev_get(HG_SPI1_DEVID);
	spi_open(spi0_dev_g, 10000000, SPI_SLAVE_MODE, SPI_WIRE_SINGLE_MODE, SPI_CPOL_0_CPHA_0);	
	spi_ioctl(spi0_dev_g,SPI_CFG_SET_NONE_CS,1,0);
	spi_ioctl(spi0_dev_g,SPI_SENSOR_VSYNC_TIMEOUT ,0xea60,0);
	spi_ioctl(spi0_dev_g,SPI_SENSOR_VSYNC_HEAD_CNT,0,0);
	spi_ioctl(spi0_dev_g,SPI_SENSOR_VSYNC_EN,1,0);
	spi_ioctl(spi0_dev_g,SPI_SENSOR_HSYNC_TIMEOUT,2400,0);
	spi_ioctl(spi0_dev_g,SPI_SENSOR_HSYNC_HEAD_CNT,6,0);
	spi_ioctl(spi0_dev_g,SPI_HIGH_SPEED_CFG,0,0);
	spi_ioctl(spi0_dev_g,SPI_SENSOR_PINGPANG_EN,1,0);
	spi_ioctl(spi0_dev_g,SPI_SENSOR_BUF_LEN,480*16,0);
	if(spi_dat_buf[0] == NULL){
		spi_dat_buf[0] = (uint8  *)custom_malloc(480*16);
	}

	if(spi_dat_buf[1] == NULL){
		spi_dat_buf[1] = (uint8  *)custom_malloc(480*16);
	}
	
	spi_ioctl(spi0_dev_g,SPI_SENSOR_SET_ADR0,(uint32)spi_dat_buf[0],0);
	spi_ioctl(spi0_dev_g,SPI_SENSOR_SET_ADR1,(uint32)spi_dat_buf[1],0);
	spi_ioctl(spi0_dev_g,SPI_RX_TIMEOUT_CFG,1,180000);
	spi_request_irq(spi0_dev_g,SPI_IRQ_FLAG_RX_DONE|SPI_IRQ_FLAG_RX_TIMEOUT,spi0_read_irq,(uint32)spi0_dev_g);		
	spi_ioctl(spi0_dev_g,SPI_KICK_DMA_EN,0,0);
}

volatile uint32 hnum = 0;
volatile uint32 sync_start = 0;
volatile uint32 display_addr = 0;
volatile uint32 prc_new_frame;
volatile int flow_data_cnt;
volatile uint8 prc_deal_num = 0;

uint8 spi_video_run = 0;
//只有主spi才能保证数据是准确的
void spi0_read_irq(uint32 irq, uint32 irq_data){
	
	if(irq == SPI_IRQ_FLAG_RX_DONE){
		prc_new_frame = 0;
		if(sync_start){
			if((hnum%2) == 0){
				prc_set_src_addr(prc_dev_g,(uint32)spi_dat_buf[0]);
			}else{
				prc_set_src_addr(prc_dev_g,(uint32)spi_dat_buf[1]);
			}			
			prc_run(prc_dev_g);
		}
		hnum++;
	}
	
	if(irq == SPI_IRQ_FLAG_RX_TIMEOUT){
		prc_new_frame = 1;
		prc_deal_num = 0;
		sync_start++;
		flow_data_cnt++;
		spi_close(spi0_dev_g);
		spi_sensor_cfg();
		hnum = 0;
	}	
}

void ybuf_deal(uint32 *sbuf,uint32 *ybuf,uint32 sz){
	uint32 itk;
	for(itk = 0;itk < sz;itk++){
		sbuf[itk] = ybuf[itk];
	}
}

void prc_deal_irq(uint32 irq, uint32 irq_data){
	
	
	static uint8 *prc_buf;
	if(prc_deal_num == 0){
		prc_buf = psram_spi_sensor[prc_buf_num];
		prc_buf_num++;
		if(prc_buf_num == 3)
			prc_buf_num = 0;
	}
	memcpy(prc_buf+prc_deal_num*(16*240),prc_yuv_line,16*240);
	memcpy(prc_buf+320*240+prc_deal_num*(4*240),prc_yuv_line+240*16,4*240);
	memcpy(prc_buf+320*240+320*240/4+prc_deal_num*(4*240),prc_yuv_line+240*16+240*4,4*240);
	prc_deal_num++;
	if(prc_deal_num == 20){
		prc_deal_num = 0;
		display_addr = (uint32)prc_buf;	
	}

}

uint32 count_timer_get = 0;
uint32 frame_end = 0;
uint32 frame_deal = 0;



void spi_sensor_reset(void){
	uint8_t pdn;
	pdn = PIN_SPI_PDN;
	
	if(pdn != 255){
		gpio_iomap_input(pdn, GPIO_IOMAP_INPUT);    //pdn up
		os_sleep_ms(1);
		gpio_iomap_output(pdn,GPIO_IOMAP_OUTPUT);
		gpio_set_val(pdn,0);						//pdn down
		os_sleep_ms(50);
	}
}

static _Sensor_Adpt_ * sensorAutoCheck_spi(struct i2c_device *p_iic,uint8 *init_buf)
{
	uint8 i;
	_Sensor_Adpt_ * devSensor_Struct=NULL;
	for(i=0;devSensorInitTable_spi[i] != NULL;i++)
	{		
		spi_sensor_reset();
		if(sensorCheckId(p_iic,devSensorInitTable_spi[i],devSensorOPTable_spi[i])>=0)
		{
			os_printf("id =%x num:%d \n",devSensorInitTable_spi[i]->id,i);
			devSensorInit = (_Sensor_Ident_ *) devSensorInitTable_spi[i];
//#if CMOS_AUTO_LOAD
//			devSensor_Struct = sensor_adpt_load(devSensorInit->id,devSensorInit->w_cmd,devSensorInit->r_cmd,init_buf);
//#else
			devSensor_Struct = (_Sensor_Adpt_ *) devSensorOPTable_spi[i];
//#endif
			break;
		}
	}
	if(devSensor_Struct == NULL)
	{
		os_printf("Er: unkown!");
		devSensorInit = (_Sensor_Ident_ *)&null_init2;
		return NULL; // index 0 is test only
	}
	return devSensor_Struct;
}

void lcd_user_frame(uint32 frame_addr);
void spi_sensor_thread(){
	struct hgpwm_v0 *global_hgpwm;
	int data_cnt_old = 0;
	uint8 sensor_staus = 0;    //0:stop  1:run
	global_hgpwm    = (struct hgpwm_v0 *)dev_get(HG_PWM0_DEVID);
	memset(psram_spi_sensor,0x80,3*(240*320+240*320/2));
	
	prc_module_init();
	//spi_sensor_cfg();

	pwm_stop((struct pwm_device *)global_hgpwm, PWM_CHANNEL_0);
	while(1){	
		if(spi_video_run == 1){
			if(sensor_staus == 0){
				sensor_staus = 1;
				sync_start = 0;
				//if(spi_dat_buf[0] == NULL){
				//	spi_dat_buf[0] = (uint8  *)custom_malloc(480*16);
				//}
				
				//if(spi_dat_buf[1]  == NULL){
				//	spi_dat_buf[1] = (uint8  *)custom_malloc(480*16);
				//}
				spi_sensor_cfg();
				pwm_start((struct pwm_device *)global_hgpwm, PWM_CHANNEL_0);
				
			}
		}
	
		if(flow_data_cnt == data_cnt_old){
			os_sleep_ms(10);
			continue;
		}


		data_cnt_old = flow_data_cnt;

		
		if(spi_video_run == 0){
			if(sensor_staus == 1){
				sensor_staus = 0;
				pwm_stop((struct pwm_device *)global_hgpwm, PWM_CHANNEL_0);
				if(spi_dat_buf[0]){
					custom_free((void *)spi_dat_buf[0]);
					spi_dat_buf[0] = NULL;
				}

				if(spi_dat_buf[1]){
					custom_free((void *)spi_dat_buf[1]);
					spi_dat_buf[1] = NULL; 
				}				
			}
			continue;
		}
		lcd_user_frame(display_addr);		
	}
}

void spi_2_jpg();
void i2c_SetSetting(struct i2c_setting *p_i2c_setting);
void spi_senosr_open(){
	uint8 adr_num,dat_num;
	
	uint8 sen_w_cmd,sen_r_cmd,id_c = 0;	
	int8 idbuf[3];
	uint32 i=0;
	static _Sensor_Adpt_ *p_sensor_cmd = NULL;
	struct i2c_setting i2c_setting;	
	struct hgpwm_v0 *global_hgpwm;
	iic_test = (struct i2c_device *)dev_get(HG_I2C2_DEVID);	
	i2c_set_baudrate(iic_test,IIC_CLK);
	i2c_open(iic_test, IIC_MODE_MASTER, IIC_ADDR_7BIT, 0);
	i2c_ioctl(iic_test,IIC_SDA_OUTPUT_DELAY,20);	
	i2c_ioctl(iic_test,IIC_FILTERING,20);
	i2c_ioctl(iic_test,IIC_STRONG_OUTPUT,1);
	
	global_hgpwm    = (struct hgpwm_v0 *)dev_get(HG_PWM0_DEVID);
	gpio_driver_strength(PIN_PWM_CHANNEL_0, GPIO_DS_28MA);
	pwm_init((struct pwm_device *)global_hgpwm, PWM_CHANNEL_0, 5, 3);
	pwm_start((struct pwm_device *)global_hgpwm, PWM_CHANNEL_0);	
	
	os_sleep_ms(3);
	os_printf("set SPI sensor finish ,Auto Check sensor id\r\n");
	p_sensor_cmd = snser.p_sensor_cmd = sensorAutoCheck_spi(iic_test,NULL);

	if(p_sensor_cmd == NULL){
		return;
	}
	os_printf("Auto Check sensor id finish\r\n");
	i2c_setting.u8DevWriteAddr = p_sensor_cmd->w_cmd;
	i2c_setting.u8DevReadAddr = p_sensor_cmd->r_cmd;
	i2c_SetSetting(&i2c_setting);
	
	
    if(p_sensor_cmd->init!=NULL)
	{
			
			os_printf("SENSER....init\r\n");
			for(i=0;;i+=u8Addrbytnum+u8Databytnum)
			{
				if((p_sensor_cmd->init[i]==0xFF)&&(p_sensor_cmd->init[i+1]==0xFF)){
					os_printf("init table num:%d\r\n",i);
					break;
				}
	
				if((p_sensor_cmd->init[i]==0xFE)&&(p_sensor_cmd->init[i+1]==0xFE)){
					if(p_sensor_cmd->init[i+2]==0x01){
						os_sleep_ms(100);
					}
				}
				else{
					i2c_write(iic_test, (int8*)&p_sensor_cmd->init[i], u8Addrbytnum, (int8*)&p_sensor_cmd->init[i+u8Addrbytnum], u8Databytnum);
					if(i==0)
					{
						os_sleep_ms(1);
					}
				}
			}		
	
			if(p_sensor_cmd->p_xc7016_adapt.sensor_init_table != NULL)
			{
				adr_num = p_sensor_cmd->p_xc7016_adapt.addr_num;
				dat_num = p_sensor_cmd->p_xc7016_adapt.data_num;
				sen_w_cmd = u8SensorwriteID;
				sen_r_cmd = u8SensorreadID;
				u8SensorwriteID = p_sensor_cmd->p_xc7016_adapt.w_cmd;
				u8SensorreadID	= p_sensor_cmd->p_xc7016_adapt.r_cmd;
				idbuf[0] = p_sensor_cmd->p_xc7016_adapt.id_reg;		
				//id_c = i2c_sensor_read(iic_test,idbuf,adr_num,dat_num,u8SensorwriteID,u8SensorreadID);
				i2c_ioctl(iic_test,IIC_SET_DEVICE_ADDR,u8SensorwriteID>>1);
				i2c_read(iic_test,idbuf,adr_num,(int8*)&id_c,dat_num);
				os_printf("\r\nchildren SID: %x %x %x\r\n",id_c,u8SensorwriteID,u8SensorreadID);			
				for(i=0;;i+=adr_num+dat_num)
				{
					if((p_sensor_cmd->p_xc7016_adapt.sensor_init_table[i]==0xFF)&&(p_sensor_cmd->p_xc7016_adapt.sensor_init_table[i+1]==0xFF)){
						os_printf("sensor table num:%d\r\n",i);
						break;
					}

					i2c_write(iic_test, (int8*)&p_sensor_cmd->p_xc7016_adapt.sensor_init_table[i], adr_num, (int8*)&p_sensor_cmd->p_xc7016_adapt.sensor_init_table[i+adr_num], dat_num);
					if(i==0)
					{
						os_sleep_ms(1);
					}
				}	
				u8SensorwriteID = sen_w_cmd;
				u8SensorreadID	= sen_r_cmd;
				i2c_ioctl(iic_test,IIC_SET_DEVICE_ADDR,u8SensorwriteID>>1);
				for(i=0;;i+=u8Addrbytnum+u8Databytnum)
				{
					if((p_sensor_cmd->p_xc7016_adapt.bypass_off[i]==0xFF)&&(p_sensor_cmd->p_xc7016_adapt.bypass_off[i+1]==0xFF)){
						os_printf("bypass table num:%d\r\n",i);
						break;
					}
					i2c_write(iic_test, (int8*)&p_sensor_cmd->p_xc7016_adapt.bypass_off[i], u8Addrbytnum, (int8*)&p_sensor_cmd->p_xc7016_adapt.bypass_off[i+u8Addrbytnum], u8Databytnum);
					if(i==0)
					{
						os_sleep_ms(1);
					}
				}			
				
			}
			
	}

	OS_TASK_INIT("dvp_manual", &dvp_manual_handle, spi_sensor_thread, NULL, OS_TASK_PRIORITY_ABOVE_NORMAL, 1024);
	
}
#else
uint8 spi_video_run = 0;

#endif

