#ifndef HAL_LCDC_H
#define HAL_LCDC_H

/* User interrupt handle */
typedef int32(*lcdc_irq_hdl)(uint32 irq_flags, uint32 irq_data, uint32 param);

/**
  * @brief UART ioctl_cmd type
  */
enum lcdc_ioctl_cmd {
    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_TIMEOUT_INF,


    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_AUTO_KICK_EN,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_GAMMA_SATURATION_EN,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_GAMMA_SATURATION_VAL,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_CCM_EN,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_COLOR_MODE,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_BUS_MODE,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_INTERFACE_MODE,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_COLRARRAY,		

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_LCD_VAILD_SIZE,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_LCD_VISIBLE_SIZE,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_SIGNAL_CONFIG,		

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_MCU_SIGNAL_CONFIG,		

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_INVALID_LINE,		
	
    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_VALID_DOT,
	
    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_HLW_VLW,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_VIDEO_BIG_ENDIAN,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_VIDEO_SIZE,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_P0_ROTATE_Y_SRC_ADDR,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_P0_ROTATE_U_SRC_ADDR,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_P0_ROTATE_V_SRC_ADDR,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_P1_ROTATE_Y_SRC_ADDR,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_P1_ROTATE_U_SRC_ADDR,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_P1_ROTATE_V_SRC_ADDR,		

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_P0P1_ROTATE_START_ADDR,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_ROTATE_P0_UP,		


    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_ROTATE_P0_P1_EN,		

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_ROTATE_LINE_BUF_NUM,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_ROTATE_LINE_BUF_Y,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_ROTATE_LINE_BUF_U,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_ROTATE_LINE_BUF_V,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_ROTATE_MIRROR,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_ROTATE_SET_SIZE,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_VIDEO_MODE,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_VIDEO_START_ADDR,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_VIDEO_EN,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_OSD_SIZE,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_OSD_START_ADDR,		

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_OSD_LUTDMA_ADDR,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_OSD_DMA_ADDR,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_OSD_DMA_LEN,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_OSD_FORMAT,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_OSD_ALPHA,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_OSD_ENC_HEAD,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_OSD_ENC_DIAP,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_OSD_EN,		

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_OSD_ENC_SRC_ADDR,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_OSD_ENC_DST_ADDR,
	
	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	LCDC_SET_OSD_ENC_SRC_LEN,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	LCDC_GET_OSD_ENC_DST_LEN,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	LCDC_SET_OSD_ENC_START,

	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	LCDC_MCU_CPU_WRITE,


	/*! Compatible version: V2;
	 *@ Describe:
	 *
	 */
	LCDC_MCU_CPU_READ,

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
	LCDC_SET_START_RUN,	

    /*! Compatible version: V2;
     *@ Describe:
     *
     */
    LCDC_SET_TE_EDGE,

    /*! Compatible version: V2;
     *@ Describe:饱和度使能
     *
     */
    LCDC_SET_SATURATION_EN,

    /*! Compatible version: V2;
     *@ Describe:饱和度的值
        0x0：0%
        0x1：12.5%
        0x2：25.0%
        0x3：37.5%
        0x4：50.0%
        0x5：62.5%
        0x6：75.0%
        0x7：87.5%
        0x8：100.0%
        0x9：112.5%
        0xA：125.0%
        0xB：137.5%
        0xC：150.0%
        0xD：162.5%
        0xE：175.0%
        0xF：187.5%
        0x10：200.0%
     *
     */
    LCDC_SET_SATURATION_VALUE,

    /*! Compatible version: V2;
     *@ Describe:对比度使能
     *
     */
    LCDC_SET_CONSTRAST_EN,
    /*! Compatible version: V2;
     *@ Describe:对比度的值
        0x0：50%
        0x1：53.125%
        0x2：56.25%
        0x3：59.375%
        0x4：62.5%
        0x5：65.625%
        0x6：68.75%
        0x7：71.875%
        0x8：75.0%
        0x9：78.125%
        0xA：81.25%
        0xB：84.375%
        0xC：87.5%
        0xD：90.625%
        0xE：93.75%
        0xF：96.875%
        0x10：100%
        0x11：103.125%
        0x12：106.25%
        0x13：109.375%
        0x14：112.5%
        0x15：115.625%
        0x16：118.75%
        0x17：121.875%
        0x18：125%
        0x19：128.125%
        0x1A：131.25%
        0x1B：134.375%
        0x1C：137.5%
        0x1D：140.625%
        0x1E：143.75%
        0x1F：146.875%
     *
     */
    LCDC_SET_CONSTRAST_VALUE,
    /*! Compatible version: V2;
     *@ Describe:gamma的使能
     *
     */
    LCDC_SET_GAMMA_EN,

    /*! Compatible version: V2;
     *@ Describe:配置GAMMA的rgb索引表
     表大小是256byte,只支持sram地址
     *
     */
    LCDC_SET_GAMMA_R,
    LCDC_SET_GAMMA_G,
    LCDC_SET_GAMMA_B,

    LCDC_SET_CCM_COEF1,
    LCDC_SET_CCM_COEF0,
    LCDC_SET_CCM_COEF2,
    LCDC_SET_CCM_COEF3,
    LCDC_GET_CON,
};


enum lcdc_color_mode {
    LCD_MODE_565 = 16,
	LCD_MODE_666 = 18,
	LCD_MODE_888 = 24,		
};

enum lcdc_bus_width {
    LCD_BUS_WIDTH_6  = 6,
	LCD_BUS_WIDTH_8  = 8,
	LCD_BUS_WIDTH_9  = 9,
	LCD_BUS_WIDTH_12 = 12,
	LCD_BUS_WIDTH_16 = 16,
	LCD_BUS_WIDTH_18 = 18,
	LCD_BUS_WIDTH_24 = 24,
};

enum lcdc_interface_mode {
    LCD_BUS_RGB,
	LCD_BUS_I80,
	LCD_BUS_I68,		
};

enum lcdc_rotate_mode {
    LCD_ROTATE_0,
	LCD_ROTATE_90,
	LCD_ROTATE_180,	
	LCD_ROTATE_270,		
};

enum lcdc_video_mode {
    VIDEO_FROM_SCALE,
	VIDEO_FROM_MEMORY_NO_ROTATE,
	VIDEO_FROM_MEMORY_ROTATE,
};

enum lcdc_osd_format {
    OSD_RGB_256,
	OSD_RGB_565,
	OSD_RGB_888,
};


struct lcdc_device {
    struct dev_obj dev;
};

struct lcdc_hal_ops {
	struct devobj_ops ops;
    int32(*init)(struct lcdc_device *lcdc_dev);
	int32(*suspend)(struct lcdc_device *lcdc_dev);
	int32(*resume)(struct lcdc_device *lcdc_dev);		
    int32(*baudrate)(struct lcdc_device *lcdc_dev, uint32 baudrate);
    int32(*open)(struct lcdc_device *lcdc_dev);
    int32(*close)(struct lcdc_device *lcdc_dev);
    int32(*ioctl)(struct lcdc_device *lcdc_dev, enum lcdc_ioctl_cmd ioctl_cmd, uint32 param1, uint32 param2);
    int32(*request_irq)(struct lcdc_device *lcdc_dev, uint32 irq_flag, lcdc_irq_hdl irq_hdl, uint32 irq_data);
    int32(*release_irq)(struct lcdc_device *lcdc_dev, uint32 irq_flag);
};






int32 lcdc_init(struct lcdc_device *p_lcdc);
int32 lcdc_open(struct lcdc_device *p_lcdc);
int32 lcdc_close(struct lcdc_device *p_lcdc);
int32 lcdc_request_irq(struct lcdc_device *p_lcdc, uint32 irq_flags, lcdc_irq_hdl irq_hdl, uint32 irq_data);
int32 lcdc_release_irq(struct lcdc_device *p_lcdc, uint32 irq_flags);
int32 lcdc_set_baudrate(struct lcdc_device *p_lcdc, uint32 mclk);
int32 lcdc_set_color_mode(struct lcdc_device *p_lcdc, enum lcdc_color_mode cm);
int32 lcdc_set_bus_width(struct lcdc_device *p_lcdc, enum lcdc_bus_width bw);
int32 lcdc_set_interface(struct lcdc_device *p_lcdc, enum lcdc_interface_mode im);
int32 lcdc_set_colrarray(struct lcdc_device *p_lcdc, uint8 colrarray);
int32 lcdc_set_lcd_vaild_size(struct lcdc_device *p_lcdc, uint32 w,uint32 h,uint8 pixel_dot_num);
int32 lcdc_set_lcd_visible_size(struct lcdc_device *p_lcdc, uint32 w,uint32 h,uint8 pixel_dot_num);
int32 lcdc_signal_config(struct lcdc_device *p_lcdc, uint8 vs_en,uint8 hs_en,uint8 de_en,uint8 vs_inv,uint8 hs_inv,uint8 de_inv,uint8 clk_inv);
int32 lcdc_mcu_signal_config(struct lcdc_device *p_lcdc, uint8 value);
int32 lcdc_set_invalid_line(struct lcdc_device *p_lcdc, uint8 invalid_line);
int32 lcdc_set_valid_dot(struct lcdc_device *p_lcdc, uint32 w_start,uint32 h_start);
int32 lcdc_set_hlw_vlw(struct lcdc_device *p_lcdc, uint32 hlw,uint32 vlw);
int32 lcdc_set_bigendian(struct lcdc_device *p_lcdc, uint8 en);
int32 lcdc_set_video_size(struct lcdc_device *p_lcdc, uint32 w,uint32 h);
int32 lcdc_set_p0_rotate_y_src_addr(struct lcdc_device *p_lcdc, uint32 yaddr);
int32 lcdc_set_p0_rotate_u_src_addr(struct lcdc_device *p_lcdc, uint32 uaddr);
int32 lcdc_set_p0_rotate_v_src_addr(struct lcdc_device *p_lcdc, uint32 vaddr);
int32 lcdc_set_p1_rotate_y_src_addr(struct lcdc_device *p_lcdc, uint32 yaddr);
int32 lcdc_set_p1_rotate_u_src_addr(struct lcdc_device *p_lcdc, uint32 uaddr);
int32 lcdc_set_p1_rotate_v_src_addr(struct lcdc_device *p_lcdc, uint32 vaddr);
int32 lcdc_set_rotate_p0_up(struct lcdc_device *p_lcdc, uint8 en);
int32 lcdc_set_rotate_p0p1_start_location(struct lcdc_device *p_lcdc, uint32 x0,uint32 y0,uint32 x1,uint32 y1);
int32 lcdc_set_rotate_linebuf_num(struct lcdc_device *p_lcdc, uint32 linebuf);
int32 lcdc_set_rotate_linebuf_y_addr(struct lcdc_device *p_lcdc, uint32 yaddr);
int32 lcdc_set_rotate_linebuf_u_addr(struct lcdc_device *p_lcdc, uint32 uaddr);
int32 lcdc_set_rotate_linebuf_v_addr(struct lcdc_device *p_lcdc, uint32 vaddr);
int32 lcdc_set_rotate_mirror(struct lcdc_device *p_lcdc, uint8 mirror,enum lcdc_rotate_mode rotate);
int32 lcdc_set_rotate_p0p1_size(struct lcdc_device *p_lcdc, uint32 w0,uint32 h0,uint32 w1,uint32 h1);
int32 lcdc_set_video_data_from(struct lcdc_device *p_lcdc, enum lcdc_video_mode vm);
int32 lcdc_set_video_start_location(struct lcdc_device *p_lcdc, uint32 x,uint32 y);
int32 lcdc_set_video_en(struct lcdc_device *p_lcdc, uint8 en);
int32 lcdc_set_osd_size(struct lcdc_device *p_lcdc, uint32 w,uint32 h);
int32 lcdc_set_osd_start_location(struct lcdc_device *p_lcdc, uint32 x,uint32 y);
int32 lcdc_set_osd_dma_addr(struct lcdc_device *p_lcdc, uint32 addr);
int32 lcdc_set_osd_lut_addr(struct lcdc_device *p_lcdc, uint32 addr);
int32 lcdc_set_osd_dma_len(struct lcdc_device *p_lcdc, uint32 len);
int32 lcdc_set_osd_format(struct lcdc_device *p_lcdc, enum lcdc_osd_format osd_format);
int32 lcdc_set_osd_alpha(struct lcdc_device *p_lcdc, uint32 alpha);
int32 lcdc_set_osd_enc_head(struct lcdc_device *p_lcdc, uint32 head, uint32 head_tran);
int32 lcdc_set_osd_enc_diap(struct lcdc_device *p_lcdc, uint32 diap, uint32 diap_tran);
int32 lcdc_set_osd_en(struct lcdc_device *p_lcdc, uint8 en);
int32 lcdc_set_osd_enc_src_addr(struct lcdc_device *p_lcdc, uint32 addr);
int32 lcdc_set_osd_enc_dst_addr(struct lcdc_device *p_lcdc, uint32 addr);
int32 lcdc_set_p0p1_enable(struct lcdc_device *p_lcdc, uint8 p0_en,uint8 p1_en);

int32 lcdc_set_osd_enc_src_len(struct lcdc_device *p_lcdc, uint32 len);
int32 lcdc_get_osd_enc_dst_len(struct lcdc_device *p_lcdc);
int32 lcdc_osd_enc_start_run(struct lcdc_device *p_lcdc,uint8_t en);
int32 lcdc_set_start_run(struct lcdc_device *p_lcdc);
int32 lcdc_video_enable_gamma_saturation(struct lcdc_device *p_lcdc, uint8 gamma,uint8 saturation);
int32 lcdc_video_gamma_saturation_val(struct lcdc_device *p_lcdc, uint32 gamma,uint32 saturation);
int32 lcdc_video_enable_ccm(struct lcdc_device *p_lcdc, uint8 ccm);
int32 lcdc_mcu_write_reg(struct lcdc_device *p_lcdc,uint8 data);
int32 lcdc_mcu_write_data(struct lcdc_device *p_lcdc,uint8 data);
int32 lcdc_video_enable_auto_ks(struct lcdc_device *p_lcdc, uint8 enable);
int32 lcdc_set_timeout_info(struct lcdc_device *p_lcdc, uint8 enable,uint8 timeout);
int32 lcdc_mcu_read_data(struct lcdc_device *p_lcdc,uint8 *read_buf,uint8 cmd,uint8 len);

int32 lcdc_set_te_edge(struct lcdc_device *p_lcdc,uint8 edge);
int32 lcdc_video_enable_saturation(struct lcdc_device *p_lcdc, uint8 en);
int32 lcdc_video_set_saturation_val(struct lcdc_device *p_lcdc, uint8 val);
int32 lcdc_video_enable_constarast(struct lcdc_device *p_lcdc, uint8 en);
int32 lcdc_video_set_constarast_val(struct lcdc_device *p_lcdc, uint8 val);
int32 lcdc_video_enable_gamma(struct lcdc_device *p_lcdc, uint8 en);
int32 lcdc_video_set_gamma_R(struct lcdc_device *p_lcdc, uint32 addr);
int32 lcdc_video_set_gamma_G(struct lcdc_device *p_lcdc, uint32 addr);
int32 lcdc_video_set_gamma_B(struct lcdc_device *p_lcdc, uint32 addr);
int32 lcdc_video_set_CCM_COEF0(struct lcdc_device *p_lcdc, uint32 matrix);
int32 lcdc_video_set_CCM_COEF1(struct lcdc_device *p_lcdc, uint32 matrix);
int32 lcdc_video_set_CCM_COEF2(struct lcdc_device *p_lcdc, uint32 matrix);
int32 lcdc_video_set_CCM_COEF3(struct lcdc_device *p_lcdc, uint32 offset);
int32 lcdc_get_con(struct lcdc_device *p_lcdc);
#endif

