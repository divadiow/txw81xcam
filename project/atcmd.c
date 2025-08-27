#include "test_demo/test_demo.h"
#include "sys_config.h"

int32 sys_atcmd_sysdbg(const char *cmd, char *argv[], uint32 argc)
{
    extern struct system_status sys_status;
    char *arg = argv[0];
    if (argc == 2) {
        if (os_strcasecmp(arg, "heap") == 0) {
            sys_status.dbg_heap = (os_atoi(argv[1]) == 1);
        }
        if (os_strcasecmp(arg, "top") == 0) {
            sys_status.dbg_top = os_atoi(argv[1]);
        }
        if (os_strcasecmp(arg, "lmac") == 0) {
            sys_status.dbg_lmac = (os_atoi(argv[1]) == 1);
        }
        if (os_strcasecmp(arg, "irq") == 0) {
            sys_status.dbg_irq = (os_atoi(argv[1]) == 1);
        }
        return ATCMD_RESULT_OK;
    } else {
        return ATCMD_RESULT_ERR;
    }
}

static const struct hgic_atcmd static_atcmds[] = {
    { "AT+RST", sys_atcmd_reset },
    { "AT+GOTOBOOT", sys_atcmd_goto_boot },
    { "AT+SYSDBG", sys_atcmd_sysdbg },
    { "AT+LOADDEF", sys_atcmd_loaddef },
    { "AT+REBOOT_TEST", sys_wifi_atcmd_reboot_test_mode },

    /*TESTMODE ATCMD*/
    { "AT+BSS_BW", atcmd_bss_bw_hdl },
    { "AT+CCA", at_cmd_cca_hdl },
    { "AT+CFG_RX_AGC", at_cmd_cfg_rx_agc_hdl },
    { "AT+CLR_RX_CNT", atcmd_rx_cnt_clr_hdl },
    { "AT+DBG_LEVEL", atcmd_dbg_level_hdl },
    { "AT+EDCA_AIFS", atcmd_edca_aifs_hdl },
    { "AT+EDCA_CW", atcmd_edca_cw_hdl },
    { "AT+EDCA_TXOP", atcmd_edca_txop_hdl },
    { "AT+TX_FRM_TYPE", atcmd_tx_frm_type_hdl },
    { "AT+TX_GAIN", atcmd_tx_gain_hdl },
    { "AT+GET_RX_CNT", atcmd_rx_cnt_get_hdl },
    { "AT+LO_FREQ", atcmd_lo_freq_hdl },
    { "AT+MAC_ADDR", atcmd_mac_addr_hdl },
    { "AT+PRINT_PERIOD", atcmd_lmac_print_period_hdl },
    { "AT+REG_RD", atcmd_reg_rd_hdl },
    { "AT+REG_WT", atcmd_reg_wt_hdl },
    { "AT+SET_XOSC", atcmd_set_xosc_hdl },
    { "AT+SET_TX_DPD_GAIN", atcmd_set_tx_dpd_gain_hdl },
    { "AT+TEST_START", atcmd_test_start_hdl },
    { "AT+TEST_ADDR", atcmd_test_addr_hdl },
    { "AT+TEMP_EN", atcmd_temp_en_hdl },
    { "AT+TX_CONT", atcmd_tx_cont_hdl },
    { "AT+TX_DELAY", atcmd_tx_delay_hdl },
    { "AT+TX_MCS", atcmd_tx_mcs_hdl },
    { "AT+TX_RF_PWR", atcmd_tx_rf_pwr_hdl },
    { "AT+TX_PHA_AMP", atcmd_tx_pha_amp_hdl },
    { "AT+TX_START", atcmd_tx_start_hdl },
    { "AT+TX_STEP", atcmd_tx_step_hdl },
    { "AT+TX_TRIG", atcmd_tx_trig_hdl },
    { "AT+TX_TYPE", atcmd_tx_type_hdl },
    { "AT+WAVE_DUMP", atcmd_wave_dump_hdl },
    { "AT+MAX_TXCNT", atcmd_max_txcnt_hdl },
    { "AT+PCF_EN", atcmd_pcf_en_hdl },
    { "AT+PCF_CHN", atcmd_pcf_chn_hdl },
#ifdef CONFIG_SLEEP
    { "AT+SLEEP_DBG", atcmd_sleep_dbg_hdl },
    { "AT+SLEEP_ALG", atcmd_sleep_alg_hdl },
    { "AT+SLEEP", atcmd_sleep_hdl },
    { "AT+DTIM", atcmd_dtim_hdl },
#endif
    { "AT+CCA_CERT", atcmd_set_cca_cert_hdl },
    { "AT+SRRC", atcmd_set_srrc_hdl},

    { "AT+JTAG", sys_atcmd_jtag },
    { "AT+SYSCFG", sys_syscfg_dump_hdl },
    { "AT+FWUPG", xmodem_fwupgrade_hdl },
    { "AT+SSID", sys_wifi_atcmd_set_ssid},
    { "AT+KEY", sys_wifi_atcmd_set_key },
    { "AT+ENCRYPT", sys_wifi_atcmd_set_encrypt },
    { "AT+WIFIMODE", sys_wifi_atcmd_set_wifimode },
    { "AT+CHANNEL", sys_wifi_atcmd_set_channel },
    { "AT+APHIDE", sys_wifi_atcmd_aphide },
    { "AT+SCAN", sys_wifi_atcmd_scan },
    { "AT+PAIR", sys_wifi_atcmd_pair },
    { "AT+HWMODE", sys_wifi_atcmd_hwmode },
    { "AT+BLENC", sys_ble_atcmd_blenc },
    //{ "AT+PING", sys_atcmd_ping},
    //{ "AT+ICMPMNTR", sys_atcmd_icmp_mntr},
    //{ "AT+IPERF2", sys_atcmd_iperf2},
    {"AT+SAVE_AUDIO",demo_atcmd_save_audio},
    {"AT+SAVE_PHOTO",demo_atcmd_save_photo},
    {"AT+SAVE_AVI",demo_atcmd_save_avi},
    {"AT+SAVE_H264",demo_atcmd_save_h264},
    {"AT+OTA",demo_atcmd_sd_ota},
	{"AT+PLAY_AUDIO",demon_atcmd_play_audio},
	//{"AT+RSO",demo_atcmd_select_resolution},
    { "AT+OSD", demo_atcmd_save_osd},
	{"AT+PLAY_MP3",demon_atcmd_play_mp3},

#if BLE_METER_TEST_EN    
    {"AT+BLE_START", atcmd_ble_start_hdl},
    {"AT+BLE_TX", atcmd_ble_tx_hdl},
    {"AT+BLE_RX", atcmd_ble_rx_hdl},
    {"AT+BLE_TX_DELAY", atcmd_ble_tx_delay_hdl},
    {"AT+BLE_CLR_RX_CNT", atcmd_ble_rx_cnt_clr_hdl},
    {"AT+BLE_GET_RX_CNT", atcmd_ble_rx_cnt_get_hdl},
    {"AT+BLE_RX_TIMEOUT", atcmd_ble_rx_timeout_hdl},
    {"AT+BLE_CHAN", atcmd_ble_chan_hdl},
    {"AT+BLE_TX_GAIN", atcmd_ble_tx_gain_hdl},
    {"AT+WRITE_BLE_TX_GAIN", atcmd_write_ble_tx_gain_hdl},
    {"AT+IO_TEST", atcmd_io_test_hdl},
    {"AT+GET_IO_TEST_RES", atcmd_io_test_res_get_hdl},
#endif
};

__init void sys_atcmd_init(void)
{
    struct atcmd_settings setting;
    os_memset(&setting, 0, sizeof(setting));
    setting.args_count = ATCMD_ARGS_COUNT;
    setting.printbuf_size = ATCMD_PRINT_BUF_SIZE;
    setting.static_atcmds = static_atcmds;
    setting.static_cmdcnt = ARRAY_SIZE(static_atcmds);
    atcmd_uart_init(ATCMD_UARTDEV, 115200, 5, &setting);
}

