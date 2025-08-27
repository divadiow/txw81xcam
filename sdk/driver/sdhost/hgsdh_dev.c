#include "sys_config.h"
#include "typesdef.h"
#include "devid.h"
#include "list.h"
#include "dev.h"
#include "osal/task.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "lib/sdhost/sdhost.h"
#include "hal/gpio.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/task.h"
#include "osal/sleep.h"
#include "osal/timer.h"


struct hgsdh_hw
{
    __IO uint32 CFG;
    __IO uint32 BAUD;
    __IO uint32 CPTR;
    __IO uint32 DPTR;
    __IO uint32 DCNT;	
#if TXW81X
    __IO uint32 STA;	
    __IO uint32 CFG1;	
    __IO uint32 ISMP;
#endif	
};


static uint8 sdhc_cmd_rsp_buf[32]__attribute__ ((aligned(4)));
#if TXW81X
static uint8 sdhc_dat_ping_buf0[512];
static uint8 sdhc_dat_ping_buf1[512];
#endif
void delay_us(uint32 n);

struct sdh_device *sdh_irq;

static void ll_sdhc_set_bus_clk(struct hgsdh_hw *p_sdhc, uint32 sys_clk, uint32 bus_clk)
{
    p_sdhc->BAUD = ((sys_clk + bus_clk - 1)/ bus_clk) - 1;
}

static void ll_sdhc_set_ioe(struct hgsdh_hw *p_sdhc, TYPE_LL_SDHC_IOE ioe)
{
    if (ioe == LL_SDHC_IROF) {
        p_sdhc->CFG &= ~LL_SDHC_ODAT_RISING_EDGE;
    } else {
        p_sdhc->CFG |= LL_SDHC_ODAT_RISING_EDGE;
    }
}

static void ll_sdhc_set_dat_width(struct hgsdh_hw *p_sdhc, TYPE_LL_SDHC_DAT_WIDTH width)
{
    if (width == LL_SDHC_DAT_4BIT) {
        p_sdhc->CFG |= LL_SDHC_DAT_WIDTH_4BIT;
    } else {
        p_sdhc->CFG &= ~LL_SDHC_DAT_WIDTH_4BIT;
    }
}

static void ll_sdhc_clk_suspend_ctrl(struct hgsdh_hw *p_sdhc, uint8 suspend_enable)
{
    if (suspend_enable) {
        p_sdhc->CFG &= ~LL_SDHC_CLK_OUT_EN;
    } else {
        p_sdhc->CFG |= LL_SDHC_CLK_OUT_EN;
    }
}

static void ll_sdhc_post_8clk_ctrl(struct hgsdh_hw *p_sdhc, uint8 enable)
{
    if (enable) {
        p_sdhc->CFG |= LL_SDHC_8CLK_AFTER_EN;
        //p_sdhc->CFG |= LL_SDHC_8CLK_BEFORE_EN;
    } else {
        p_sdhc->CFG &= ~LL_SDHC_8CLK_AFTER_EN;
        //p_sdhc->CFG &= ~LL_SDHC_8CLK_BEFORE_EN;       
    }
}

static void ll_sdhc_before_8clk_ctrl(struct hgsdh_hw *p_sdhc, uint8 enable)
{
    if (enable) {
        p_sdhc->CFG |= LL_SDHC_8CLK_BEFORE_EN;
    } else {
        p_sdhc->CFG &= ~LL_SDHC_8CLK_BEFORE_EN;       
    }
}
static void ll_sdhc_8clk_ctrl(struct hgsdh_hw *p_sdhc, uint8 enable)
{
    ll_sdhc_before_8clk_ctrl(p_sdhc, enable);
}
static void ll_sdhc_cmd_intr_enable(struct hgsdh_hw *p_sdhc, uint8 enable)
{
    if (enable) {
        p_sdhc->CFG |= LL_SDHC_CMD_INTR_EN;
    } else {
        p_sdhc->CFG &= ~LL_SDHC_CMD_INTR_EN;
    }
}

/**
 * @brief   ll_sdhc_set_dat_width
 * @param   p_sdhc  : sdhc register structure pointer
 * @retval  none
 */
static void ll_sdhc_dat_intr_enable(struct hgsdh_hw *p_sdhc, uint8 enable)
{
    if (enable) {
        p_sdhc->CFG |= LL_SDHC_DAT_INTR_EN;
    } else {
        p_sdhc->CFG &= ~LL_SDHC_DAT_INTR_EN;
    }
}

static void ll_sdhc_clr_cmd_done_pending(struct hgsdh_hw *p_sdhc)
{
    p_sdhc->CFG |= LL_SDHC_CMD_DONE_CLR;
}

#ifdef TXW81X
static void ll_sdhc_cmd_sample_enable(struct hgsdh_hw *p_sdhc, uint8 enable)
{
    if (enable)
    {
        p_sdhc->CFG1 |= LL_SDHC_CMD_SMP_EN;
    }else{
        p_sdhc->CFG1 &= ~LL_SDHC_CMD_SMP_EN;
    } 
}

static void ll_sdhc_dat_sample_enable(struct hgsdh_hw *p_sdhc, uint8 enable)
{
    if (enable)
    {
        p_sdhc->CFG1 |= LL_SDHC_DAT_SMP_EN;
    }else{
        p_sdhc->CFG1 &= ~LL_SDHC_DAT_SMP_EN;
    } 
}

static void ll_sdhc_delay_clock(struct hgsdh_hw *p_sdhc, TYPE_LL_SDHC_DELAY_SYSCLK type , uint8_t delay)
{
    if (type == LL_SDHC_DLY_HALF_SYSCLK)
    {
        p_sdhc->CFG1 &= ~0xff;
        p_sdhc->CFG1 |= LL_SDHC_DELAY_HALF_SYSCLK;
    }else if(type == LL_SDHC_DLY_ONE_SYSCLK){
        p_sdhc->CFG1 &= ~0xff;
        p_sdhc->CFG1 |= LL_SDHC_DELAY_WHOLE_SYSCLK;
    }else if(type == LL_SDHC_DLY_CHAIN){
        p_sdhc->CFG1 &= ~0xff;
        p_sdhc->CFG1 |= LL_SDHC_DELAY_CHAIN_EN;
        p_sdhc->CFG1 |= delay;
    }else{
        p_sdhc->CFG1 &= ~0xff;
    }
}

static void ll_sdhc_dat_overflow_stop_enable(struct hgsdh_hw *p_sdhc, uint8_t enable)
{
    if (enable)
    {
        p_sdhc->CFG1 |= LL_SDHC_DAT_OVERFLOW_STOP_EN;
    }else{
        p_sdhc->CFG1 &= ~LL_SDHC_DAT_OVERFLOW_STOP_EN;
    }
}
#endif

static bool ll_sdhc_get_cmd_done_pending(struct hgsdh_hw *p_sdhc)
{
    return ((p_sdhc->CFG & LL_SDHC_CMD_DONE)) ? TRUE : FALSE;
}

static bool ll_sdhc_get_cmd_rsp_timeout_pending(struct hgsdh_hw *p_sdhc)
{
    return (p_sdhc->CFG & LL_SDHC_RSP_TIMEOUT) ? TRUE : FALSE;
}

static bool ll_sdhc_get_cmd_rsp_err_pending(struct hgsdh_hw *p_sdhc)
{
    return (p_sdhc->CFG & LL_SDHC_RSP_CRC_ERR) ? TRUE : FALSE;
}

static void ll_sdhc_clr_dat_done_pending(struct hgsdh_hw *p_sdhc)
{
    p_sdhc->CFG |= LL_SDHC_DAT_DONE_CLR;
}

static bool ll_sdhc_get_dat_done_pending(struct hgsdh_hw *p_sdhc)
{
    return ((p_sdhc->CFG & LL_SDHC_DAT_DONE)) ? TRUE : FALSE;
}

static bool ll_sdhc_get_rd_dat_err_pending(struct hgsdh_hw *p_sdhc)
{
    return (p_sdhc->CFG & LL_SDHC_DAT_CRC_ERR) ? TRUE : FALSE;
}

static bool ll_sdhc_get_wr_dat_err_pending(struct hgsdh_hw *p_sdhc)
{
    return ((p_sdhc->CFG & LL_SDHC_CRC_STA) != (2<<16)) ? TRUE : FALSE;
}

static void ll_sdhc_read_data_kick(struct hgsdh_hw *p_sdhc, uint32 addr, uint32 len)
{
    p_sdhc->CFG &= ~LL_SDHC_DAT_RW_MSK;
	p_sdhc->CFG &= ~LL_SDHC_CLK_OUT_EN;
    ll_sdhc_clr_dat_done_pending(p_sdhc);

    //os_printf("\r\nDPTR:%x\r\n",addr);
    p_sdhc->DPTR = addr;
    p_sdhc->DCNT = len & (~ 0x3UL);
    p_sdhc->CFG |= LL_SDHC_DAT_RCV;
}

static void ll_sdhc_write_data_kick(struct hgsdh_hw *p_sdhc, uint32 addr, uint32 len)
{
    p_sdhc->CFG &= ~LL_SDHC_DAT_RW_MSK;
    ll_sdhc_clr_dat_done_pending(p_sdhc);

    p_sdhc->DPTR = addr;
    p_sdhc->DCNT = len & (~ 0x3UL);
    p_sdhc->CFG |= LL_SDHC_DAT_SEND_WAIT_BUSY;
}

static bool ll_sdhc_read_data_wait(struct hgsdh_hw *p_sdhc, uint32 time_ms)
{
    do {

    } while (!ll_sdhc_get_dat_done_pending(p_sdhc));


    if (ll_sdhc_get_rd_dat_err_pending(p_sdhc)) {
        os_printf("dat err\r\n");
        return FALSE;
    }
    return TRUE;
}

static bool ll_sdhc_write_data_wait(struct hgsdh_hw *p_sdhc, uint32 time_ms)
{
    uint64 time = os_jiffies();
    do {
        if ((os_jiffies() - time) > time_ms) {
            return FALSE;
        }
    } while (!ll_sdhc_get_dat_done_pending(p_sdhc));

    if (ll_sdhc_get_wr_dat_err_pending(p_sdhc)) {
        return FALSE;
    }
    return TRUE;
}


void hgic_sdc_irq_update(struct sdh_device  *host, int32_t enable)
{
	struct hgsdh *sdh_hw = (struct hgsdh*)host; 
	struct hgsdh_hw *hw =  (struct hgsdh_hw *)sdh_hw->hw;

    if (enable) {
        os_printf("enable sdio irq");
    } else {
        os_printf("disable sdio irq");
    }
#if CMD_ISR_EN
    ll_sdhc_cmd_intr_enable(hw, enable);
#endif
#if DAT_ISR_EN  
    ll_sdhc_dat_intr_enable(hw, enable);
#endif
//  ll_sdhc_dat1_intr_enable(host->p_sdhc, enable);

}



//volatile uint8 sd_8clk_open = 0;
void ll_sdhc_config(struct sdh_device *sdhost,struct hgsdh_hw * p_sdhc, TYPE_LL_SDHC_CFG *p_cfg)
{
    ll_sdhc_set_bus_clk(p_sdhc, peripheral_clock_get(HG_AHB_PT_SDMMC), p_cfg->bus_clk);
    ll_sdhc_set_ioe(p_sdhc, p_cfg->ioe);
    ll_sdhc_set_dat_width(p_sdhc, p_cfg->width);

    ll_sdhc_clk_suspend_ctrl(p_sdhc, 0);
    ll_sdhc_8clk_ctrl(p_sdhc, sdhost->sd_8clk_open);

    p_sdhc->CFG  |= LL_SDHC_EN;
#ifdef TXW81X
	p_sdhc->CFG1 |= (LL_SDHC_DAT_OVERFLOW_STOP_EN);
#endif
}

void ll_sdhc_close(struct hgsdh_hw * p_sdhc)
{
    p_sdhc->CFG &= ~LL_SDHC_EN;
}

#ifdef TXW81X
void ll_sdhc_set_sample(struct hgsdh_hw *p_sdhc, uint8_t cmd_sample, uint8_t dat_sample)
{
    p_sdhc->ISMP = (dat_sample << 16) | cmd_sample;
}
#endif

volatile uint8 sd_cmd_isr_get = 0;
volatile uint8 sd_dat_isr_get = 0;

int32 sdh_cmd(struct sdh_device *sdhost,struct rt_mmcsd_cmd* cmd)
{
    uint32 rsp_type;
    uint32 cmd_tick;
    //uint32 cur_tick;
	struct hgsdh *sdh_hw = (struct hgsdh*)sdhost; 
	struct hgsdh_hw *p_sdhc =  (struct hgsdh_hw *)sdh_hw->hw;
    sdhc_cmd_rsp_buf[0] = cmd->cmd_code | 0x40;
    //put_unaligned_be32(cmd->arg, &sdhc_cmd_rsp_buf[1]);
    sdhc_cmd_rsp_buf[1] = (cmd->arg >> 24) & 0xFF;
    sdhc_cmd_rsp_buf[2] = (cmd->arg >> 16) & 0xFF;
    sdhc_cmd_rsp_buf[3] = (cmd->arg >> 8) & 0xFF;
    sdhc_cmd_rsp_buf[4] = (cmd->arg >> 0) & 0xFF;
    p_sdhc->CPTR = (uint32)sdhc_cmd_rsp_buf;


    rsp_type = ((cmd->flags & MMC_RSP_PRESENT) ? BIT(5) : 0) |
               ((cmd->flags & MMC_RSP_136) ? BIT(6) : 0) |
               ((cmd->flags & MMC_RSP_BUSY) ? BIT(7) : 0);
    
    if (0 == rsp_type) {
        rsp_type = (0x2 << 5);
    }
    //ll_sdhc_post_8clk_ctrl(p_sdhc, sd_8clk_open);         
    ll_sdhc_clr_cmd_done_pending(p_sdhc);
//  os_printf("cmd:%d  %x\r\n",cmd->cmd_code,(p_sdhc->CFG & ~LL_SDHC_CMD_RESP_MSK) | rsp_type);
    p_sdhc->CFG  = (p_sdhc->CFG & ~LL_SDHC_CMD_RESP_MSK) | rsp_type;

    if(cmd->cmd_code == STOP_TRANSMISSION)
    {
        cmd_tick = ~0;
    }
    else
    {
        cmd_tick = 0x1fffff;
    }
    //ll_gpio_bit_reset(GPIOB,BIT(5));
    while (!ll_sdhc_get_cmd_done_pending(p_sdhc)&&!sd_cmd_isr_get)
    {
        if (--cmd_tick == 0) {
            //ll_gpio_bit_reset(GPIOB,BIT(5));
        
            
            //sdreg(base,SDxCON1) = BIT(BIT_CPCLR);
            //sdreg(base,SDxCON0) &= ~BIT(BIT_8CKE);
            ll_sdhc_clr_cmd_done_pending(p_sdhc);
//          ll_sdhc_post_8clk_ctrl(p_sdhc,0);
//          p_sdhc->CFG &= ~(BIT(5)|BIT(6)|BIT(7)); 
            p_sdhc->CFG &= ~BIT(0);
//          os_printf ("CMD%d pending time out!resp:%x\r\n", cmd->cmd_code,rsp_type);
            //ll_gpio_bit_set(GPIOB,BIT(5));
            p_sdhc->CFG |= BIT(0);
            
            //ll_sdhc_post_8clk_ctrl(p_sdhc, 0);        
            return -1;
        }

            
    }

    sd_cmd_isr_get = 0;
    //ll_gpio_bit_set(GPIOB,BIT(5));
    if ((rsp_type != (0x2 << 5)) && (ll_sdhc_get_cmd_rsp_timeout_pending(p_sdhc) || ((cmd->flags & MMC_RSP_CRC) && ll_sdhc_get_cmd_rsp_err_pending(p_sdhc))))
    {
        os_printf("read timeout pending set or rsp err:%d %d\r\n",ll_sdhc_get_cmd_rsp_timeout_pending(p_sdhc),ll_sdhc_get_cmd_rsp_err_pending(p_sdhc));
        
        return -1;
    }
   
    cmd->resp[0] = (sdhc_cmd_rsp_buf[9] << 24)  | 
                   (sdhc_cmd_rsp_buf[10] << 16) |
                   (sdhc_cmd_rsp_buf[11] << 8)  |
                   (sdhc_cmd_rsp_buf[12] << 0);

    cmd->resp[1] = (sdhc_cmd_rsp_buf[13] << 24)  | 
                   (sdhc_cmd_rsp_buf[14] << 16) |
                   (sdhc_cmd_rsp_buf[15] << 8)  |
                   (sdhc_cmd_rsp_buf[16] << 0);

    cmd->resp[2] = (sdhc_cmd_rsp_buf[17] << 24)  | 
                   (sdhc_cmd_rsp_buf[18] << 16) |
                   (sdhc_cmd_rsp_buf[19] << 8)  |
                   (sdhc_cmd_rsp_buf[20] << 0);
    
    cmd->resp[3] = (sdhc_cmd_rsp_buf[21] << 24)  | 
                   (sdhc_cmd_rsp_buf[22] << 16) |
                   (sdhc_cmd_rsp_buf[23] << 8)  |
                   (sdhc_cmd_rsp_buf[24] << 0); 
#if 0
    os_printf("\r\n"); 
    for(cmd_tick = 0;cmd_tick < 32;cmd_tick++){
        os_printf("%02x",sdhc_cmd_rsp_buf[cmd_tick]);
    }                  
    os_printf("\r\n"); 
#endif  
//  os_printf("cmd->resp:%x\r\n",cmd->resp[0]);
//  ll_sdhc_post_8clk_ctrl(p_sdhc, 0);      

    return 0;    
}


void sdhc_interrupt_handle(struct sdh_device *sdhost)
{
	struct hgsdh *sdh_hw = (struct hgsdh*)sdhost; 
	struct hgsdh_hw *hw =  (struct hgsdh_hw *)sdh_hw->hw;
    if (ll_sdhc_get_dat_done_pending(hw)){
        ll_sdhc_clr_dat_done_pending(hw);
        if(sdhost->sd_opt == SD_M_R){
            if(ll_sdhc_get_rd_dat_err_pending(hw))
            {
                if(sdhost->io_cfg.self_adaption_flag != MMCSD_SMP_EN)    os_printf("read dat err pending\r\n");
                sdhost->data.err = DATA_DIR_READ;
                os_sema_up(&sdhost->dat_sema);
                return;
            }

            if(sdhost->data.blks - 1)
            {
                sdhost->data.blks = sdhost->data.blks -1;
                sdhost->data.buf  = sdhost->data.buf + sdhost->data.blksize;
                if(sdhost->data.psram_flag)
                {
                    ll_sdhc_read_data_kick(hw, (uint32_t)sdhost->data.ping_buff[(sdhost->data.ping_sel+1)%2], sdhost->data.blksize);
                    hw_memcpy((void *)(sdhost->data.buf - sdhost->data.blksize), (void *)sdhost->data.ping_buff[sdhost->data.ping_sel++], sdhost->data.blksize);
                } else {
                    ll_sdhc_read_data_kick(hw, (uint32_t)sdhost->data.buf, sdhost->data.blksize);
                }
            }else
            {
                os_sema_up(&sdhost->dat_sema);
            }

        }
        else if(sdhost->sd_opt == SD_M_W)
        {
            if (ll_sdhc_get_wr_dat_err_pending(hw))
            {
                os_printf("write dat err pending\r\n");
                sdhost->data.err = DATA_DIR_READ;
                os_sema_up(&sdhost->dat_sema);
                return;
            }
            
            if(sdhost->data.blks - 1)
            {
                sdhost->data.blks = sdhost->data.blks -1;
                sdhost->data.buf  = sdhost->data.buf + sdhost->data.blksize;
                ll_sdhc_write_data_kick(hw, (uint32_t)sdhost->data.buf, sdhost->data.blksize);
            }else
            {
                os_sema_up(&sdhost->dat_sema);
            }           
        }
        else
        {
            os_sema_up(&sdhost->dat_sema);   //single read/write
        }
    } 
    
}



void SDHOST_IRQHandler_action(void *sdhost)
{
    sdhc_interrupt_handle(sdhost);
}


int32 sdh_write(struct sdh_device *sdhost,uint8_t * buf)
{
	struct hgsdh *sdh_hw = (struct hgsdh*)sdhost; 
	struct hgsdh_hw *hw  =  (struct hgsdh_hw *)sdh_hw->hw;
    uint32_t xfer_len    = sdhost->data.blksize;  

#if DAT_ISR_EN
#if TXW81X
    sdhost->data.psram_flag = 0;
    sys_dcache_clean_range_unaligned((uint32 *)buf, (uint32_t)(sdhost->data.blks * sdhost->data.blksize));
#endif
    sdhost->data.buf = buf;
    ll_sdhc_write_data_kick(hw, (uint32_t)buf, xfer_len);
#else   
#if TXW81X
    sdhost->data.psram_flag = 0;
    sys_dcache_clean_range_unaligned((uint32 *)buf, (uint32_t)(sdhost->data.blks * sdhost->data.blksize));
#endif
    uint32_t blocks = sdhost->data.blks;
    uint32_t size = sdhost->data.blks * sdhost->data.blksize;   
    do {
        ll_sdhc_write_data_kick(hw, (uint32_t)buf, xfer_len);
        buf += xfer_len;
        if (blocks) {
            blocks--;
        }

        if (!ll_sdhc_write_data_wait(hw, sdhost->data.timeout_ns/1000000))                             
        {
            os_printf("write timeout:%d\r\n",sdhost->data.timeout_ns);
            sdhost->data.err = DATA_DIR_WRITE;
            break;
        }

        if (ll_sdhc_get_wr_dat_err_pending(hw)) {                  
            os_printf("write dat err pending\r\n");
            sdhost->data.err = DATA_DIR_WRITE;
        }

        sd_dat_isr_get = 0;
    } while (blocks);   
#endif  
    return 0;
}

int32 sdh_read(struct sdh_device *sdhost,uint8* buf)
{
	struct hgsdh *sdh_hw = (struct hgsdh*)sdhost; 
	struct hgsdh_hw *hw  =  (struct hgsdh_hw *)sdh_hw->hw;
    uint32_t xfer_len    = sdhost->data.blksize;   
    sdhost->sd_8clk_open = 0;
    ll_sdhc_8clk_ctrl(hw, sdhost->sd_8clk_open);
#if DAT_ISR_EN
    sdhost->data.buf = buf;
#if TXW81X 
    sdhost->data.psram_flag = ((((uint32_t)buf) >> 24) == 0x38 || (((uint32_t)buf) >> 24) == 0x08);
    if(sdhost->data.psram_flag) buf = (uint8 *)sdhost->data.ping_buff[sdhost->data.ping_sel];
#endif
    ll_sdhc_read_data_kick(hw, (uint32_t)buf, xfer_len);
#else
    uint32_t blocks = sdhost->data.blks;
    uint32_t size = sdhost->data.blks * sdhost->data.blksize;   
    do {
        ll_sdhc_read_data_kick(hw, (uint32_t)buf, xfer_len);
        buf += xfer_len;
        if (blocks) {
            blocks--;
        }
                      
        if (!ll_sdhc_read_data_wait(hw, sdhost->data.timeout_ns/1000000)) 
        {   
            os_printf("read dat timeout\r\n");
            //while(1);
            sdhost->data.err = DATA_DIR_READ;
        }


        if (ll_sdhc_get_rd_dat_err_pending(hw)) {
            os_printf("read dat err pending\r\n");
            sdhost->data.err = DATA_DIR_READ;
        }
        sd_dat_isr_get = 0;
    } while (blocks);
#endif  
    ll_sdhc_8clk_ctrl(hw,sdhost->sd_8clk_open);   

    return 0;
}

int32 sdh_complete(struct sdh_device *sdhost)
{
#if DAT_ISR_EN
	int res;
    int ret = 0;
    res = os_sema_down(&sdhost->dat_sema, 2000);
	struct hgsdh *sdh_hw = (struct hgsdh*)sdhost; 
	struct hgsdh_hw *hw =  (struct hgsdh_hw *)sdh_hw->hw;
	if(sdhost->data.err != 0 || res == 0){
        if(res == 0)
        {
            sdhost->sd_opt = SD_OFF;
        }
		ret = -1;
	}
    if(sdhost->sd_8clk_default) sdhost->sd_8clk_open = 1;
    ll_sdhc_8clk_ctrl(hw,sdhost->sd_8clk_open); 
#if TXW81X
    if(sdhost->data.psram_flag && (sdhost->sd_opt == SD_M_R)&& !ret)     hw_memcpy((void *)(sdhost->data.buf), (void *)sdhost->data.ping_buff[sdhost->data.ping_sel++], sdhost->data.blksize);
#endif
    return ret;
#endif
}


//set the sdh config
int32 sdh_open(struct sdh_device *sdhost,uint8 bus_w)
{
	struct hgsdh *sdh_hw = (struct hgsdh*)sdhost; 
	struct hgsdh_hw *hw =  (struct hgsdh_hw *)sdh_hw->hw;

    if (sdhost == 0) {
        os_printf("mmcsd alloc host fail");
        return 0;
    }
    sdh_hw->opened = 0;

	SYSCTRL->SYS_CON1 &= ~(BIT(20));
	delay_us(10);
	SYSCTRL->SYS_CON1 |= (BIT(20));
    sysctrl_sdhc_clk_open();
	
    sdh_irq = sdhost;
    /* set host default attributes */
    sdhost->sd_8clk_default = 1;
    sdhost->sd_8clk_open    = sdhost->sd_8clk_default;
    sdhost->freq_min = 400 * 1000;
#ifdef TXW81X
    sdhost->freq_max = 50000000;
#else
    sdhost->freq_max = 24000000;
#endif
    sdhost->valid_ocr = 0x00FFFF80;/* The voltage range supported is 1.65v-3.6v */

#ifdef TXW81X
	if(bus_w == 4)
    	sdhost->flags = MMCSD_BUSWIDTH_4 | MMCSD_MUTBLKWRITE | MMCSD_SUP_SDIO_IRQ | MMCSD_SUP_HIGHSPEED;
	else
    	sdhost->flags = MMCSD_MUTBLKWRITE | MMCSD_SUP_SDIO_IRQ | MMCSD_SUP_HIGHSPEED;
#else
	if(bus_w == 4)
    	sdhost->flags = MMCSD_BUSWIDTH_4 | MMCSD_MUTBLKWRITE | MMCSD_SUP_SDIO_IRQ;
	else
    	sdhost->flags = MMCSD_MUTBLKWRITE | MMCSD_SUP_SDIO_IRQ;
#endif

    sdhost->max_seg_size = 4096;
    sdhost->max_dma_segs = 1;
    sdhost->max_blk_size = 512;
    sdhost->max_blk_count = 512;
    sdhost->sd_stop       = 0;
    sdhost->sd_opt        = SD_IDLE;
    sdhost->new_lba       = 0;
    if(sdhost->dat_sema.hdl)
    {
        os_sema_del(&sdhost->dat_sema);
    }
    os_sema_init(&sdhost->dat_sema, 0);

    /* link up host and sdio */
    sdhost->private_data = 0;
    sdhost->data.timeout_ns =  100000000;  /* 100ms */
    TYPE_LL_SDHC_CFG cfg;
    cfg.bus_clk = sdhost->freq_min;
    cfg.ioe = LL_SDHC_IROF;//LL_SDHC_IROF;
    if(bus_w == 4)
    	cfg.width = LL_SDHC_DAT_4BIT;
	else
		cfg.width = LL_SDHC_DAT_1BIT;
    ll_sdhc_config(sdhost,hw, &cfg);

#ifdef TXW81X
    sdhost->data.ping_buff[0] = (uint32_t)sdhc_dat_ping_buf0;
    sdhost->data.ping_buff[1] = (uint32_t)sdhc_dat_ping_buf1;
    sdhost->data.ping_sel     = 0;
    sdhost->io_cfg.dat_overflow_stop_flag = 1;
	sdhost->io_cfg.self_adaption_flag	  = 0;
#endif

    hgic_sdc_irq_update(sdhost, 1);
    NVIC_EnableIRQ(SDHOST_IRQn);
	request_irq(SDHOST_IRQn, SDHOST_IRQHandler_action, (void*)sdhost);	
    return 0;
    
}

static void hgsdh_set_clock(struct sdh_device *sdhost, uint32_t clk)
{
	struct hgsdh *sdh_hw = (struct hgsdh*)sdhost; 
	struct hgsdh_hw *hw =  (struct hgsdh_hw *)sdh_hw->hw;
	
    if (clk < sdhost->freq_min) {
        clk = sdhost->freq_min;
    }

    if (clk > sdhost->freq_max) {
        clk = sdhost->freq_max;
    }
    
    if (clk > 25000000) {
        ll_sdhc_set_ioe(hw, LL_SDHC_IROR);
    } else {
        ll_sdhc_set_ioe(hw, LL_SDHC_IROF);
    }

    if (clk) {
        ll_sdhc_set_bus_clk(hw, sys_get_apbclk(), clk);
    }
}

static void hgsdh_set_bus_width(struct hgsdh_hw *p_sdhc, uint32 bus_width)
{
    switch (bus_width)
    {
        case MMCSD_BUS_WIDTH_1:
            ll_sdhc_set_dat_width(p_sdhc, LL_SDHC_DAT_1BIT);
            break;

        case MMCSD_BUS_WIDTH_4:
            ll_sdhc_set_dat_width(p_sdhc, LL_SDHC_DAT_4BIT);
            break;

        case MMCSD_BUS_WIDTH_8: 
            os_printf("bus errr\r\n");
            break;
        
        default:
            os_printf("no support bus width :%d\r\n", bus_width);
            break;
    }
}

static void hgsdh_set_sample(struct hgsdh_hw *p_sdhc, TYPE_LL_SDHC_SMP_CFG type, uint32_t cmd_smp, uint32_t dat_smp)
{
    switch (type)
    {
        case LL_SDHC_ALL_SMP_CFG_DIS:
            ll_sdhc_cmd_sample_enable(p_sdhc, 0);
            ll_sdhc_dat_sample_enable(p_sdhc, 0);
            ll_sdhc_set_sample(p_sdhc, 0, 0);
            break;
        
        case LL_SDHC_CMD_SMP_CFG_EN:
            ll_sdhc_cmd_sample_enable(p_sdhc, 1);
            ll_sdhc_set_sample(p_sdhc, cmd_smp, dat_smp);
            break;

        case LL_SDHC_DAT_SMP_CFG_EN:
            ll_sdhc_dat_sample_enable(p_sdhc, 1);
            ll_sdhc_set_sample(p_sdhc, cmd_smp, dat_smp);
            break;

        case LL_SDHC_ALL_SMP_CFG_EN:
            ll_sdhc_cmd_sample_enable(p_sdhc, 1);
            ll_sdhc_dat_sample_enable(p_sdhc, 1);
            ll_sdhc_set_sample(p_sdhc, cmd_smp, dat_smp);
            break;
        
        default:
            os_printf("sample type : %d err!\r\n", type);
            break;
    }
}

static void hgsdh_set_power_mode(uint8_t mode)
{
    switch (mode)
    {
        case MMCSD_POWER_OFF:
            break;
        case MMCSD_POWER_UP:
            break;
        case MMCSD_POWER_ON:
            break;
        default:
            os_printf("unknown power_mode %d", mode);
            break;
    }
}

int32 sdh_cfg(struct sdh_device *sdhost, struct rt_mmcsd_io_cfg *io_cfg)
{
	struct hgsdh *sdh_hw = (struct hgsdh*)sdhost; 
	struct hgsdh_hw *hw =  (struct hgsdh_hw *)sdh_hw->hw;

    switch (io_cfg->ioctl_type)
    {
        case LL_SDHC_IOCTRL_SET_CLOCK:
            hgsdh_set_clock(sdhost, io_cfg->clock);
            io_cfg->crc_sample_max = sys_get_apbclk() / io_cfg->clock;
            break;

        case LL_SDHC_IOCTRL_SET_BUS_WIDTH:
            hgsdh_set_bus_width(hw, io_cfg->bus_width);
            break;

#ifdef TXW81X
        case LL_SDHC_IOCTRL_SET_SMP:
            hgsdh_set_sample(hw, io_cfg->smp_type, io_cfg->cmd_crc_sample, io_cfg->dat_crc_sample);
            break;

        case LL_SDHC_IOCTRL_SET_DELAY_TYPE:
            (io_cfg->delay_flag) ? (ll_sdhc_delay_clock(hw, io_cfg->delay_type, io_cfg->delay_chain_cnt)) :
                                   (ll_sdhc_delay_clock(hw, LL_SDHC_DLY_NONE, 0));
            break;

        case LL_SDHC_IOCTRL_SET_DAT_OF_STOP_CLK:
            ll_sdhc_dat_overflow_stop_enable(hw, io_cfg->dat_overflow_stop_flag);
            break;
#endif
        case LL_SDHC_IOCTRL_SET_POWER_MODE:
            hgsdh_set_power_mode(io_cfg->power_mode);
            break;

        default:
            os_printf("err ioctrl_cmd : %d\r\n", io_cfg->ioctl_type);
            break;
    }

    if(io_cfg->ioctl_type < LL_SDHC_IOCTRL_SET_SMP)
    {
        os_printf("clk:%d width:%s%s%s power:%s%s%s\r\n",
            io_cfg->clock,
            io_cfg->bus_width == MMCSD_BUS_WIDTH_8 ? "8" : "",
            io_cfg->bus_width == MMCSD_BUS_WIDTH_4 ? "4" : "",
            io_cfg->bus_width == MMCSD_BUS_WIDTH_1 ? "1" : "",
            io_cfg->power_mode == MMCSD_POWER_OFF ? "OFF" : "",
            io_cfg->power_mode == MMCSD_POWER_UP ? "UP" : "",
            io_cfg->power_mode == MMCSD_POWER_ON ? "ON" : ""
            );      
    }

    return 0;
}


int32 sdh_close(struct sdh_device *sdhost)
{
	struct hgsdh *sdh_hw = (struct hgsdh*)sdhost; 
	struct hgsdh_hw *hw =  (struct hgsdh_hw *)sdh_hw->hw;
    ll_sdhc_close(hw);
    sdh_hw->opened = 0;
    sdh_hw->dsleep = 0;
    sysctrl_sdhc_clk_close();
    os_sema_del(&sdhost->dat_sema);
    NVIC_DisableIRQ(SDHOST_IRQn);
	pin_func(sdhost->dev.dev_id,0);
    return 0;
}

#ifdef CONFIG_SLEEP
int32 sdh_suspend(struct sdh_device *sdhost)
{
	struct hgsdh *sdh_hw = (struct hgsdh*)sdhost; 
	struct hgsdh_hw *hw  = (struct hgsdh_hw *)sdh_hw->hw;
	struct hgsdh_hw *hw_cfg;
    if (!sdh_hw->opened || sdh_hw->dsleep)
    {
        return RET_OK;
    }

    if (os_mutex_lock(&sdhost->lock, osWaitForever))
    {
        return RET_ERR;
    }

    if (0 > os_mutex_lock(&sdhost->bp_suspend_lock, 10000)) 
    {
        return RET_ERR;
    }
    
	sysctrl_sdhc_clk_close();
	sdhost->cfg_backup = (uint32 *)os_malloc(sizeof(struct hgsdh_hw));	
	//memcpy((uint8 *)sdhost->cfg_backup,(uint8 *)hw,sizeof(struct hgsdh_hw));
	hw_cfg = (struct hgsdh_hw*)sdhost->cfg_backup;
	hw_cfg->BAUD = hw->BAUD;
	hw_cfg->CFG  = hw->CFG;
	hw_cfg->CPTR = hw->CPTR;
	hw_cfg->DPTR = hw->DPTR;
	hw_cfg->DCNT = hw->DCNT;
#ifdef TXW81X
    hw_cfg->CFG1 = hw->CFG1;
    hw_cfg->ISMP = hw->ISMP;
#endif
	irq_disable(sdh_hw->irq_num);
    sdh_hw->dsleep = 1;
    os_mutex_unlock(&sdhost->bp_suspend_lock);
	return 0;
}

int32 sdh_resume(struct sdh_device *sdhost)
{
	struct hgsdh *sdh_hw = (struct hgsdh*)sdhost; 
	struct hgsdh_hw *hw =  (struct hgsdh_hw *)sdh_hw->hw;
	struct hgsdh_hw *hw_cfg;

    if ((!sdh_hw->opened) || (!sdh_hw->dsleep)) {
        return RET_OK;
    }

    if (0 > os_mutex_lock(&sdhost->bp_resume_lock, 10000)) 
    {
        return RET_ERR;
    }

	sysctrl_sdhc_clk_open();
	hw_cfg = (struct hgsdh_hw*)sdhost->cfg_backup;
	//memcpy((uint8 *)hw,(uint8 *)sdhost->cfg_backup,sizeof(struct hgsdh_hw));	
	hw->BAUD = hw_cfg->BAUD;
	hw->CPTR = hw_cfg->CPTR;
	hw->DPTR = hw_cfg->DPTR;
	hw->DCNT = hw_cfg->DCNT;
	hw->CFG  = hw_cfg->CFG;
#ifdef TXW81X
    hw->CFG1 = hw_cfg->CFG1;
    hw->ISMP = hw_cfg->ISMP;
#endif
	irq_enable(sdh_hw->irq_num);
	os_free(sdhost->cfg_backup);	
    sdh_hw->dsleep = 0;
    os_mutex_unlock(&sdhost->bp_resume_lock);
    os_mutex_unlock(&sdhost->lock);
	return 0;
}
#endif



void hgsdh_attach(uint32 dev_id, struct hgsdh *sdhost)
{
    sdhost->dev.open                  = sdh_open;
    sdhost->dev.close                 = sdh_close;
    sdhost->dev.iocfg                 = sdh_cfg;
    sdhost->dev.cmd                   = sdh_cmd;
    sdhost->dev.write                 = sdh_write;
    sdhost->dev.read                  = sdh_read;   
    sdhost->dev.complete              = sdh_complete;   

    memset(&sdhost->dev.dat_sema,0,sizeof(sdhost->dev.dat_sema));
#ifdef CONFIG_SLEEP
    sdhost->dev.suspend               = sdh_suspend;
    sdhost->dev.resume                = sdh_resume;	
    os_mutex_init(&sdhost->dev.bp_suspend_lock);
    os_mutex_init(&sdhost->dev.bp_resume_lock);
#endif
    os_mutex_init(&sdhost->dev.lock);
    irq_disable(sdhost->irq_num);
    dev_register(dev_id, (struct dev_obj *)sdhost); 
}



