typedef unsigned char u8;
typedef unsigned int u32;
typedef signed int s32;

#define REG32(a) (*(volatile u32 *)(a))
#define SYSCTRL_BASE 0x40020000u
#define GPIOA_BASE   0x40020A00u
#define UART0_BASE   0x40004000u
#define PMU_BASE     0x40018000u

#define SYS_KEY          REG32(SYSCTRL_BASE + 0x000u)
#define SYS_CLK_CON2     REG32(SYSCTRL_BASE + 0x04Cu)
#define SYS_MODE_REG     REG32(SYSCTRL_BASE + 0x0C4u)
#define SYS_IOFUNCINCON4 REG32(SYSCTRL_BASE + 0x150u)
#define SYS_CHIP_ID      REG32(SYSCTRL_BASE + 0x05Cu)

#define GPIOA_MODE       REG32(GPIOA_BASE + 0x000u)
#define GPIOA_AIOEN      REG32(GPIOA_BASE + 0x050u)
#define GPIOA_IOFUNCOUT2 REG32(GPIOA_BASE + 0x07Cu)

#define UART0_CON        REG32(UART0_BASE + 0x00u)
#define UART0_BAUD       REG32(UART0_BASE + 0x04u)
#define UART0_DATA       REG32(UART0_BASE + 0x08u)
#define UART0_STA        REG32(UART0_BASE + 0x0Cu)
#define UART0_DMACON     REG32(UART0_BASE + 0x10u)
#define UART0_DMASTA     REG32(UART0_BASE + 0x14u)
#define UART0_RSTADR     REG32(UART0_BASE + 0x18u)
#define UART0_TSTADR     REG32(UART0_BASE + 0x1Cu)
#define UART0_RDMALEN    REG32(UART0_BASE + 0x20u)
#define UART0_TDMALEN    REG32(UART0_BASE + 0x24u)
#define UART0_TOCON      REG32(UART0_BASE + 0x28u)
#define UART0_RS485_CON  REG32(UART0_BASE + 0x30u)
#define UART0_RS485_DET  REG32(UART0_BASE + 0x34u)
#define UART0_RS485_TAT  REG32(UART0_BASE + 0x38u)

#define PMUCON7          REG32(PMU_BASE + 0x34u)

#define UART_STA_TX_BUF_EMPTY (1u << 0)
#define UART_STA_RX_NOT_EMPTY (1u << 1)
#define UART_STA_TX_COMPLETE  (1u << 12)

#define SYSCTRL_UNLOCK_KEY 0x3fac87e4u
#define SYSCTRL_LOCK_KEY   0xc053781bu
#define GPIO_OUT_UART0_TX  14u
#define HG_APB0_PT_UART0   15u

extern void sysctrl_cmu_init(void);
extern u32 peripheral_clock_get(u32 peripheral);
extern void system_goto_boot(void);
extern u32 get_chip_pack(void);
extern u32 get_bios_id(void);
extern u32 sysctrl_efuse_get_module_type(void);

/* Used by libsysctrl's early QSPI protection path. */
u32 G_WRITE_PROT_L;
u32 G_WRITE_PROT_U;

/* Required by Taixin's image format. gcc_csky.ld places SYS_PARAM immediately
   after the 0x180-byte vector table and BinScript reads this length from image
   offset 0x180. */
#define SYS_FACTORY_PARAM_SIZE 2048u
const unsigned short sys_factory_param[SYS_FACTORY_PARAM_SIZE / 2u]
    __attribute__((section("SYS_PARAM"), used)) = { SYS_FACTORY_PARAM_SIZE };

static u32 g_pclk;
static volatile u32 g_nonce;
static volatile u32 g_armed;

u32 disable_irq(void)
{
    u32 old;
    __asm__ volatile("mfcr %0, cr<0, 0>\n\tpsrclr ie" : "=r"(old) :: "memory");
    return old & 0x40u;
}

void enable_irq(u32 old)
{
    if (old) {
        __asm__ volatile("psrset ie" ::: "memory");
    }
}

s32 hg_qspi_flash_protect(void *hw, u32 wp_en)
{
    (void)hw;
    (void)wp_en;
    return 0;
}

static void sys_unlock(void) { SYS_KEY = SYSCTRL_UNLOCK_KEY; }
static void sys_lock(void) { SYS_KEY = SYSCTRL_LOCK_KEY; }

static void uart0_pinmux_pa8_pa9(void)
{
    u32 v;
    sys_unlock();

    GPIOA_AIOEN &= ~((1u << 8) | (1u << 9));

    /* PA8 = UART0 TX. IOFUNCOUTCON2 controls PA8..PA11. */
    v = GPIOA_IOFUNCOUT2;
    v = (v & ~0x000000ffu) | GPIO_OUT_UART0_TX;
    GPIOA_IOFUNCOUT2 = v;

    v = GPIOA_MODE;
    v = (v & ~(3u << 16)) | (1u << 16);
    /* PA9 = UART0 RX/input. */
    v &= ~(3u << 18);
    GPIOA_MODE = v;

    /* UART0 input selector #17 is IOFUNCINCON4[15:8]. PA9 index = 9. */
    v = SYS_IOFUNCINCON4;
    v = (v & ~(0xffu << 8)) | (9u << 8);
    SYS_IOFUNCINCON4 = v;

    SYS_CLK_CON2 |= (1u << 10);
    sys_lock();
}

static void uart0_init_115200(void)
{
    u32 div;
    uart0_pinmux_pa8_pa9();

    g_pclk = peripheral_clock_get(HG_APB0_PT_UART0);
    if (g_pclk < 1000000u || g_pclk > 500000000u) {
        g_pclk = 60000000u;
    }
    div = g_pclk / 115200u;
    if (div == 0) div = 1;

    UART0_CON       = 0;
    UART0_BAUD      = 0;
    UART0_STA       = 0xffffffffu;
    UART0_DMACON    = 0;
    UART0_DMASTA    = 0xffffffffu;
    UART0_RSTADR    = 0;
    UART0_TSTADR    = 0;
    UART0_RDMALEN   = 0;
    UART0_TDMALEN   = 0;
    UART0_TOCON     = 0;
    UART0_RS485_CON = 0;
    UART0_RS485_DET = 0;
    UART0_RS485_TAT = 0;
    UART0_BAUD      = div - 1u;
    UART0_CON       = 1u;
}

static void txc(u8 c)
{
    u32 guard = 0x400000u;
    while (!(UART0_STA & UART_STA_TX_BUF_EMPTY) && guard) --guard;
    UART0_DATA = (u32)c;
}

static void txs(const char *s)
{
    while (*s) txc((u8)*s++);
}

static void txnl(void) { txc('\r'); txc('\n'); }

static void txhex(u32 v)
{
    int i;
    for (i = 7; i >= 0; --i) {
        u32 n = (v >> (i * 4)) & 15u;
        txc((u8)(n < 10u ? ('0' + n) : ('A' + n - 10u)));
    }
}

static int hexval(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static u32 parsehex(const char *p, int *ok)
{
    u32 v = 0;
    int n = 0, h;
    while ((h = hexval((u8)*p)) >= 0) {
        v = (v << 4) | (u32)h;
        ++p;
        ++n;
    }
    *ok = n > 0;
    return v;
}

static int streq(const char *a, const char *b)
{
    while (*a && *b && *a == *b) { ++a; ++b; }
    return *a == 0 && *b == 0;
}

static void snapshot(void)
{
    txs("TXWDIAG SNAP mode=0x"); txhex(SYS_MODE_REG);
    txs(" pmu7=0x"); txhex(PMUCON7);
    txs(" chipreg=0x"); txhex(SYS_CHIP_ID);
    txs(" pack=0x"); txhex(get_chip_pack());
    txs(" bios=0x"); txhex(get_bios_id());
    txs(" module=0x"); txhex(sysctrl_efuse_get_module_type());
    txs(" pclk=0x"); txhex(g_pclk);
    txnl();
}

static void wait_tx_complete(void)
{
    u32 guard = 0x800000u;
    while (!(UART0_STA & UART_STA_TX_COMPLETE) && guard) --guard;
    for (guard = 0; guard < 200000u; ++guard) __asm__ volatile("nop");
}

static void process(char *s)
{
    int ok;
    u32 n;

    if (streq(s, "AT+DIAG=PING")) {
        txs("TXWDIAG PONG v=0.2 target=TXW817-810 PA8/PA9"); txnl();
        return;
    }
    if (streq(s, "AT+DIAG=SNAPSHOT")) {
        snapshot();
        return;
    }
    if (s[0]=='A' && s[1]=='T' && s[2]=='+' && s[3]=='D' && s[4]=='I' &&
        s[5]=='A' && s[6]=='G' && s[7]=='=' && s[8]=='A' && s[9]=='R' &&
        s[10]=='M' && s[11]==',') {
        n = parsehex(s + 12, &ok);
        if (ok) {
            g_nonce = n;
            g_armed = 1;
            snapshot();
            txs("TXWDIAG ARMED 0x"); txhex(n); txnl();
        } else {
            txs("TXWDIAG ERROR BAD_NONCE"); txnl();
        }
        return;
    }
    if (s[0]=='A' && s[1]=='T' && s[2]=='+' && s[3]=='D' && s[4]=='I' &&
        s[5]=='A' && s[6]=='G' && s[7]=='=' && s[8]=='E' && s[9]=='X' &&
        s[10]=='E' && s[11]=='C' && s[12]==',') {
        n = parsehex(s + 13, &ok);
        if (!ok || !g_armed || n != g_nonce) {
            txs("TXWDIAG ERROR NOT_ARMED"); txnl();
            return;
        }
        g_armed = 0;
        txs("TXWDIAG ROMGO 0x"); txhex(n); txnl();
        wait_tx_complete();
        system_goto_boot();
        for (;;) {}
    }
    txs("TXWDIAG ERROR UNKNOWN"); txnl();
}

static void diagnostic_loop(void)
{
    char line[96];
    u32 used = 0;

    txs("TXWDIAG READY v=0.2 target=TXW817-810 PA8/PA9 115200 pclk=0x");
    txhex(g_pclk);
    txs(" mode=0x"); txhex(SYS_MODE_REG);
    txnl();
    snapshot();

    for (;;) {
        if (UART0_STA & UART_STA_RX_NOT_EMPTY) {
            int c = (int)(UART0_DATA & 0xffu);
            if (c == '\r' || c == '\n') {
                if (used) {
                    line[used] = 0;
                    process(line);
                    used = 0;
                }
            } else if (used + 1u < sizeof(line)) {
                line[used++] = (char)c;
            } else {
                used = 0;
            }
        }
    }
}

/* These two hooks are called by Taixin's genuine txw81x_startup.o. */
void SystemInit(void)
{
    SYS_KEY = 0xb3a2164cu;
    SYS_KEY = SYSCTRL_UNLOCK_KEY;
    sysctrl_cmu_init();
    uart0_init_115200();
}

void pre_main(void)
{
    diagnostic_loop();
    for (;;) {}
}

void Default_Handler(void) { for (;;) {} }
void trap0_handler(void) { Default_Handler(); }
void tspend_handler(void) { Default_Handler(); }
#define IRQ_STUB(n) void n(void) { Default_Handler(); }
IRQ_STUB(ADKEY_IRQHandler)
IRQ_STUB(AUDIO_ADC_IRQHandler)
IRQ_STUB(AUDIO_DAC_IRQHandler)
IRQ_STUB(AUDIO_VAD_HS_ALAW_IRQHandler)
IRQ_STUB(CMP_IRQHandler)
IRQ_STUB(CORET_IRQHandler)
IRQ_STUB(CRC_IRQHandler)
IRQ_STUB(DVP_IRQHandler)
IRQ_STUB(GFSK_IRQHandler)
IRQ_STUB(GMAC_IRQHandler)
IRQ_STUB(GPIO_IRQHandler)
IRQ_STUB(IIS0_IRQHandler)
IRQ_STUB(IIS1_IRQHandler)
IRQ_STUB(JPG_IRQHandler)
IRQ_STUB(LCD_IRQHandler)
IRQ_STUB(LED_TMR_IRQHandler)
IRQ_STUB(LMAC_IRQHandler)
IRQ_STUB(LVD_IRQHandler)
IRQ_STUB(M2M0_IRQHandler)
IRQ_STUB(M2M1_IRQHandler)
IRQ_STUB(OSPI_IRQHandler)
IRQ_STUB(PDM_IRQHandler)
IRQ_STUB(PDWKPND_IRQHandler)
IRQ_STUB(PD_TMR_IRQHandler)
IRQ_STUB(PRC_IRQHandler)
IRQ_STUB(QSPI_IRQHandler)
IRQ_STUB(SCALE1_IRQHandler)
IRQ_STUB(SCALE2_IRQHandler)
IRQ_STUB(SCALE3_IRQHandler)
IRQ_STUB(SDHOST_IRQHandler)
IRQ_STUB(SDIO_IRQHandler)
IRQ_STUB(SDIO_RST_IRQHandler)
IRQ_STUB(SPI0_IRQHandler)
IRQ_STUB(SPI1_IRQHandler)
IRQ_STUB(SPI2_IRQHandler)
IRQ_STUB(STMR_IRQHandler)
IRQ_STUB(SYSAES_IRQHandler)
IRQ_STUB(SYS_ERR_IRQHandler)
IRQ_STUB(TIM0_IRQHandler)
IRQ_STUB(TIM1_IRQHandler)
IRQ_STUB(TIM2_IRQHandler)
IRQ_STUB(TIM3_IRQHandler)
IRQ_STUB(UART0_IRQHandler)
IRQ_STUB(UART1_IRQHandler)
IRQ_STUB(UART45_IRQHandler)
IRQ_STUB(USB20DMA_IRQHandler)
IRQ_STUB(USB20MC_IRQHandler)
IRQ_STUB(USB20PHY_RTC_IRQHandler)
IRQ_STUB(VPP_IRQHandler)
IRQ_STUB(WDT_IRQHandler)
IRQ_STUB(WKPND_IRQHandler)
