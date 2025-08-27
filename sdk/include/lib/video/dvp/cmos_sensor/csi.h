#ifndef _CSI_MODE_DEFINE_
#define _CSI_MODE_DEFINE_
#include "tx_platform.h"
#include "list.h"
#include "dev.h"
#include "hal/i2c.h"
#include "dev/csi/hgdvp.h"

typedef struct {
	uint32 rotate;
} Rotate_Adapt;

typedef struct {
	uint32 pclk;
	uint32 v_len;
	uint32 step_val;
	uint32 step_max;
} Hvb_Adapt;

typedef struct {
	void (*fp_rotate)(uint32 rotate);

	uint32 (*fp_hvblank)(int8_t hb,int8_t vb);

	void (*fp_exp_gain_wr)(uint32 exp,uint32 gain);
} P_Fun_Adapt;

typedef struct {
	uint8 *bypass_on;
	uint8 *bypass_off;
	//_Sensor_Ident_ * sensor_init;
	uint8 *sensor_init_table;
	uint8 id,w_cmd,r_cmd,addr_num,data_num;
	uint16 id_reg;	
} P_XC7016_Fun;


typedef struct {
	uint8 w_cmd,r_cmd,id,addr_num,data_num;
	uint16 id_reg;//id ¼Ä´æÆ÷µØÖ·
	uint8 * init;
	uint8 * preset;
	
	uint16 typ; //0: raw 1:yuv 2:mipi
	uint32 mclk;
	uint32 pclk_fir_en;
	uint16 pixelw; 
	uint16 pixelh;
	uint8 colrarray; 
	bool hsyn; 
	bool vsyn;
	bool rduline;
	bool rawwide; 
	
	/*void (*fp_rotate)(uint8 rotate);
	uint32 (*fp_hvblank)(int8_t dh,int8_t dv);
	Exp_Adapt exp_adapt;
	uint32 csi_awb_gain;*/
	Rotate_Adapt rotate_adapt;
	Hvb_Adapt hvb_adapt;
	volatile uint32 isp_all_mod;
	uint32 allow_miss_dots;
	uint32 align[1];		// only for 16byte align
	//sens_isp_adapt
	//BLC blc_adapt;
	//LC  lc_adapt;
	//DDC ddc_adapt;
	//AWB awb_adapt;
	//CCM ccm_adapt;
	//YGAMA ygama_adapt; 	
	//RGB_GAMA rgbgama_adapt;
	//EE ee_adapt;
	//CCF ccf_adapt;
	//LSC lsc_adapt;
	//CBHS cbhs_adapt;
	//MD md_adapt;
	//EXP exp_adapt;
	P_Fun_Adapt p_fun_adapt;
	P_XC7016_Fun p_xc7016_adapt;
} _Sensor_Adpt_;

typedef struct
{       
	uint8 id,w_cmd,r_cmd,addr_num,data_num;
	uint16 id_reg;
}_Sensor_Ident_;

typedef struct
{
	uint8 online;
	uint32 pclk;
	uint32 iclk;
	_Sensor_Ident_ * p_sensor_ident;
	_Sensor_Adpt_ * p_sensor_cmd;
}SNSER;

typedef struct
{
	uint16 in_w;			//sensor pixel
	uint16 in_h;
	uint16 out0_w; 			//to jpg encode
	uint16 out0_h;	
	uint16 out1_w; 			//to jpg encode
	uint16 out1_h;
}Vpp_stream;


typedef struct
{
	uint16 out_w; 
	uint16 out_h;
}Output_photo;

struct i2c_setting
{
	uint8 u8DevWriteAddr;
	uint8 u8DevReadAddr;
};



#define ALIGNED(x) 				__attribute__ ((aligned(x)))
#define SENSOR_INIT_SECTION     ALIGNED(32)
#define SENSOR_OP_SECTION       ALIGNED(32)
#define  CMOS_INIT_LEN


#define DVP                     ((struct hgdvp_hw *) DVP_BASE)


extern uint8 u8SensorwriteID,u8SensorreadID;
extern uint8 u8Addrbytnum,u8Databytnum;
Vpp_stream photo_msg;
void Delay_nopCnt(uint32 cnt);
int sensorCheckId(struct i2c_device *p_iic,const _Sensor_Ident_ *p_sensor_ident,const _Sensor_Adpt_ *p_sensor_adpt);
//void sensor_ClockInit(struct hgdvp_hw *p_dvp,uint32 mclk);
//uint32 Sensor_ReadRegister(SPI_I2C_TypeDef *p_iic,uint8 *pbdata ,uint8 u8AddrLength,uint8 u8DataLength);
//bool Sensor_WriteRegister(SPI_I2C_TypeDef *p_iic,uint8 *pbdata ,uint8 u8AddrLength,uint8 u8DataLength);
//void i2c_SendStop(SPI_I2C_TypeDef *p_iic);
//void iic_init_sensor(uint32 u32iicClk,SPI_I2C_TypeDef *p_iic);
void vpp_put_psram(uint32 yuvbuf_addr,uint32 s_w,uint32 s_h);

//////////////////////////////////////////COMMON//////////////////////////////////////////////////////////////////////

#define IMAGE_W    640     
#define IMAGE_H    480

#define IMAGE_FORMAT					0   //0:YUV
											//1:RGB565
											//2:JPEG

#define SCEN_EN                         0   

#define FRAME_RATE                      0   //0:  100%
											//1:  75%
											//2:  50%
											//3:  25%

#define ONCE_SAMPLE						0   //这个只处理整帧中断的情况,即RGB/Y ONLY

////////////////////////////////////////////////////////RGB//////////////////////////////////////////////////////////////////


#define RGB2YUV							0


////////////////////////////////////////////////////////YUV//////////////////////////////////////////////////////////////////


#define HSYS							0    //0:h->1 v->1
											 //1:h->1 v->0
											 //2:h->0 v->1
											 //3:h->0 v->0


#define YUV_MODE                        0    //0：YUYV
											 //1：YVYU
											 //2：UYVY
											 //3：VYUY


											
#define ONLY_Y							0

////////////////////////////////////////////////////////JPEG//////////////////////////////////////////////////////////////////
#define JPEG_LEN                        2*1024
#ifdef TXW81X
#define SCALE_TO_JPG                    0
#define SCALE_WIDTH_TO_JPEG				1280	
#define SCALE_HIGH_TO_JPEG				720


#define IPF_EN					0
#define DET_EN					0

#if BBM_DEMO
#define VPP_BUF1_EN				1
#else
#define VPP_BUF1_EN				0
#endif

#endif

///////////////////////////////////////////////////////sensor//////////////////////////////////////////////////////////////////////



#ifndef DEV_SENSOR_OV7725
#define DEV_SENSOR_OV7725  				(0||CMOS_AUTO_LOAD)
#endif

#ifndef DEV_SENSOR_OV7670
#define DEV_SENSOR_OV7670  				(0||CMOS_AUTO_LOAD)
#endif

#ifndef DEV_SENSOR_OV7660
#define DEV_SENSOR_OV7660  				(0||CMOS_AUTO_LOAD)
#endif

#ifndef DEV_SENSOR_GC0308
#define DEV_SENSOR_GC0308  				(0||CMOS_AUTO_LOAD)
#endif

#ifndef DEV_SENSOR_GC0309
#define DEV_SENSOR_GC0309  				(0||CMOS_AUTO_LOAD)
#endif

#ifndef DEV_SENSOR_GC0328
#define DEV_SENSOR_GC0328  				(0||CMOS_AUTO_LOAD)
#endif

#ifndef DEV_SENSOR_BF3A03
#define DEV_SENSOR_BF3A03  				(0||CMOS_AUTO_LOAD)
#endif

#ifndef DEV_SENSOR_BF3703
#define DEV_SENSOR_BF3703  				(0||CMOS_AUTO_LOAD)
#endif

#ifndef DEV_SENSOR_GC0311
#define DEV_SENSOR_GC0311  				(0||CMOS_AUTO_LOAD)
#endif

#ifndef DEV_SENSOR_GC0329
#define DEV_SENSOR_GC0329  				(0||CMOS_AUTO_LOAD)
#endif

#ifndef DEV_SENSOR_GC0312
#define DEV_SENSOR_GC0312  				(0||CMOS_AUTO_LOAD)
#endif

#ifndef DEV_SENSOR_SP0718
#define DEV_SENSOR_SP0718  				(0||CMOS_AUTO_LOAD)
#endif

#ifndef DEV_SENSOR_SP0A19
#define DEV_SENSOR_SP0A19  				(0||CMOS_AUTO_LOAD)
#endif

#ifndef DEV_SENSOR_BF3720
#define DEV_SENSOR_BF3720  				(0||CMOS_AUTO_LOAD)
#endif

#ifndef DEV_SENSOR_GC032A
#define DEV_SENSOR_GC032A  				(0||CMOS_AUTO_LOAD)
#endif

#ifndef DEV_SENSOR_OV2640
#define DEV_SENSOR_OV2640               0
#endif

#ifndef DEV_SENSOR_BF2013
#define DEV_SENSOR_BF2013               (0||CMOS_AUTO_LOAD)
#endif

#ifndef DEV_SENSOR_NT99230
#define DEV_SENSOR_NT99230               0
#endif


#ifndef DEV_SENSOR_XC7016_H63
#define DEV_SENSOR_XC7016_H63           0
#endif

#ifndef DEV_SENSOR_XC7011_H63
#define DEV_SENSOR_XC7011_H63           0
#endif

#ifndef DEV_SENSOR_XC7011_GC1054
#define DEV_SENSOR_XC7011_GC1054  				0
#endif

#ifndef DEV_SENSOR_XCG532
#define DEV_SENSOR_XCG532  				0
#endif

#ifndef DEV_SENSOR_GC2145
#define DEV_SENSOR_GC2145           0
#endif

#ifndef DEV_SENSOR_BF30A2
#define DEV_SENSOR_BF30A2           (0||CMOS_AUTO_LOAD)
#endif


#ifndef DEV_SENSOR_IOT101
#define DEV_SENSOR_IOT101           0
#endif


#if DEV_SENSOR_OV7725
extern const _Sensor_Ident_ ov7725_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ ov7725_cmd;
#endif


#if DEV_SENSOR_OV7670
extern const _Sensor_Ident_ ov7670_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ ov7670_cmd;
#endif

#if DEV_SENSOR_OV7660
extern const _Sensor_Ident_ ov7660_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ ov7660_cmd;
#endif

#if DEV_SENSOR_GC0308
extern const _Sensor_Ident_ gc0308_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc0308_cmd;
#endif

#if DEV_SENSOR_GC0309
extern const _Sensor_Ident_ gc0309_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc0309_cmd;
#endif

#if DEV_SENSOR_GC0328
extern const _Sensor_Ident_ gc0328_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc0328_cmd;
#endif

#if DEV_SENSOR_GC0311
extern const _Sensor_Ident_ gc0311_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc0311_cmd;
#endif

#if DEV_SENSOR_GC0329
extern const _Sensor_Ident_ gc0329_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc0329_cmd;
#endif

#if DEV_SENSOR_GC0312
extern const _Sensor_Ident_ gc0312_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc0312_cmd;
#endif


#if DEV_SENSOR_BF3A03
extern const _Sensor_Ident_ bf3a03_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ bf3a03_cmd;
#endif

#if DEV_SENSOR_BF3703
extern const _Sensor_Ident_ bf3703_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ bf3703_cmd;
#endif

#if DEV_SENSOR_BF2013
extern const _Sensor_Ident_ bf2013_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ bf2013_cmd;
#endif

#if DEV_SENSOR_OV2640
extern const _Sensor_Ident_ ov2640_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ ov2640_cmd;
#endif

#if DEV_SENSOR_XC7016_H63
extern const _Sensor_Ident_ xc7016_h63_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ xc7016_h63_cmd;
#endif

#if DEV_SENSOR_XC7011_H63
extern const _Sensor_Ident_ xc7011_h63_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ xc7011_h63_cmd;
#endif

#if DEV_SENSOR_XC7011_GC1054
extern const _Sensor_Ident_ xc7011_gc1054_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ xc7011_gc1054_cmd;
#endif

#if DEV_SENSOR_XCG532
extern const _Sensor_Ident_ xcg532_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ xcg532_cmd;
#endif

#if DEV_SENSOR_GC2145
extern const _Sensor_Ident_ gc2145_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc2145_cmd;
#endif

#if DEV_SENSOR_SP0718
extern const _Sensor_Ident_ sp0718_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ sp0718_cmd;
#endif

#if DEV_SENSOR_SP0A19
extern const _Sensor_Ident_ sp0a19_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ sp0a19_cmd;
#endif

#if DEV_SENSOR_BF3720
extern const _Sensor_Ident_ bf3720_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ bf3720_cmd;
#endif

#if DEV_SENSOR_GC032A
extern const _Sensor_Ident_ gc032a_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ gc032a_cmd;
#endif

#if DEV_SENSOR_BF30A2
extern const _Sensor_Ident_ bf30a2_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ bf30a2_cmd;
#endif

#if DEV_SENSOR_NT99230
extern const _Sensor_Ident_ nt99230_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ nt99230_cmd;
#endif

#if DEV_SENSOR_IOT101
extern const _Sensor_Ident_ sc101iot_init;
extern SENSOR_OP_SECTION const _Sensor_Adpt_ sc101iot_cmd;
#endif

#endif
