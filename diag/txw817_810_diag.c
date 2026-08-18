typedef unsigned int u32;

extern int putchar(int c);
extern int getchar(void);
extern void system_goto_boot(void);
extern u32 sysctrl_get_chip_id(void);
extern u32 get_chip_pack(void);
extern u32 get_bios_id(void);
extern u32 sysctrl_efuse_get_module_type(void);

#define MODE_REG (*(volatile u32 *)0x400200C4u)

static volatile u32 g_nonce;
static volatile u32 g_armed;

static void txc(int c) { (void)putchar(c); }
static void txs(const char *s) { while (*s) txc((unsigned char)*s++); }
static void txnl(void) { txc('\r'); txc('\n'); }

static void txhex(u32 v)
{
    int i;
    for (i = 7; i >= 0; --i) {
        u32 n = (v >> (i * 4)) & 15u;
        txc((int)(n < 10u ? ('0' + n) : ('A' + n - 10u)));
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
    int n = 0;
    int h;
    while ((h = hexval((unsigned char)*p)) >= 0) {
        v = (v << 4) | (u32)h;
        ++p;
        ++n;
    }
    *ok = n > 0;
    return v;
}

static int streq(const char *a, const char *b)
{
    while (*a && *b && *a == *b) {
        ++a;
        ++b;
    }
    return *a == 0 && *b == 0;
}

static void delay_cycles(volatile u32 n)
{
    while (n--) __asm__ volatile("nop");
}

static void snapshot(void)
{
    u32 mode = MODE_REG;
    txs("TXWDIAG SNAPSHOT mode=0x");
    txhex(mode);
    txs(" cp="); txc((int)('0' + ((mode >> 0) & 1u)));
    txs(" ft="); txc((int)('0' + ((mode >> 1) & 1u)));
    txs(" chip=0x"); txhex(sysctrl_get_chip_id());
    txs(" pack=0x"); txhex(get_chip_pack());
    txs(" bios=0x"); txhex(get_bios_id());
    txs(" module=0x"); txhex(sysctrl_efuse_get_module_type());
    txnl();
}

static void process(char *s)
{
    int ok;
    u32 n;

    if (streq(s, "AT+DIAG=PING")) {
        txs("TXWDIAG PONG v=0.1 target=TXW817-810");
        txnl();
        return;
    }

    if (streq(s, "AT+DIAG=SNAPSHOT")) {
        snapshot();
        return;
    }

    if (streq(s, "HELP")) {
        txs("TXWDIAG CMDS PING SNAPSHOT ARM,<nonce> EXEC,<nonce>");
        txnl();
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
            txs("TXWDIAG ARMED nonce=");
            txhex(n);
            txnl();
        } else {
            txs("TXWDIAG ERR bad_nonce");
            txnl();
        }
        return;
    }

    if (s[0]=='A' && s[1]=='T' && s[2]=='+' && s[3]=='D' && s[4]=='I' &&
        s[5]=='A' && s[6]=='G' && s[7]=='=' && s[8]=='E' && s[9]=='X' &&
        s[10]=='E' && s[11]=='C' && s[12]==',') {
        n = parsehex(s + 13, &ok);
        if (!ok || !g_armed || n != g_nonce) {
            txs("TXWDIAG ERR not_armed");
            txnl();
            return;
        }
        g_armed = 0;
        txs("TXWDIAG ROMGO nonce=");
        txhex(n);
        txnl();
        delay_cycles(1500000u);
        system_goto_boot();
        for (;;) {}
    }

    txs("TXWDIAG ERR unknown");
    txnl();
}

int main(void)
{
    char line[96];
    u32 used = 0;
    int c;

    txs("TXWDIAG READY v=0.1 target=TXW817-810 mode=0x");
    txhex(MODE_REG);
    txnl();

    for (;;) {
        c = getchar();
        if (c < 0) continue;
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
