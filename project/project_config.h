#ifndef __SDK_PROJECT_CONFIG_H__
#define __SDK_PROJECT_CONFIG_H__

//是否使能需要psram的空间
//#define PSRAM_HEAP
//#define CUSTOM_PSRAM_SIZE               (2000*1024)
#define CUSTOM_SIZE                     (60*1024) 
#define DEFAULT_SYS_CLK   				(240*1000000) 


/*************************************************************************** */
//VCAM是否能,部分io需要打开才能有输出,典型的是:DVP需要打开才能有输出
#define VCAM_EN                         1
/***************************************************************************** */ 


/**************************************按键配置******************************************************** */
#define KEY_MODULE_EN					0
/*************************************************************************************************** */


/**********************************sd卡配置*********************************************************** */
#define SDH_EN                          0
#define FS_EN                           0
/*************************************************************************************************** */

/***************************视频相关宏要打开********************************************************** */
#define DVP_EN                          1               //dvp镜头使能
#define VPP_EN                          1               //硬件vpp使能
#define JPG_EN                          1               //jpg硬件编码使能
#define SCALE_EN						1               //scale使能
#define PRC_EN                          1
/*************************************************************************************************** */

/**********************************************音频相关********************************************* */
#define AUDIO_EN                        1
#define AUDIO_DAC_EN                    0
/************************************************************************************************** */



/************************************速率调整参数选择 ************************************************/
#define RATE_CONTROL_ERSHAO         1
#define RATE_CONTROL_HANGPAI        2
#define RATE_CONTROL_IPC            3
#define RATE_CONTROL_BABYMPNITOR    4

#define RATE_CONTROL_SELECT     RATE_CONTROL_IPC
/************************************************************************************************** */


/*****************************************wifi相关参数配置*********************************************************** */
#define WIFI_RF_PWR_LEVEL               0           //选择WIFI功率。0：普通功率；1：比0的功率小；2：比1更小；10：大电流
#define WIFI_RTS_THRESHOLD              1600        //RTS阈值，2304等效于不用RTS
#define WIFI_RTS_MAX_RETRY              2           //RTS重试次数，范围为2~16
#define WIFI_TX_MAX_RETRY               7           //最大传输次数，范围为1~31
#define WIFI_TX_MAX_POWER               7           //TX最大发射功率，有0~7档
/* 每1bit代表一种速率。每bit代表的格式：
 * bit 0  : DSSS 1M
 * bit 1  : DSSS 2M
 * bit 2  : DSSS 5.5M
 * bit 3  : DSSS 11M
 * bit 4  : NON-HT 6M
 * bit 5  : NON-HT 9M
 * bit 6  : NON-HT 12M
 * bit 7  : NON-HT 18M
 * bit 8  : NON-HT 24M
 * bit 9  : NON-HT 36M
 * bit 10 : NON-HT 48M
 * bit 11 : NON-HT 54M
 * bit 12 : HT MCS0
 * bit 13 : HT MCS1
 * bit 14 : HT MCS2
 * bit 15 : HT MCS3
 * bit 16 : HT MCS4
 * bit 17 : HT MCS5
 * bit 18 : HT MCS6
 * bit 19 : HT MCS7
 */
#define WIFI_TX_SUPP_RATE               0x0FFFFF    //TX速率支持，每1bit对应一种速率
#define WIFI_MAX_STA_CNT                8           //最多连接sta的数量。有效值为1~8
#define WIFI_MULICAST_RETRY             0           //组播帧传输次数
#define WIFI_ACS_CHAN_LISTS             0x03FF      //要扫描的信道。每1bit对应1个信道(bit 0~11 -> chan 1~12)
#define WIFI_ACS_SCAN_TIME              150         //每个信道的扫描时间，单位ms
#define WIFI_MAX_PS_CNT                 30           //底层为休眠sta缓存的帧最大数量。0代表sta休眠由umac全程管理，底层不缓存
#define CHANNEL_DEFAULT                 0
#define SSID_DEFAULT                    "HG-WIFI_"
#define WIFI_TX_DUTY_CYCLE              80         //tx发送占空比，单位是%，范围是0~100
#define WIFI_SSID_FILTER_EN             0           //是否使能SSID过滤功能。使能后，只有隐藏SSID和指定SSID的beacon才会上传
#define WIFI_PREVENT_PS_MODE_EN         1           //是否尽可能的阻止sta进入休眠

//设置AP的ip地址以及ip池
#define IP_ADDR_DEFAULT                 0x0104A8C0  //192.168.4.1
#define NET_MASK_DEFAULT                0x00FFFFFF  //255.255.255.0
#define GW_IP_DEFAULT                   0x0104A8C0  //192.168.4.1
#define DHCP_START_IP_DEFAULT           0x6404A8C0  //192.168.4.100
#define DHCP_END_IP_DEFAULT             0xFE04A8C0  //192.168.4.254
/************************************************************************************************************************************ */





/***********************************************IO配置,需要用到的IO配置需要在这里配置 **************************************/

//串口配置
#define PIN_UART0_TX PC_7
#define PIN_UART0_RX PC_6

//AT命令使用哪个串口
#define ATCMD_UARTDEV       HG_UART0_DEVID


//默认sdk匹配学习板,使用PC_2和PC_3作为dvp的i2c
#define PIN_IIC2_SCL PC_2
#define PIN_IIC2_SDA PC_3


//DVP的power down和reset引脚
#define PIN_DVP_RESET  255
#define PIN_DVP_PDN    255

//sd卡的io
#define PIN_SDH_CLK		  PC_4//PC_2//
#define PIN_SDH_CMD		  PC_3//PC_3//
#define PIN_SDH_DAT0	  PC_2//PC_4//
#define PIN_SDH_DAT1	  255
#define PIN_SDH_DAT2	  255
#define PIN_SDH_DAT3	  255


//LCD的io
#define PIN_LCD_RESET  255//255//

#define DOTCLK_RWR PA_0
#define DE_ERD PA_2
#define VS_CS  PC_5
#define HS_DC  PA_11
#define LCD_TE 255


#define LCD_D0 PC_8
#define LCD_D1 PC_9
#define LCD_D2 PC_10
#define LCD_D3 PC_11
#define LCD_D4 PC_12
#define LCD_D5 PC_13
#define LCD_D6 PC_14
#define LCD_D7 PC_15
#define LCD_D8 255
#define LCD_D9 255
#define LCD_D10 255
#define LCD_D11 255
#define LCD_D12 255
#define LCD_D13 255
#define LCD_D14 255
#define LCD_D15 255
#define LCD_D16 255
#define LCD_D17 255
#define LCD_D18 255
#define LCD_D19 255
#define LCD_D20 255
#define LCD_D21 255
#define LCD_D22 255
#define LCD_D23 255


#define PIN_SPI_PDN 255


/*********************************************************************************************************************** */




/**************************************DVP摄像头型号配置***************************************************************** */
//如果sdk没有对应型号,需要自己去添加
#define CMOS_AUTO_LOAD           	1
#define DEV_SENSOR_GC0308           1
#define DEV_SENSOR_GC0309           1
#define DEV_SENSOR_GC0311           1
#define DEV_SENSOR_OV2640           1
#define DEV_SENSOR_GC0328           1
#define DEV_SENSOR_GC0329           1
#define DEV_SENSOR_SP0A19           1
#define DEV_SENSOR_SP0718           1
#define DEV_SENSOR_BF3A03           1
#define DEV_SENSOR_BF30A2           1
#define DEV_SENSOR_BF2013           1
#define DEV_SENSOR_BF3703           1
#define DEV_SENSOR_BF3720           1
#define DEV_SENSOR_GC0312           1
#define DEV_SENSOR_OV7660           1
#define DEV_SENSOR_OV7670           1
#define DEV_SENSOR_OV7725           1
#define DEV_SENSOR_XC7011_GC1054    1
#define DEV_SENSOR_XC7011_H63       1
#define DEV_SENSOR_XC7016_H63       1
#define DEV_SENSOR_XCG532           1
#define DEV_SENSOR_HI704            1
#define DEV_SENSOR_SP0828           1
#define DEV_SENSOR_SP0A20           1
#define DEV_SENSOR_PAS6175          1

/*********************************************************************************************************************** */



/**********************************************MCU屏驱动配置************************************************************* */
#define LCD_EN			 				0
#define LVGL_STREAM_ENABLE              0
#define LCD_ST7789V_MCU565_EN           0


/************************************************************************************************************************* */





/*******************************************jpeg 空间分配,需要根据自己内存去配置,带psram和不带psram配置不一致************************************** */

//jpg0
#define JPG0_HEAD_RESERVER  0                   //每段JPEG BUF前面预留用于填充其他数据的数据量，无需预留填0
#define JPG0_BUF_LEN        1308                //8192//1308//2880//4096+24   // 1*1024// 1*1024//       JPG buf长度+reserver长度          1308
#define JPG0_NODE           40                  //20//18//30*2
#define JPG0_TAIL_RESERVER  0


//jpg1
#define JPG1_HEAD_RESERVER  0                   //每段JPEG BUF前面预留用于填充其他数据的数据量，无需预留填0
#define JPG1_BUF_LEN        1308                //2880//4096+24   // 1*1024// 1*1024//       JPG buf长度+reserver长度          1308
#define JPG1_NODE           40                  //18//30*2
#define JPG1_TAIL_RESERVER  0

#endif
