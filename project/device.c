#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/timer.h"
#include "osal/task.h"


#include "hal/uart.h"
#include "hal/i2c.h"
#include "hal/timer_device.h"
#include "hal/pwm.h"
#include "hal/capture.h"
#include "hal/dma.h"
#include "hal/netdev.h"
#include "hal/spi.h"
#include "hal/spi_nor.h"
#include "hal/auadc.h"
#include "hal/audac.h"
#include "lib/ota/fw.h"
#include "lib/syscfg/syscfg.h"
#include "lib/net/ethphy/eth_mdio_bus.h"
#include "lib/net/ethphy/eth_phy.h"
#include "lib/net/ethphy/phy/ip101g.h"


#include "dev/uart/hguart_v2.h"
#include "dev/uart/hguart_v4.h"
#include "dev/sdio/hgsdio20_slave.h"
#include "dev/dma/dw_dmac.h"
#include "dev/gpio/hggpio_v4.h"
#include "dev/dma/hg_m2m_dma.h"
#include "dev/usb/hgusb20_v1_dev_api.h"
#include "dev/emac/hg_gmac_eva_v2.h"
#include "dev/crc/hg_crc.h"
#include "dev/timer/hgtimer_v4.h"
#include "dev/timer/hgtimer_v5.h"
#include "dev/timer/hgtimer_v7.h"
#include "dev/pwm/hgpwm_v0.h"
#include "dev/capture/hgcapture_v0.h"
#include "dev/spi/hgspi_v3.h"
#include "dev/spi/hgspi_xip.h"
#include "dev/adc/hgadc_v0.h"
#include "dev/i2c/hgi2c_v1.h"
#include "dev/sysaes/hg_sysaes_v3.h"
#include "dev/sha/hgsha_v0.h"
#include "dev/i2s/hgi2s_v0.h"
#include "dev/pdm/hgpdm_v0.h"
#include "dev/led/hgled_v0.h"
#include "dev/csi/hgdvp.h"
#include "dev/jpg/hgjpg.h"
#include "dev/tk/hg_touchkey_v2.h"
#include "dev/vpp/hgvpp.h"
#include "dev/prc/hgprc.h"
#include "dev/of/hgof.h"
#include "dev/lcdc/hglcdc.h"
#include "dev/scale/hgscale.h"
#include "dev/xspi/hg_xspi.h"
#include "dev/xspi/hg_xspi_psram.h"
#include "dev/audio/hg_audio_v0.h"
#include "lib/sdhost/sdhost.h"
//#include "drv_usart.h"
#include "device.h"
#include "syscfg.h"

extern const struct hgwphy_ah_cfg nphyahcfg;
extern union _dpd_ram dpd_ram;
extern const uint32 rx_imb_iq_normal[6];
extern const union hgdbgpath_cfg ndbgpathcfg;
extern const union hgrfmipi_cfg nrfmipicfg;
extern struct dma_device *m2mdma;

struct hgusb20_os_dev usb20_os_dev;
struct hgusb20_dev usb20_dev = {
    .usb_hw = (void *)HG_USB20_DEVICE_BASE,
	.usb_os_msg = &usb20_os_dev,
    .ep_irq_num = USB20MC_IRQn,
    .dma_irq_num = USB20DMA_IRQn,
    //.flags = BIT(HGUSB20_FLAGS_FULL_SPEED),
};

struct hgsdio20_slave sdioslave = {
    .hw          = (void *)HG_SDIO20_SLAVE_BASE,
    .irq_num     = SDIO_IRQn,
    .rst_irq_num = SDIO_RST_IRQn,
    //.block_size = 64,
};

struct hg_crc crc32_module = {
    .hw = (void *)HG_CRC32_BASE,
    .irq_num = CRC_IRQn,
};

struct hg_sysaes_v3 sysaes = {
    .hw = (void *)HG_SYSAES_BASE,
    .irq_num = SPACC_PKA_IRQn,
};

struct hgsha_v0 sha = {
    .hw = (void *)HG_SHA_BASE,
    .irq_hw_num = SPACC_PKA_IRQn,
    .irq_num = SHA_VIRTUAL_IRQn,
};

struct hguart_v2 uart0 = {
    .hw      = HG_UART0_BASE,
    .irq_num = UART0_IRQn
};
struct hguart_v2 uart1 = {
    .hw      = HG_UART1_BASE,
    .irq_num = UART1_IRQn,
};

struct hguart_v4 uart4 = {
    .hw           = HG_UART4_BASE,
    .irq_num      = UART4_IRQn,
    .comm_irq_num = UART45_IRQn,
};

struct hguart_v4 uart5 = {
    .hw           = HG_UART5_BASE,
    .irq_num      = UART5_IRQn,
    .comm_irq_num = UART45_IRQn,
};

struct hggpio_v4 gpioa = {
    .hw           = HG_GPIOA_BASE,
    .comm_irq_num = GPIOABCE_IRQn,
    .irq_num      = GPIOA_IRQn,
    .pin_num      = {PA_0, PA_15},
};

struct hggpio_v4 gpiob = {
    .hw           = HG_GPIOB_BASE,
    .comm_irq_num = GPIOABCE_IRQn,
    .irq_num      = GPIOB_IRQn,
    .pin_num      = {PB_0, PB_15},
};

struct hggpio_v4 gpioc = {
    .hw           = HG_GPIOC_BASE,
    .comm_irq_num = GPIOABCE_IRQn,
    .irq_num      = GPIOC_IRQn,
    .pin_num = {PC_0, PC_15},
};

struct hggpio_v4 gpioe = {
    .hw           = HG_GPIOE_BASE,
    .comm_irq_num = GPIOABCE_IRQn,
    .irq_num      = GPIOE_IRQn,
    .pin_num      = {PE_0, PE_15},
};


struct mem_dma_dev mem_dma = {
    .hw      = (void *)M2M_DMA_BASE,
    .irq_num = M2M0_IRQn,	
};

struct hg_gmac_eva_v2 gmac = {
    .hw            = HG_GMAC_BASE,
    .irq_num       = GMAC_IRQn,
    .tx_buf_size   = 4 * 1024,
    .rx_buf_size   = 8 * 1024,
    .modbus_devid  = HG_ETH_MDIOBUS0_DEVID,
    .phy_devid     = HG_ETHPHY0_DEVID,
    .mdio_pin      = PIN_GMAC_RMII_MDIO,
    .mdc_pin       = PIN_GMAC_RMII_MDC,
};

struct hgtimer_v4 timer0 = {
    .hw      = TIMER0_BASE,
    .irq_num = TIM0_IRQn,
};

struct hgtimer_v4 timer1 = {
    .hw      = TIMER1_BASE,
    .irq_num = TIM1_IRQn,
};

struct hgtimer_v4 timer2 = {
    .hw      = TIMER2_BASE,
    .irq_num = TIM2_IRQn,
};

struct hgtimer_v4 timer3 = {
	.hw 	 = TIMER3_BASE,
	.irq_num = TIM3_IRQn,
};

struct hgtimer_v7 simtimer0 = {
    .hw      = SIMPLE_TIMER0_BASE,
    .irq_num = STMR012345_IRQn,
};

struct hgtimer_v5 led_timer0 = {
    .hw      = LED_TIMER0_BASE,
    .irq_num = LED_TMR_IRQn,
};


struct hgpwm_v0 pwm = {
    .channel[0] = (void *) &timer0,
    .channel[1] = (void *) &timer1,
    .channel[2] = (void *) &timer2,
    .channel[3] = (void *) &timer3,
    .channel[4] = (void *) &simtimer0,
};

struct hgcapture_v0 capture = {
    .channel[0] = (void *) &timer0,
    .channel[1] = (void *) &timer1,
};

struct hgadc_v0 adc = {
    .hw      = ADKEY_BASE,
    .irq_num = ADKEY01_IRQn,
};


struct hgcqspi cqspi = {
    .hw = (void *)QSPI_BASE,
    .irq_num = QSPI_IRQn,
    .opened = 0,
};

struct hg_xspi xspi = {
    .hw = (void *)HG_OSPI_BASE,
    .irq_num = OSPI_IRQn,
    .opened = 0,
};


struct hgspi_v3 spi0 = {
    .hw      = SPI0_BASE,
    .irq_num = SPI0_IRQn,
};

struct hgspi_v3 spi1 = {
    .hw      = SPI1_BASE,
    .irq_num = SPI1_IRQn,
};

struct hgspi_v3 spi2 = {
    .hw      = SPI2_BASE,
    .irq_num = SPI2_IRQn,
};


struct hgspi_xip spi7 = {
    .hw      = QSPI_BASE,
};

struct hgi2c_v1 iic1 = {
    .hw      = IIC1_BASE,
    .irq_num = SPI1_IRQn,
};

struct hgi2c_v1 iic2 = {
    .hw      = IIC2_BASE,
    .irq_num = SPI2_IRQn,
};

//struct hgi2c iic3 = {
//    .hw      = IIC3_BASE,
//    .irq_num = SPI3_IRQn,
//};


struct hgdvp dvp = {
    .hw = DVP_BASE,
    .irq_num = DVP_IRQn,
};

struct hgjpg jpg0 = {
    .hw = JPG0_BASE,
#ifdef FPGA_SUPPORT
    .thw = MJPEG0_TAB_BASE,
#else
    .thw = MJPEG0_TAB_BASE,
#endif
    .irq_num = MJPEG01_IRQn,
};

struct hgjpg jpg1 = {
    .hw = JPG1_BASE,
#ifdef FPGA_SUPPORT
    .thw = MJPEG1_TAB_BASE,
#else
    .thw = MJPEG1_TAB_BASE,
#endif
    .irq_num = MJPEG01_IRQn,
};

struct hgvpp vpp = {
    .hw      = VPP_BASE,
    .irq_num = VPP_IRQn,
};

struct hgprc prc = {
    .hw      = PRC_BASE,
    .irq_num = PRC_IRQn,
};

struct hgscale scale1 = {
    .hw = SCALE1_BASE,
    .irq_num = SCALE1_IRQn,
};

struct hgscale scale2 = {
    .hw = SCALE2_BASE,
    .irq_num = SCALE2_IRQn,
};

struct hgscale scale3 = {
    .hw = SCALE3_BASE,
    .irq_num = SCALE3_IRQn,
};

struct hglcdc lcdc = {
    .hw = LCDC_BASE,
    .irq_num = LCD_IRQn,
};

struct hgof of = {
    .hw = OF_BASE,
};

struct hgsdh sdh = {
    .hw = SDHOST_BASE,
    .irq_num = SDHOST_IRQn,
};

struct hgi2s_v0 i2s0 = {
    .hw      = HG_IIS0_BASE,
    .irq_num = IIS0_IRQn,
};

struct hgi2s_v0 i2s1 = {
    .hw      = HG_IIS1_BASE,
    .irq_num = IIS1_IRQn,
};

struct hgpdm_v0 pdm = {
    .hw      = HG_PDM_BASE,
    .irq_num = PDM_IRQn,
};

struct ethernet_mdio_bus mdio_bus0;
//
struct ethernet_phy_device ethernet_phy0 = {
    .addr = 0x01,
    .drv  = &ip101g_driver,
};

struct hg_audio_v0 auadc = {
    .hw       = AUDIO_BASE,
    .irq_num  = AUDIO_SUBSYS1_IRQn,
    .p_comm   = (void *)&auadc.comm_dat,
    .dev_type = AUDIO_TYPE_AUADC,
};


struct hg_audio_v0 audac = {
    .hw       = AUDIO_BASE,
    .irq_num  = AUDIO_SUBSYS2_IRQn,
    .p_comm   = (void *)&auadc.comm_dat,
    .dev_type = AUDIO_TYPE_AUDAC,
};

struct hg_audio_v0 aufade = {
    .hw = AUDIO_BASE,
    .irq_num = 0,
    .p_comm  = (void *)&auadc.comm_dat,
    .dev_type = AUDIO_TYPE_AUFADE,
};

struct hg_audio_v0 aualaw = {
    .hw       = AUDIO_BASE,
    .irq_num  = AUALAW_IRQn,
    .p_comm   = (void *)&auadc.comm_dat,
    .dev_type = AUDIO_TYPE_AUALAW,
};

struct hg_audio_v0 auvad = {
   .hw       = AUDIO_BASE,
   .irq_num  = AUVAD_IRQn,
   .p_comm   = (void *)&auadc.comm_dat,
   .dev_type = AUDIO_TYPE_AUVAD,
};

struct hg_audio_v0 aueq = {
   .p_comm   = (void *)&auadc.comm_dat,
   .dev_type = AUDIO_TYPE_AUEQ,
};

#ifdef PSRAM_EN
struct spi_nor_flash flash0 = {
    .spidev      = (struct spi_device *)&spi0,
    .spi_config  = {12000000, SPI_CLK_MODE_0, SPI_WIRE_NORMAL_MODE, 0},
    .vendor_id   = 0,
    .product_id  = 0,
    .size        = 0x200000, /* Special External-Flash Size */
    .block_size  = 0x10000,
    .sector_size = 0x1000,
    .page_size   = 256,
    .mode        = SPI_NOR_NORMAL_SPI_MODE,
};
#else
struct spi_nor_flash flash0 = {
    .spidev      = (struct spi_device *)&spi7,
    .spi_config  = {12000000, SPI_CLK_MODE_0, SPI_WIRE_NORMAL_MODE, 0},
    .vendor_id   = 0,
    .product_id  = 0,
    .size        = 0x200000, /* Special External-Flash Size */
    .block_size  = 0x10000,
    .sector_size = 0x1000,
    .page_size   = 4096,
    .mode        = SPI_NOR_XIP_MODE,
};
#endif
extern uint32_t get_flash_cap();
void device_init(void)
{
    extern void *console_handle;
    uint32_t flash_size = get_flash_cap();
    if (flash_size > 0) {
        flash0.size = flash_size;
    }

    hggpio_v4_attach(HG_GPIOA_DEVID, &gpioa);
    hggpio_v4_attach(HG_GPIOB_DEVID, &gpiob);
    hggpio_v4_attach(HG_GPIOC_DEVID, &gpioc);
    hggpio_v4_attach(HG_GPIOE_DEVID, &gpioe);

    hgi2s_v0_attach(HG_IIS0_DEVID, &i2s0);
    hgi2s_v0_attach(HG_IIS1_DEVID, &i2s1);
    //hguart_v3_attach(HG_UART4_DEVID, &uart4);
    //hguart_v3_attach(HG_UART5_DEVID, &uart5);
    //hgspi_v2_attach(HG_SPI5_DEVID, &spi5);
    //hgspi_v2_attach(HG_SPI6_DEVID, &spi6);
    hgpdm_v0_attach(HG_PDM0_DEVID, &pdm);
    //hgled_v0_attach(HG_LED0_DEVID, &led);
    //hgtimer_v4_attach(HG_TIMER0_DEVID, &timer0);
    //hgtimer_v4_attach(HG_TIMER1_DEVID, &timer1);
    //hgtimer_v5_attach(HG_LED_TIMER0_DEVID, &led_timer0);
    //hgtimer_v6_attach(HG_SUPTMR0_DEVID, &super_timer0);
    hgpwm_v0_attach(HG_PWM0_DEVID, &pwm);
    //hgcapture_v0_attach(HG_CAPTURE0_DEVID, &capture);
    //hgadc_v0_attach(HG_ADC1_DEVID, &adc1);
    hgadc_v0_attach(HG_ADC0_DEVID, &adc);

    hguart_v2_attach(HG_UART0_DEVID, &uart0);
    hguart_v2_attach(HG_UART1_DEVID, &uart1);

    hguart_v4_attach(HG_UART4_DEVID, &uart4);
#ifdef MACBUS_SDIO
    //hgsdio20_slave_attach(HG_SDIOSLAVE_DEVID, &sdioslave);
#endif
    hg_m2m_dma_dev_attach(HG_M2MDMA_DEVID, &mem_dma);
    m2mdma = (struct dma_device *)&mem_dma;

#ifdef ROM_FUNC_ENABLE
    /* excute again because mrom function not define func dev_register*/
    dev_register(HG_WPHY_DEVID, (struct dev_obj *)&wphy);
#endif

#if USB_EN
#if USB_HOST_EN
    hgusb20_host_attach(HG_USBDEV_DEVID, &usb20_dev);
#else
    hgusb20_dev_attach(HG_USBDEV_DEVID, &usb20_dev);
#endif
#endif

    //hguart_attach(HG_UART0_DEVID, &uart0);
#if DVP_EN
    hgdvp_attach(HG_DVP_DEVID, &dvp);
#endif

#if JPG_EN
    hgjpg_attach(HG_JPG0_DEVID, &jpg0);
    hgjpg_attach(HG_JPG1_DEVID, &jpg1);
#endif

#if PRC_EN
    hgprc_attach(HG_PRC_DEVID, &prc);
#endif

#if OF_EN
    hgof_attach(HG_OF_DEVID, &of);
#endif

#if SCALE_EN
    hgscale1_attach(HG_SCALE1_DEVID, &scale1);
    hgscale2_attach(HG_SCALE2_DEVID, &scale2);
    hgscale3_attach(HG_SCALE3_DEVID, &scale3);
#endif

#if LCD_EN
    hglcdc_attach(HG_LCDC_DEVID,&lcdc);
#endif

#if VPP_EN
    hgvpp_attach(HG_VPP_DEVID, &vpp);
#endif

#if SDH_EN
    hgsdh_attach(HG_SDIOHOST_DEVID, &sdh);
#endif

    hgpwm_v0_attach(HG_PWM0_DEVID, &pwm);
   // hgtimer_v4_attach(HG_TIMER0_DEVID, &timer0);
#if PWM_EN
    hgtimer_v4_attach(HG_TIMER0_DEVID, &timer0);
    hgtimer_v4_attach(HG_TIMER1_DEVID, &timer1);
    hgtimer_v4_attach(HG_TIMER2_DEVID, &timer2);
    hgtimer_v4_attach(HG_TIMER3_DEVID, &timer3);
    hgtimer_v7_attach(HG_SIMTMR0_DEVID, &simtimer0);
    hgpwm_v0_attach(HG_PWM0_DEVID, &pwm);
    hgcapture_v0_attach(HG_CAPTURE0_DEVID, &capture);
#endif
//    hg_gmac_v2_attach(HG_ETH_GMAC_DEVID, &gmac);


    hg_crc_attach(HG_CRC_DEVID, &crc32_module);
    hg_sysaes_v3_attach(HG_HWAES0_DEVID, &sysaes);
    hgsha_v0_attach(HG_SHA_DEVID, &sha);

//    hgtimer_attach(HG_TIMER0_DEVID, &timer0);
//    hgtimer_attach(HG_TIMER1_DEVID, &timer1);
//    hgtimer_attach(HG_TIMER2_DEVID, &timer2);
//    hgtimer_attach(HG_TIMER3_DEVID, &timer3);
//    hgtimer_attach(HG_TIMER4_DEVID, &timer4);
//    hgtimer_attach(HG_TIMER5_DEVID, &timer5);
//    hgtimer_attach(HG_TIMER6_DEVID, &timer6);
//    hgtimer_attach(HG_TIMER7_DEVID, &timer7);

    hgspi_v3_attach(HG_SPI0_DEVID, &spi0);
    //hgspi_v3_attach(HG_SPI1_DEVID, &spi1);
    hgspi_xip_attach(HG_SPI7_DEVID, &spi7);
//    hgspi_v1_attach(SPI3_DEVID, &spi3);
    spi_nor_attach(&flash0, HG_FLASH0_DEVID);

//    hgi2c_v1_attach(HG_I2C0_DEVID, &iic0);
    hgspi_v3_attach(HG_SPI1_DEVID, &spi1);
    hgi2c_v1_attach(HG_I2C2_DEVID, &iic2);
//    hgi2c_attach(HG_I2C3_DEVID, &iic3);

    //hgcqspi_attach(HG_QSPI_DEVID, &cqspi);

    eth_mdio_bus_attach(HG_ETH_MDIOBUS0_DEVID, &mdio_bus0);
    eth_phy_attach(HG_ETHPHY0_DEVID, &ethernet_phy0);


    hg_audio_v0_attach(HG_AUADC_DEVID, &auadc);
    hg_audio_v0_attach(HG_AUDAC_DEVID, &audac);
    hg_audio_v0_attach(HG_AUVAD_DEVID, &auvad);
    hg_audio_v0_attach(HG_AUALAW_DEVID, &aualaw);
    hg_audio_v0_attach(HG_AUEQ_DEVID, &aueq);
	hg_audio_v0_attach(HG_AUFADE_DEVID, &aufade);

    uart_open((struct uart_device *)&uart0, 921600);
    console_handle = (void*)&uart0;
    //hg_gmac_v2_attach(HG_GMAC_DEVID, &gmac);

#if UART_FLY_CTRL_EN
    uart_open((struct uart_device *)&uart0, 115200);
#endif

#ifdef CONFIG_SLEEP
        hg_xspi_attach(HG_XSPI_DEVID, &xspi);
#endif

#ifdef LMAC_BGN_PCF
        if(dev_get(HG_TIMER3_DEVID)){
            os_printf("PCF_error: timer3 used\r\n");
        }else{
            hgtimer_v4_attach(HG_TIMER3_DEVID, &timer3);
        }
#endif

#ifdef LMAC_BGN_RAW
            if(dev_get(HG_TIMER3_DEVID)){
                os_printf("RAW_error: timer3 used\r\n");
            }else{
                hgtimer_v4_attach(HG_TIMER3_DEVID, &timer3);
            }
#endif

	extern void device_burst_set(void);
	device_burst_set();

}

void device_burst_set(void)
{
	//ll_sysctrl_dma2ahb_m2m1_wr_lmac_wave_wr_priority_switch(1);
	ll_sysctrl_dma2ahb_osd_enc_rd_lmac_wave_rd_priority_switch(1);
	ll_sysctrl_dma2ahb_pri_switch(0xE00);
	
	ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_LMAC_WAVE_RD, DMA2AHB_BURST_SIZE_32);
	ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_LMAC_WAVE_WR, DMA2AHB_BURST_SIZE_32);
	
	//ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_MJPEG1_SCALE2_YUV_WR, DMA2AHB_BURST_SIZE_64);
	//ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_MJPEG1_RD, DMA2AHB_BURST_SIZE_64);
	ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_M2M0_RD, DMA2AHB_BURST_SIZE_32);
	ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_M2M0_WR, DMA2AHB_BURST_SIZE_32);
	ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_M2M1_RD, DMA2AHB_BURST_SIZE_32);
	ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_M2M1_WR, DMA2AHB_BURST_SIZE_32);
	ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_SCALE3_Y_WR, DMA2AHB_BURST_SIZE_32);
	ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_SCALE3_U_WR, DMA2AHB_BURST_SIZE_32);
	ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_SCALE3_V_WR, DMA2AHB_BURST_SIZE_32); 
	ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_LCD_ROTATE_RD, DMA2AHB_BURST_SIZE_16);
	ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_OSD_ENC_RD, DMA2AHB_BURST_SIZE_32);
	ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_OSD_ENC_WR, DMA2AHB_BURST_SIZE_32);
	ll_sysctrl_dma2ahb_burst_set(DMA2AHB_BURST_CH_OSD0_RD, DMA2AHB_BURST_SIZE_32);
	ll_sysctrl_dma2ahb_select(DMA2AHB_BURST_OBJ_OSPI);
}

struct gpio_device *gpio_get(uint32 pin)
{
    if (pin >= gpioa.pin_num[0] && pin <= gpioa.pin_num[1]) {
        return (struct gpio_device *)&gpioa;
    } else if (pin >= gpiob.pin_num[0] && pin <= gpiob.pin_num[1]) {
        return (struct gpio_device *)&gpiob;
    } else if (pin >= gpioc.pin_num[0] && pin <= gpioc.pin_num[1]) {
        return (struct gpio_device *)&gpioc;
    } else if (pin >= gpioe.pin_num[0] && pin <= gpioe.pin_num[1]) {
        return (struct gpio_device *)&gpioe;
    }
    return NULL;
}

int32 syscfg_info_get(struct syscfg_info *pinfo)
{
#if SYSCFG_ENABLE
    pinfo->flash1 = &flash0;
    pinfo->flash2 = &flash0;
    pinfo->size  = pinfo->flash1->sector_size;
    pinfo->addr1 = pinfo->flash1->size - (2 * pinfo->size);
    pinfo->addr2 = pinfo->flash1->size - pinfo->size;
    pinfo->erase_mode = SYSCFG_ERASE_MODE_SECTOR;
    ASSERT((pinfo->addr1 & ~(pinfo->flash1->sector_size - 1)) == pinfo->addr1);
    ASSERT((pinfo->addr2 & ~(pinfo->flash2->sector_size - 1)) == pinfo->addr2);
    ASSERT((pinfo->size >= sizeof(struct sys_config)) && 
           (pinfo->size == (pinfo->size & ~(pinfo->flash1->sector_size - 1))));
    return 0;
#else
    return -1;
#endif
}

int32 ota_fwinfo_get(struct ota_fwinfo *pinfo)
{
#if 1
    pinfo->flash0 = &flash0;
    pinfo->flash1 = &flash0;
    pinfo->addr0  = 0;
    pinfo->addr1  = flash0.size/2;
    return 0;
#else
    return -1;
#endif
}

