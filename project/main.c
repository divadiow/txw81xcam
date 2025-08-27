#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "osal/irq.h"
#include "osal/task.h"
#include "osal/sleep.h"
#include "osal/timer.h"
#include "osal/work.h"
#include "hal/gpio.h"
#include "hal/uart.h"
#include "lib/common/common.h"
#include "lib/common/sysevt.h"
#include "lib/syscfg/syscfg.h"
#include "lib/lmac/lmac.h"
#include "lib/skb/skbpool.h"
#include "lib/atcmd/libatcmd.h"
#include "lib/bus/xmodem/xmodem.h"
#include "lib/net/dhcpd/dhcpd.h"
#include "lib/umac/ieee80211.h"
#include "lib/umac/umac.h"
#include "lib/common/atcmd.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/tcpip.h"
#include "netif/ethernetif.h"
#include "syscfg.h"
#include "lib/net/eloop/eloop.h"
#include "speedTest/speedTest.h"

#include "update/ota.h"
#include "dhcpd_eloop/dhcpd_eloop.h"
#include "sta_interface/sta_interface.h"
#include "ota.h"
#include "lib/heap/sysheap.h"
#include "spook.h"
#include "remote_control.h"
#include "lib/sdhost/sdhost.h"
#include "lib/ble/ble_def.h"
#include "lib/ble/ble_demo.h"
#include "lib/ble/uble.h"
#include "lib/common/dsleepdata.h"
#include "dns_eloop/dns_eloop.h"

#include "custom_mem/custom_mem.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "audio_app/audio_adc.h"
#include "audio_app/audio_dac.h"
#include "stream_frame.h"
#include "test_demo/test_demo.h"
#include "atcmd.c"
#include "keyWork.h"
#include "flashdisk/flashdisk.h"
#include "osal_file.h"

#if MP3_EN
#include "mp3/mp3_decode.h"
#include "third_audio/libmad/global.h"
#endif

#if LCD_EN
#if LVGL_STREAM_ENABLE == 1
	#include "app_lcd/app_lcd.h"
	#include "decode/yuv_stream.h"
#else

#endif
#endif

#if RTT_USB_EN
#include "rtthread.h"
#endif

extern uint8_t get_psram_status();

struct system_status sys_status;
extern uint32 srampool_start;
extern uint32 srampool_end;
static struct os_work main_wk;
uint8_t mac_adr[6];
void *g_ops = NULL;

uint8_t qc_mode = 0;
void delay_us(uint32 n);
int32 wifi_qc_scan(uint8 ifidx);

struct hgic_atcmd_normal {
    struct atcmd_dataif dataif;
};


struct hgic_atcmd_normal *atcmd_uart_normal = NULL;



#if NET_PAIR
void set_pair_mode(uint8_t enable)
{
    os_printf("%s enable:%d\n",__FUNCTION__,enable);
    sys_cfgs.net_pair_switch = enable;
    ieee80211_pairing(sys_cfgs.wifi_mode, sys_cfgs.net_pair_switch);
}

uint8_t get_net_pair_status()
{
    return sys_cfgs.net_pair_switch;
}

#endif


#ifdef CONFIG_UMAC4
int32 sys_wifi_event(uint8 ifidx, uint16 evt, uint32 param1, uint32 param2)
{
    switch (evt) {
        case IEEE80211_EVENT_SCAN_START:
            os_printf("inteface%d: start scanning ...\r\n", ifidx);
            break;
        case IEEE80211_EVENT_SCAN_DONE:
            os_printf("inteface%d: scan done!\r\n", ifidx);
            break;
        case IEEE80211_EVENT_CONNECT_START:
            os_printf("inteface%d: start connecting ...\r\n", ifidx);
            break;
        case IEEE80211_EVENT_CONNECTED:
            os_printf("inteface%d: sta "MACSTR" connected\r\n", ifidx, MAC2STR((uint8 *)param1));
            break;
        case IEEE80211_EVENT_DISCONNECTED:
            os_printf("inteface%d: sta "MACSTR" disconnected\r\n", ifidx, MAC2STR((uint8 *)param1));
            break;
        case IEEE80211_EVENT_RSSI:
            //os_printf("inteface%d rssi: %d\r\n", ifidx, param1);
            break;
        case IEEE80211_EVENT_NEW_BSS:
            os_printf("inteface%d find new bss: "MACSTR"-%s\r\n", ifidx, MAC2STR((uint8 *)param1), (uint8 *)param2);
            break;
		#if 0
        case IEEE80211_EVENT_PRE_AUTH:
			os_printf("inteface%d pre auth, pair_bssid:"MACSTR",new_bssid: "MACSTR"\r\n", ifidx, MAC2STR((uint8 *)sys_cfgs.bssid), MAC2STR((uint8 *)param1));
			if(memcmp(sys_cfgs.bssid,(uint8 *)param1,6)!=0)
			{
				return RET_ERR;
			}
            break;
		#endif
        //这里不一定配对成功,只是代表收到对方消息,但自己的消息对方不一定收到
        //这里在AP模式不会自动关闭配对,只有STA模式收到配对信息后,就会关闭配对,去连接网络
        case IEEE80211_EVENT_PAIR_SUCCESS:
			#if NET_PAIR
            sys_status.pair_success = 1;
			os_printf("inteface%d pair success, bssid: "MACSTR"\r\n", ifidx, MAC2STR((uint8 *)param1));
            if(WIFI_MODE_STA == ifidx)
            {
                set_pair_mode(0);
                sys_event_new(SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_PAIR_DONE), 1);
            }
			#if 0
			else if(WIFI_MODE_AP == ifidx)
			{
				os_memcpy(sys_cfgs.bssid,(uint8 *)param1,6);
				syscfg_save();
			}
			#endif
			#endif
            break;
        //配对结束
        case IEEE80211_EVENT_PAIR_DONE:
            os_printf("inteface%d pair done, bssid: "MACSTR"\r\n", ifidx, MAC2STR((uint8 *)param1));
            sys_event_new(SYS_EVENT(SYS_EVENT_WIFI, SYSEVT_WIFI_PAIR_DONE), param2);
            break;
        default:
            break;
    }
    return RET_OK;
}
#endif

__init static void sys_cfg_load()
{
    
    struct sys_config *sys_cfgs_tmp = (struct sys_config*)malloc(sizeof(struct sys_config));
    if(sys_cfgs_tmp)
    {
        memcpy(sys_cfgs_tmp,&sys_cfgs,sizeof(struct sys_config));
    }
    if (syscfg_init(&sys_cfgs, sizeof(sys_cfgs)) == RET_OK) {
        os_printf("old cfg_ver:%d\n",sys_cfgs.cfg_ver);
		if(sys_cfgs.cfg_ver == CFG_VERSION_NUM){
			goto sys_cfg_load_end;
		}
		sys_cfgs.cfg_ver = CFG_VERSION_NUM;  //修改version
    }
    if(sys_cfgs_tmp)
    {
        memcpy(&sys_cfgs,sys_cfgs_tmp,sizeof(struct sys_config));
    }
    os_printf("use default params(%x).\r\n",sys_cfgs.cfg_ver);
    syscfg_set_default_val();
    syscfg_save();

sys_cfg_load_end:
    if(sys_cfgs_tmp)
    {
        free(sys_cfgs_tmp);
    }
    else
    {
        os_printf("%s:%d err,malloc sys_config err\n",__FUNCTION__,__LINE__);
    }
}

void user_io_preconfig();
void user_protocol();


__weak void user_io_preconfig()
{

}

__weak void user_protocol()
{
    spook_init();
}

__weak void user_hardware_config()
{

}


__weak uint8_t set_qc_mode_io()
{
	return 255;
}

void sys_mode_confirm()
{
	if(qc_mode){
		sys_cfgs.wifi_mode = WIFI_MODE_STA;
        extern void qc_wifi_config();
        qc_wifi_config();
	}else{
		sys_cfgs.wifi_mode = sys_cfgs.default_wifi_mode;
	}	
}

uint8_t qc_ret = 0;
uint8_t mode_io_1 = 0;
uint8_t io_dect = 0;


//period 100ms
__weak int32 is_qc_mode(uint8_t mode_io)
{
	int32 io_direct,io_direct1,io_direct2,io_direct3;
	io_direct = 0;
	gpio_iomap_input(mode_io, GPIO_DIR_INPUT);
	
	io_direct1 = gpio_get_val(mode_io);
	delay_us(5000);       //delay 5ms
	io_direct2 = gpio_get_val(mode_io);
	delay_us(5000);       //delay 5ms
	io_direct3 = gpio_get_val(mode_io);
	
	if((io_direct1 == io_direct2)&&(io_direct3 == io_direct2)){
		io_direct = io_direct2;
		delay_us(45000);  //delay 45ms
	}else if((io_direct1 == io_direct2)&&(io_direct3 != io_direct2)){
		io_direct = io_direct1;
		delay_us(40000);  //delay 40ms		
	}else if((io_direct1 != io_direct2)&&(io_direct3 == io_direct2)){
		io_direct = io_direct3;
		delay_us(50000);  //delay 50ms		
	}
	
	if(gpio_get_val(mode_io) == io_direct){
		return 0;
	}
	
	delay_us(50000);      //delay 50ms
	if(gpio_get_val(mode_io) != io_direct){		
		return 0;
	}	

	delay_us(50000);      //delay 50ms
	if(gpio_get_val(mode_io) == io_direct){
		return 0;
	}

	delay_us(50000);      //delay 50ms
	if(gpio_get_val(mode_io) != io_direct){
		return 0;
	}
	//check 200ms,all correct,return true
	return 1;
	
}

void wifi_qc_mode_inspect()
{
#if (NET_TO_QC == 1)
	uint8_t wifi_qc_mode_io;
	wifi_qc_mode_io = set_qc_mode_io();
	if(wifi_qc_mode_io != 255){
		if(is_qc_mode(wifi_qc_mode_io)){
			qc_mode = 1;
		}else{
			qc_mode = 0;
		}	
	}else{
		qc_mode = 0;
	}
#endif
}


static int32 atcmd_normal_write(struct atcmd_dataif *dataif, uint8 *buf, int32 len)
{
    int32 i = 0;
    for(i = 0;i < len;i++){
        _os_printf("%c",buf[i]);
    }
    return 0;
}


void user_sta_add(char *addr){
	memcpy(mac_adr,addr,6);	
	os_printf("user_sta_add:%x %x %x %x %x %x\r\n",addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
}

void user_sta_del(char *addr){
	memset(mac_adr,0,6); 
	os_printf("user_sta_del:%x %x %x %x %x %x\r\n",addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
}

void sta_ps_mode_enter(uint16 aid);
void sta_ps_mode_exit(uint16 aid);
void aid_to_mac(uint16 aid, uint8* mac);

sysevt_hdl_res sysevt_wifi_event(uint32 event_id, uint32 data, uint32 priv){
    uint8 mac[6];
    switch(event_id&0xffff){
        case SYSEVT_WIFI_DISCONNECT:
			aid_to_mac((uint16)data,mac);
			user_sta_del((char *)mac);
            sys_status.wifi_connected = 0;
            break;
        case SYSEVT_WIFI_CONNECTTED:
			aid_to_mac((uint16)data,mac);
			user_sta_add((char *)mac);
#if BLE_PAIR_NET == 2
            char *ble_pair_network_succ = "network_connect_succ";
            struct lmac_ops *lops = (struct lmac_ops *)g_ops;
            struct bt_ops *bt_ops = (struct bt_ops *)lops->btops;

            os_printf("notify client: %s\r\n", ble_pair_network_succ);
            uble_gatt_notify(10, (uint8 *)ble_pair_network_succ, os_strlen(ble_pair_network_succ));
            os_sleep(1);
            ble_demo_stop(bt_ops);
            ble_set_coexist_en(bt_ops, 0, 0);
#endif
            //240327
            if (sys_cfgs.wifi_mode == WIFI_MODE_STA) {
                if(sys_cfgs.dhcpc_en){//dynamic ip, 
                    lwip_netif_set_dhcp2("w0", 1);
                }else{ //static ip
                    ip_addr_t addr; 
                    addr.addr = 0x0101A8C0 + ((data&0xff)<<24);
                    lwip_netif_set_ip2("w0", &addr, NULL, NULL);
                    os_printf("w0_ip= %x\r\n", lwip_netif_get_ip2("w0"));
                }
            }
            sys_status.wifi_connected = 1;
            {
                struct ieee80211_bss_wpadata bss_data;
                uint8_t bssid[6];
                ieee80211_conf_get_bssid(WIFI_MODE_STA,bssid);
                ieee80211_bss_get_wpadata((uint8_t*)bssid,IEEE80211_BAND_2GHZ,&bss_data);
                int channel = ieee80211_conf_get_channel(WIFI_MODE_STA);
			#if 0
				ieee80211_conf_get_ssid(sys_cfgs.wifi_mode, sys_cfgs.ssid);
			#endif
                syscfg_save_AP_msg((uint8_t*)bssid,channel,&bss_data);
            }
            break;
		case SYSEVT_WIFI_STA_DISCONNECT:
			aid_to_mac((uint16)data,mac);
			user_sta_del((char *)mac);
			break;
		case SYSEVT_WIFI_STA_CONNECTTED:
			aid_to_mac((uint16)data,mac);
			user_sta_add((char *)mac);
			break;			
		case SYSEVT_WIFI_STA_PS_START:
			//sta_ps_mode_enter((uint16)data);
			break;
		case SYSEVT_WIFI_STA_PS_END:	
			//sta_ps_mode_exit((uint16)data);
			break;
		case SYSEVT_WIFI_SCAN_DONE:
			os_printf("scan down.......\r\n");
			break;
        //wifi配对完成事件,暂时没有响应
        case SYSEVT_WIFI_PAIR_DONE:
            if (sys_status.pair_success) {
                ieee80211_conf_get_ssid(sys_cfgs.wifi_mode, sys_cfgs.ssid);
                ieee80211_conf_get_psk(sys_cfgs.wifi_mode, sys_cfgs.psk);
                sys_cfgs.key_mgmt = ieee80211_conf_get_keymgmt(sys_cfgs.wifi_mode);
                
                if((int32)data == 1 && sys_cfgs.wifi_mode == WIFI_MODE_STA){
                    os_printf(KERN_NOTICE"wifi pair done, ngo role AP!\r\n");
                    sys_cfgs.wifi_mode = WIFI_MODE_AP;
                    ieee80211_iface_stop(WIFI_MODE_STA);
                    wificfg_flush(WIFI_MODE_AP);
                    ieee80211_iface_start(WIFI_MODE_AP);
                }
            
                if((int32)data == -1 && sys_cfgs.wifi_mode == WIFI_MODE_AP){
                    os_printf(KERN_NOTICE"wifi pair done, ngo role STA!\r\n");
                    sys_cfgs.wifi_mode = WIFI_MODE_STA;
                    ieee80211_iface_stop(WIFI_MODE_AP);
                    wificfg_flush(WIFI_MODE_STA);
                    ieee80211_iface_start(WIFI_MODE_STA);
                }
            
                if(sys_cfgs.wifi_mode == WIFI_MODE_STA || data){
                    syscfg_save();
                }
            }
        break;
        default:
            os_printf("no this event(%x)...\r\n",event_id);
            break;
    }
    return SYSEVT_CONTINUE;
}

sysevt_hdl_res sysevt_network_event(uint32 event_id, uint32 data, uint32 priv)
{
    struct netif *nif;

    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_NETWORK, SYSEVT_LWIP_DHCPC_DONE):
            nif = netif_find("w0");
            sys_status.dhcpc_done = 1;
#ifdef CONFIG_SLEEP
			err_t etharp_set_static(ip4_addr_t *ipaddr);
			err_t etharp_request(struct netif *netif, const ip4_addr_t *ipaddr);
			
            if(etharp_set_static(&nif->gw)){
                etharp_request(nif, &nif->gw);
            }
#endif
            os_printf("dhcp done, ip:"IPSTR", mask:"IPSTR", gw:"IPSTR"\r\n", IP2STR_N(nif->ip_addr.addr), IP2STR_N(nif->netmask.addr), IP2STR_N(nif->gw.addr));
            break;
    }
    return SYSEVT_CONTINUE;
}

sysevt_hdl_res sysevt_lte_event(uint32 event_id, uint32 data, uint32 priv)
{
    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_LTE, SYSEVT_LTE_CONNECTED):
            os_printf("lte connected\r\n");
#ifndef STATIC_RNDIS_NETDEV
            if (sys_status.wifi_connected == 0) {
                lwip_netif_set_default((struct netdev *)dev_get(HG_LTE_RNDIS_DEVID));
                lwip_netif_set_dhcp2("l0", 1);
                os_printf("start dhcp client on l0 ...\r\n");
            }
#endif
            break;
    }
    return SYSEVT_CONTINUE;
}

sysevt_hdl_res sysevt_ble_event(uint32 event_id, uint32 data, uint32 priv)
{
    switch (event_id) {
        case SYS_EVENT(SYS_EVENT_BLE, SYSEVT_BLE_NETWORK_CONFIGURED):
#if BLE_SUPPORT 
    		wpa_passphrase(sys_cfgs.ssid, sys_cfgs.passwd, sys_cfgs.psk);
    		ieee80211_conf_set_ssid(WIFI_MODE_STA, sys_cfgs.ssid);
    		ieee80211_conf_set_psk(WIFI_MODE_STA, sys_cfgs.psk);
    		ieee80211_conf_set_keymgmt(WIFI_MODE_STA, sys_cfgs.key_mgmt);
    		sys_cfgs.wifi_mode = WIFI_MODE_STA;
    		syscfg_save();

    		ieee80211_iface_stop(WIFI_MODE_AP);
            ieee80211_iface_stop(WIFI_MODE_STA);
    		wificfg_flush(WIFI_MODE_STA);
    		netdev_set_wifi_mode((struct netdev *)dev_get(HG_WIFI0_DEVID), WIFI_MODE_STA);
    		ieee80211_iface_start(WIFI_MODE_STA);
#if BLE_PAIR_NET == 1
			struct lmac_ops *lops = (struct lmac_ops *)g_ops;
			struct bt_ops *bt_ops = (struct bt_ops *)lops->btops;
            ble_demo_stop(bt_ops);
#endif
#endif
            break;
    }
    return SYSEVT_CONTINUE;
}

__init static void sys_wifi_start_acs(void *ops){
	int32 ret;
	struct lmac_acs_ctl acs_ctl;
    if(sys_cfgs.wifi_mode == WIFI_MODE_AP){
        if(sys_cfgs.channel == 0) {
            acs_ctl.enable     = 1;
            acs_ctl.scan_ms    = WIFI_ACS_SCAN_TIME;;
            acs_ctl.chn_bitmap = WIFI_ACS_CHAN_LISTS;
            
            ret = lmac_start_acs(ops, &acs_ctl, 1);  //阻塞式扫描
            if(ret != RET_ERR) {
                sys_cfgs.channel = ret;
            }
        }
    }

}

__init static void sys_wifi_parameter_init(void *ops)
{
    lmac_set_rf_pwr_level(ops, WIFI_RF_PWR_LEVEL);
#if WIFI_FEM_CHIP != LMAC_FEM_NONE
    lmac_set_fem(ops, WIFI_FEM_CHIP);   //初始化FEM之后，不能进行RF档位选择！
#endif
#if WIFI_MODULE_80211W_EN
    lmac_bgn_module_80211w_init(ops);
#endif
    lmac_set_rts(ops, WIFI_RTS_THRESHOLD);
    lmac_set_retry_cnt(ops, WIFI_TX_MAX_RETRY, WIFI_RTS_MAX_RETRY);
    lmac_set_txpower(ops, WIFI_TX_MAX_POWER);
    lmac_set_supp_rate(ops, WIFI_TX_SUPP_RATE);
    lmac_set_max_sta_cnt(ops, WIFI_MAX_STA_CNT);
    lmac_set_mcast_dup_txcnt(ops, WIFI_MULICAST_RETRY);
    lmac_set_max_ps_frame(ops, WIFI_MAX_PS_CNT);
    lmac_set_tx_duty_cycle(ops, WIFI_TX_DUTY_CYCLE);
#if WIFI_SSID_FILTER_EN
    lmac_set_ssid_filter(ops, sys_cfgs.ssid, strlen(sys_cfgs.ssid));
#endif
#if WIFI_PREVENT_PS_MODE_EN
    lmac_set_prevent_ps_mode(ops, WIFI_PREVENT_PS_MODE_EN);
#endif
#ifdef LMAC_BGN_PCF
    lmac_set_prevent_ps_mode(ops, 0);//bbm允许sta休眠
#endif

#ifdef RATE_CONTROL_SELECT
    //lmac_rate_ctrl_mcs_mask(ops, 0);
    lmac_rate_ctrl_type(ops, RATE_CONTROL_SELECT);
#endif
//使用小电流时，就不使用增大发射功率的功率表
#if (WIFI_RF_PWR_LEVEL != 1) && (WIFI_RF_PWR_LEVEL != 2)
#if (WIFI_FEM_CHIP == LMAC_FEM_GSR2701) || (WIFI_FEM_CHIP == LMAC_FEM_KCT8227D) || (WIFI_FEM_CHIP == LMAC_FEM_GSR2701_5V)
    uint8 gain_table[] = {
        96, 96, 96, 96, 88, 88, 64, 64,         // NON HT OFDM
        96, 96, 96, 88, 88, 64, 64, 64,         // HT
        64, 64, 64, 64,                         // DSSS
    };
#else
    uint8 gain_table[] = {
        125, 125, 105, 100, 85, 85, 64, 64,     // NON HT OFDM
        125, 105, 105, 85,  85, 64, 64, 64,     // HT
        80,  80,  80,  80,                      // DSSS
    };
#endif
    lmac_set_tx_modulation_gain(ops, gain_table, sizeof(gain_table));
#endif
    lmac_set_temperature_compesate_en(ops, WIFI_TEMPERATURE_COMPESATE_EN);
    lmac_set_freq_offset_track_mode(ops, WIFI_FREQ_OFFSET_TRACK_MODE);

#if WIFI_HIGH_PRIORITY_TX_MODE_EN
    struct lmac_txq_param txq_max;
    struct lmac_txq_param txq_min;

    txq_max.aifs   = 0xFF;                  //不限制aifs
    txq_max.txop   = 0xFFFF;                //不限制txop
    txq_max.cw_min = 1;                     //cwmin最大值。如果觉得冲突太厉害，可以改成3
    txq_max.cw_max = 3;                     //cwmax最大值。如果觉得冲突太厉害，可以改成7
    lmac_set_edca_max(ops, &txq_max);
    lmac_set_tx_edca_slot_time(ops, 6);     //6us是其他客户推荐的值
    lmac_set_nav_max(ops, 0);               //完全关闭NAV功能

    txq_min.aifs   = 0;
    txq_min.txop   = 100;                   //txop最小值限制为100
    txq_min.cw_min = 0;
    txq_min.cw_max = 0;
    lmac_set_edca_min(ops, &txq_min);
#endif
}

__init static void sys_wifi_test_mode_init(void *ops)
{
    uint8 default_gain_table[] = {          //gain默认值
        64, 64, 64, 64, 64, 64, 64, 64,     //OFDM
        64, 64, 64, 64, 64, 64, 64, 64,     //HT
        64, 64, 64, 64,                     //DSSS
    };

#if WIFI_FEM_CHIP != LMAC_FEM_NONE
    lmac_set_fem(ops, WIFI_FEM_CHIP);   //初始化FEM之后，不能进行RF档位选择！
#endif
    lmac_set_tx_modulation_gain(ops, default_gain_table, sizeof(default_gain_table));
}

__init static void sys_wifi_init()
{
    void *ops;
    struct lmac_init_param lparam;

#if DCDC_EN
    pmu_dcdc_open();
#endif
    void *rxbuf = (void*)os_malloc(WIFI_RX_BUFF_SIZE);
    if(!rxbuf)
    {
        os_printf("%s:%d\tmalloc size:%d fail\n",__FUNCTION__,__LINE__,WIFI_RX_BUFF_SIZE);
        return;
    }
    lparam.rxbuf      = (uint32_t)rxbuf;
    lparam.rxbuf_size = WIFI_RX_BUFF_SIZE;
    ops = lmac_bgn_init(&lparam);
#ifdef CONFIG_SLEEP
	void *bgn_dsleep_init(void *ops);
    bgn_dsleep_init(ops);
#endif

#ifdef LMAC_BGN_PCF
    lmac_set_bbm_mode_init(ops, 1); //init before ieee80211_support_txw80x
#endif

    // enter wifi test mode
    if(system_is_wifi_test_mode()) {
        sys_wifi_test_mode_init(ops);
        return;
    }
    
    lmac_set_aggcnt(ops,0);
    lmac_set_rx_aggcnt(ops,0);

#if BLE_SUPPORT
	g_ops = ops;
	ble_ll_init(ops);
#endif

#if WPA_CRYPTO_OPS
#ifdef CONFIG_SAE
    ieee80211_crypto_bignum_support();
    ieee80211_crypto_ec_support();
#endif

#ifdef CONFIG_OWE
    ieee80211_crypto_ecdh_support();
#endif
    
#ifdef CONFIG_WPS
    ieee80211_crypto_aes_support();
#endif
#endif

    struct ieee80211_initparam param;
    os_memset(&param, 0, sizeof(param));
    param.vif_maxcnt = 2;
    param.sta_maxcnt = 2;
    param.bss_maxcnt = 2;
    param.bss_lifetime  = 300; //300 seconds
    param.no_rxtask = 1;
    param.evt_cb = sys_wifi_event;
    ieee80211_init(&param);
    ieee80211_support_txw80x(ops);

    sys_wifi_parameter_init(ops);
    ieee80211_iface_create_ap(WIFI_MODE_AP, IEEE80211_BAND_2GHZ);
    ieee80211_iface_create_sta(WIFI_MODE_STA, IEEE80211_BAND_2GHZ);
    
#if NET_TO_QC == 1
	qc_mode = wifi_qc_scan(WIFI_MODE_STA);
#endif
	
	wificfg_flush(WIFI_MODE_STA);
	sys_mode_confirm();
	sys_wifi_start_acs(ops);
	wificfg_flush(WIFI_MODE_AP);

    uint8 txq[][8] = {
        {0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00,},
        {0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x01,},
        {0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x02,},
        {0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x03,},
    };


    ieee80211_conf_set_wmm_param( WIFI_MODE_AP, 0xf0, ( struct ieee80211_wmm_param *)&txq[0]);
    ieee80211_conf_set_wmm_param( WIFI_MODE_AP, 0xf1, ( struct ieee80211_wmm_param *)&txq[1]);
    ieee80211_conf_set_wmm_param(WIFI_MODE_AP, 0xf2, ( struct ieee80211_wmm_param *)&txq[2]);
    ieee80211_conf_set_wmm_param(WIFI_MODE_AP, 0xf3, ( struct ieee80211_wmm_param *)&txq[3]);
#if NET_PAIR
    ieee80211_pair_enable(WIFI_MODE_AP, 0x1234);
    ieee80211_pair_enable(WIFI_MODE_STA, 0x1234);
    set_pair_mode(0);
#endif

}

static void sys_dhcpc_check(void)
{
    if (sys_cfgs.dhcpc_en && !sys_status.dhcpc_done) {
        ip_addr_t ip = lwip_netif_get_ip2("w0");
        if (ip.addr == 0) {
            lwip_netif_set_dhcp2("w0", 1);
        }
    }
}

__init static void sys_dhcpd_start()
{
    //使用eloop机制去实现dhcp,所以要在lwip初始化完成并且eloop启动才能使用
    struct dhcpd_param param;
    os_memset(&param, 0, sizeof(param));
    param.start_ip   = sys_cfgs.dhcpd_startip;
    param.end_ip     = sys_cfgs.dhcpd_endip;
    param.netmask    = sys_cfgs.netmask;
    param.router     = sys_cfgs.gw_ip;
    param.dns1       = sys_cfgs.gw_ip;
    param.lease_time = sys_cfgs.dhcpd_lease_time;
    if (dhcpd_start_eloop("w0", &param)) {
        os_printf("dhcpd start error\r\n");
    }
    dns_start_eloop("w0");
}

__init static void sys_network_init(void)
{
    ip_addr_t ipaddr, netmask, gw;
    struct netdev *ndev;
	int offset = sys_cfgs.wifi_mode == WIFI_MODE_STA?WIFI_MODE_STA:WIFI_MODE_AP;
    tcpip_init(NULL, NULL);

    ndev = (struct netdev *)dev_get(HG_WIFI0_DEVID + offset - 1);
    if (ndev) {
        ipaddr.addr  = sys_cfgs.ipaddr;
        netmask.addr = sys_cfgs.netmask;
        gw.addr      = sys_cfgs.gw_ip;
        lwip_netif_add(ndev, "w0", &ipaddr, &netmask, &gw);
        lwip_netif_set_default(ndev);
        if (sys_cfgs.wifi_mode == WIFI_MODE_STA && sys_cfgs.dhcpc_en) {
            sys_status.dhcpc_done = 0;
            lwip_netif_set_dhcp(ndev, 1);
            os_printf("start dhcp client ...\r\n");
        }
        os_printf("add w0 interface!\r\n");
    }else{
        os_printf("**not find wifi device**\r\n");
    }
}

extern u8_t netif_num;
void close_wifi(void)
{
	struct netdev *ndev;

	if (sys_cfgs.wifi_mode == WIFI_MODE_AP)
	{
		ndev = (struct netdev *)dev_get(HG_WIFI1_DEVID);
	}
	else
	{
		ndev = (struct netdev *)dev_get(HG_WIFI0_DEVID);
	}

	if (ndev)
	{
		lwip_netif_set_dhcp(ndev, 0);
		os_sleep_ms(1);
		lwip_netif_remove(ndev);
		netif_num--;
	}
}

void open_wifi(void)
{
	struct netdev *ndev;
	ip_addr_t ipaddr, netmask, gw;

	if (sys_cfgs.wifi_mode == WIFI_MODE_AP)
	{
		ndev = (struct netdev *)dev_get(HG_WIFI1_DEVID);
		//wificfg_flush(WIFI_MODE_AP);
		wifi_create_ap((char*)sys_cfgs.ssid, sys_cfgs.passwd, WPA_KEY_MGMT_NONE, sys_cfgs.channel);
	}
	else
	{
		ndev = (struct netdev *)dev_get(HG_WIFI0_DEVID);
		//wificfg_flush(WIFI_MODE_STA);
		// wifi_create_station("BGN_test", "12345678", WPA_KEY_MGMT_PSK);
	}

	if (ndev)
	{
		ipaddr.addr  = sys_cfgs.ipaddr;
		netmask.addr = sys_cfgs.netmask;
		gw.addr      = sys_cfgs.gw_ip;
		lwip_netif_add(ndev, "w0", &ipaddr, &netmask, &gw);
		lwip_netif_remove_register(ndev);
		lwip_netif_set_default(ndev);

		if (sys_cfgs.wifi_mode == WIFI_MODE_STA && sys_cfgs.dhcpc_en)
		{
			lwip_netif_set_dhcp(ndev, 1);
		}

		if (sys_cfgs.wifi_mode == WIFI_MODE_AP)
		{
			_os_printf("dhcpd start.....\r\n");
			sys_dhcpd_start();
		}

		os_printf("add w0 interface!\r\n");
	}
	else
	{
		os_printf("**not find wifi device**\r\n");
	}
}


void sys_wifi_close_open_thread(){
	uint8 wifi_action = 0;
	while(1){
		os_sleep_ms(30000);
		
		if(wifi_action){
			wifi_action = 0;
			_os_printf("open wifi..............\r\n");
			open_wifi();
		}else{
			wifi_action = 1;
			_os_printf("close wifi..............\r\n");
			close_wifi();
		}

	}
}

/*
 *	wifi_exchange_mode:  WIFI_MODE_AP or WIFI_MODE_STA
 */
__init static void sys_net_workmode_exchange(uint8  wifi_exchange_mode)
{	
	struct netdev *ndev;
	ip_addr_t ipaddr, netmask, gw;
	if(wifi_exchange_mode == sys_cfgs.wifi_mode){
		return;
	}
	
#ifdef CONFIG_UMAC4
    if(sys_cfgs.wifi_mode == WIFI_MODE_AP){
        ndev = (struct netdev *)dev_get(HG_WIFI1_DEVID);
    }else{
        ndev = (struct netdev *)dev_get(HG_WIFI0_DEVID);
    }
#else
    ndev = (struct netdev *)dev_get(HG_WIFI0_DEVID);
#endif	
	if (ndev){
		lwip_netif_remove(ndev);		
	}


	sys_cfgs.wifi_mode = wifi_exchange_mode;
#ifdef CONFIG_UMAC4
	if(sys_cfgs.wifi_mode == WIFI_MODE_AP){
		ndev = (struct netdev *)dev_get(HG_WIFI1_DEVID);
		//wificfg_flush(WIFI_MODE_AP);
		wifi_create_ap("HG_TEST","12345678",WPA_KEY_MGMT_NONE,5);
	}else{
		ndev = (struct netdev *)dev_get(HG_WIFI0_DEVID);
		//wificfg_flush(WIFI_MODE_STA);
		wifi_create_station("BGN_test","12345678",WPA_KEY_MGMT_PSK);
	}
#else
	ndev = (struct netdev *)dev_get(HG_WIFI0_DEVID);
#endif

	if(ndev){
        ipaddr.addr  = sys_cfgs.ipaddr;
        netmask.addr = sys_cfgs.netmask;
        gw.addr      = sys_cfgs.gw_ip;
        lwip_netif_add(ndev, "w0", &ipaddr, &netmask, &gw);
		lwip_netif_remove_register(ndev);
        lwip_netif_set_default(ndev);
        if (sys_cfgs.wifi_mode == WIFI_MODE_STA && sys_cfgs.dhcpc_en) {
            lwip_netif_set_dhcp(ndev, 1);
        }
		if(sys_cfgs.wifi_mode == WIFI_MODE_AP){
			sys_dhcpd_start();
		}

        os_printf("add w0 interface!\r\n");	
	}else{
        os_printf("**not find wifi device**\r\n");
	}
}


__init ip_addr_t sys_network_get_ip(void){
	return lwip_netif_get_ip2("w0");
}

/********************************************
应用相关的网络应用,eventLoop、默认一些socket
的监听等
********************************************/

__init static void app_network_init()
{
    #define ELOOP_TASK_HEAP 2048
    eloop_init ();  
    os_task_create("eloop_run", user_eloop_run, NULL, OS_TASK_PRIORITY_NORMAL, 0, NULL, ELOOP_TASK_HEAP);
    os_sleep_ms(1);
    if (sys_cfgs.wifi_mode == WIFI_MODE_AP) { //AP
        sys_dhcpd_start();
    }
    //ota_Tcp_Server();
    #if NET_TO_QC == 1
        if (sys_cfgs.wifi_mode == WIFI_MODE_STA){
            sta_send_udp_msg_init();
        }
    #endif

#if USB_EN || JPG_EN
    user_protocol();
#endif


#if SDH_EN && FS_EN   && OPENDML_EN
    extern void recv_record_Server(int port);
    recv_record_Server(43210);

#endif


#ifdef CONFIG_UMAC4
#if BLE_SUPPORT 
    #if BLE_PAIR_NET == 1
        //配网仅仅支持station模式
        if(sys_cfgs.wifi_mode == WIFI_MODE_STA)
        {
			//关闭wifi
			ieee80211_iface_stop(WIFI_MODE_STA);
			ieee80211_iface_stop(WIFI_MODE_AP);
            //启动蓝牙配网
            //没有配过网络,先进行网络配置
            if(!get_sys_cfgs_ble_pair_status())
            {
                ble_reset_pair_network(38, 1);
            }
            //否则直接发送小无线
            else
            {
                ble_reset_pair_network(38, NULL);
            }
        }
    #elif BLE_PAIR_NET == 2
        struct lmac_ops *lops = (struct lmac_ops *)g_ops;
	    struct bt_ops *bt_ops = (struct bt_ops *)lops->btops;

        ble_set_coexist_en(bt_ops, 1, 0);
		ble_demo_start(bt_ops, 2);
    #endif
#endif
#endif

#if LOWPOWER_DEMO == 1
	app_lowpower_init();
#endif


}


uint8 vcam_en()
{
    uint8 ret = TRUE;
#if VCAM_EN
	pmu_vcam_dis();
	os_sleep_ms(1);
    pmu_set_vcam_vol(VCAM_VOL_2V80);
    pmu_vcam_lc_en();
    pmu_vcam_oc_detect_dis();
    pmu_vcam_oc_int_dis();
    pmu_vcam_discharge_dis();
    pmu_vcam_pg_dis();
#ifdef VCAM_33
    pmu_set_vcam_vol(VCAM_VOL_3V25);
    pmu_vcam_en();
    os_sleep_ms(1);
    pmu_vcam_pg_en();
#else
    pmu_vcam_en();
    os_sleep_ms(1);
#endif    
//    sys_reset_pending_clr();
    pmu_vcam_lc_dis();
#ifndef VCAM_33
    pmu_vcam_oc_detect_en();
#endif
    if(PMU_VCAM_OC_PENDING){
        return FALSE;
    }
    pmu_vcam_oc_detect_dis();
    pmu_vcam_oc_int_dis();
    pmu_lvd_oe_en();
#endif
    return ret;
}


void hardware_init(uint8 vcam)
{
	//gpio_set_val(PC_7,1);
	//gpio_iomap_output(PC_7,GPIO_IOMAP_OUTPUT);
    if(vcam == FALSE)
	{
		os_printf("vcam err\n");
        return;
	}



    
    //需要workqueue支持
    stream_work_queue_start();
	#if KEY_MODULE_EN == 1
	keyWork_init(10);
	#endif

#if FLASHDISK_EN
	flash_fatfs_init();
#endif


#if JPG_EN
  //jpg_cfg(HG_JPG1_DEVID,VPP_DATA0);
    #if 1//SD_SAVE
		extern void jpg_cfg(uint8 jpgid,enum JPG_SRC_FROM src_from);
		#if BBM_DEMO
			jpg_cfg(HG_JPG0_DEVID,VPP_DATA1);
		#else
			jpg_cfg(HG_JPG0_DEVID,VPP_DATA0);
		#endif
    #endif
#endif
        
#if DVP_EN
	bool csi_ret;
    bool csi_cfg();
    csi_ret = csi_cfg();
#endif



#if LCD_EN


    #if LVGL_STREAM_ENABLE == 1
	uint16_t w,h;
    uint16_t screen_w,screen_h;
    uint8_t rotate,video_rotate;
    extern stream *lvgl_R_osd_stream(const char *name);
    extern stream *osd_show_stream(const char *name);
    extern stream *yuv_recv_stream(const char *name);
    lcd_hardware_init(&w,&h,&rotate,&screen_w,&screen_h,&video_rotate);
    lcd_arg_setting(w,h,rotate,screen_w,screen_h,video_rotate);
    lcd_driver_init((void*)lvgl_R_osd_stream(R_OSD_ENCODE),(void*)osd_show_stream(R_OSD_SHOW),(void*)yuv_recv_stream(R_VIDEO_P1),(void*)yuv_stream(R_VIDEO_P0));

    #else
		uint16_t w,h;
		uint8_t rotate;
	    void lcd_module_run(uint16_t *w,uint16_t *h,uint8_t *rotate);	
        lcd_module_run(&w,&h,&rotate);
    #endif
#endif

#if DVP_EN
    bool csi_open();
	if(csi_ret)
    	csi_open();
#endif

#if SDH_EN && FS_EN
    extern bool fatfs_register();
    sd_open();
    fatfs_register();
#endif

#if SD_SAVE
	void sd_save_thread_start();
    sd_save_thread_start();
#endif


#if AUDIO_EN
	audio_adc_init();
    
#endif

#if AUDIO_DAC_EN
    audio_da_init();
#endif
    

#if (USB_EN && RTT_USB_EN)
	#if (USB_DETECT_EN && !USB_HOST_EN)
        extern void hg_usb_connect_detect_init(void);
		hg_usb_connect_detect_init();
	#else
		#if USB_HOST_EN
            extern rt_err_t hg_usbh_register(rt_uint32_t devid);
			hg_usbh_register(HG_USB_HOST_CONTROLLER_DEVID);
		#else 
            extern void hg_usbd_class_driver_register();
            extern int hg_usbd_register(rt_uint32_t devid);
			hg_usbd_class_driver_register();
			hg_usbd_register(HG_USB_DEV_CONTROLLER_DEVID);
		#endif
	#endif


#endif

#if (USB_EN && !RTT_USB_EN)
    void hg_usb_test(void);
    hg_usb_test();
#endif

#if LCD_EN
	void lvgl_init(uint16_t w,uint16_t h,uint8_t rotate);
	lvgl_init(w,h,rotate);
#endif



#if MP3_EN
    reg_mp3_malloc(custom_malloc_psram,custom_free_psram,custom_calloc_psram);
#endif

#if PRINTER_EN
	printer_thread_init();
#endif

	//user_hardware_config();
}

static int32 main_loop(struct os_work *work)
{
    static int8 print_interval = 0;
    sys_dhcpc_check();
    if (print_interval++ >= 5) {
        uint32 sysheap_freesize(struct sys_heap *heap);
        os_printf("ip:%x  freemem:%d\r\n", lwip_netif_get_ip2("w0").addr,sysheap_freesize(&sram_heap));
        print_interval = 0;
    }

    #ifdef PSRAM_HEAP
        print_custom_psram();
    #endif
    print_custom_sram();

	os_printf("freemem:%d\r\n", sysheap_freesize(&sram_heap));
#ifdef CONFIG_SLEEP
    extern void dsleep_ip_addr_set(uint32 ip_addr);
    dsleep_ip_addr_set(lwip_netif_get_ip2("w0").addr);
#endif
    os_run_work_delay(&main_wk, 1000);
	return 0;
}


//如果已经在project_config.h添加了CUSTOM_SIZE,就不需要再次定义
//放这里是为了调整空间的时候不需要修改project_config.h,减少编译时间
#ifndef CUSTOM_SIZE
    #ifdef PSRAM_HEAP
        #define CUSTOM_SIZE (5*1024) 
    #else
        /**********************************************************************
         * 设置空间默认值(实际根据需求需要修改SYS_HEAP_SIZE以及CUSTOM_SIZE的值
         * 这里给的默认值只是图传功能,如果是额外增加其他功能,比如录卡等,需要
         * 自定义
         * 
         * 
         * usb默认给113K
         * DVP默认给60K
        ***********************************************************************/
        #if USB_EN == 1
            #define CUSTOM_SIZE (113*1024) 
        #elif DVP_EN == 1
            #define CUSTOM_SIZE (60*1024) 
        #else
            #define CUSTOM_SIZE (5*1024)    
        #endif
    #endif
#endif

#ifndef SPEED_TEST_DEMO
int main(void)
{
    uint32 sysheap_freesize(struct sys_heap *heap);
    os_printf("freemem:%d\r\n",sysheap_freesize(&sram_heap));
	#ifdef PSRAM_HEAP
		while(!get_psram_status())
		{
			os_printf("psram no ready:%d\n",get_psram_status());
			os_sleep_ms(1000);
			
		}
	#endif
	//sys_watchdog_pre_init();

	#ifdef PSRAM_HEAP
		#ifdef CUSTOM_PSRAM_SIZE
			#if CUSTOM_PSRAM_SIZE > 0
			{
				void *custom_buf = (void*)os_malloc_psram(CUSTOM_PSRAM_SIZE);
				custom_mem_psram_init(custom_buf,CUSTOM_PSRAM_SIZE);
				print_custom_psram();
			}
			#endif
		#endif
	#endif

    #ifdef CUSTOM_SIZE
        #if CUSTOM_SIZE > 0
        {
            void *custom_buf = (void*)os_malloc(CUSTOM_SIZE);
            custom_mem_init(custom_buf,CUSTOM_SIZE);
            print_custom_sram();
        }
        #endif
    #endif


    uint8 vcam;
    //do_global_ctors();
    //wifi_qc_mode_inspect();
    //sys_watchdog_pre_init();
    void *skb_pool_addr = (void*)os_malloc(SKB_POOL_SIZE);
    if(!skb_pool_addr)
    {
        while(1)
        {
			os_printf("skb malloc fail,malloc size:%d\tremain size:%d\n",SKB_POOL_SIZE,sysheap_freesize(&sram_heap));
			os_sleep_ms(1000);
        }
    }
    skbpool_init((uint32_t)skb_pool_addr, SKB_POOL_SIZE, 80, 0);
    sys_cfg_load();
    vcam = vcam_en();
    user_io_preconfig();
    sys_event_init(32);
    sys_event_take(SYS_EVENT(SYS_EVENT_WIFI, 0), sysevt_wifi_event, 0);
    sys_event_take(SYS_EVENT(SYS_EVENT_NETWORK, 0), sysevt_network_event, 0);
    sys_event_take(SYS_EVENT(SYS_EVENT_LTE, 0), sysevt_lte_event, 0);
    sys_event_take(SYS_EVENT(SYS_EVENT_BLE, 0), sysevt_ble_event, 0);
    sys_atcmd_init();
    sys_wifi_init();
    // enter wifi test mode
    if(system_is_wifi_test_mode()) {
        system_reboot_normal_mode();
    } else {
        sys_network_init();
        //dsleep_test_init();
        //udp_kalive_init("192.168.1.100", 60002, SYSTEM_SLEEP_TYPE_SRAM_WIFI, NULL);
#if 1
        hardware_init(vcam);
#endif

#ifdef CONFIG_SLEEP
#if SYS_APP_DSLEEP_TEST
        sock_monitor_init();
        dsleep_test_init();
#endif
#endif
	
        app_network_init();
        mcu_watchdog_timeout(5);
        OS_WORK_INIT(&main_wk, main_loop, 0);
        os_run_work_delay(&main_wk, 1000);
    }
    pmu_clr_deadcode_pending();
#ifdef LLM_DEMO
extern void llm_demo(void);
    llm_demo();
#endif
    return 0;
}


//测试速率使用,wifi模式:RATE_CONTROL_SELECT请选择RATE_CONTROL_IPC
#else

#undef SKB_POOL_SIZE
#define SKB_POOL_SIZE (150*1024)
static int32 main_loop2(struct os_work *work)
{
	struct os_task_info mytsk_info[20];
	cpu_loading_print(1,mytsk_info,20);
    os_run_work_delay(&main_wk, 5000);
	return 0;
}


int main()
{
    uint32 sysheap_freesize(struct sys_heap *heap);
    os_printf("freemem:%d\r\n",sysheap_freesize(&sram_heap));
	#ifdef PSRAM_HEAP
		while(!get_psram_status())
		{
			os_printf("psram no ready:%d\n",get_psram_status());
			os_sleep_ms(1000);
			
		}
	#endif

	#ifdef PSRAM_HEAP
		#ifdef CUSTOM_PSRAM_SIZE
			#if CUSTOM_PSRAM_SIZE > 0
			{
				void *custom_buf = (void*)os_malloc_psram(CUSTOM_PSRAM_SIZE);
				custom_mem_psram_init(custom_buf,CUSTOM_PSRAM_SIZE);
				print_custom_psram();
			}
			#endif
		#endif
	#endif

    #ifdef CUSTOM_SIZE
        #if CUSTOM_SIZE > 0
        {
            void *custom_buf = (void*)os_malloc(CUSTOM_SIZE);
            custom_mem_init(custom_buf,CUSTOM_SIZE);
            print_custom_sram();
        }
        #endif
    #endif
    void *skb_pool_addr = (void*)os_malloc(SKB_POOL_SIZE);
    if(!skb_pool_addr)
    {
        while(1)
        {
			os_printf("skb malloc fail,malloc size:%d\tremain size:%d\n",SKB_POOL_SIZE,sysheap_freesize(&sram_heap));
			os_sleep_ms(1000);
        }
    }
    skbpool_init((uint32_t)skb_pool_addr, SKB_POOL_SIZE, 80, 0);
    sys_cfg_load();
    sys_event_init(32);
    sys_event_take(SYS_EVENT(SYS_EVENT_WIFI, 0), sysevt_wifi_event, 0);
    sys_event_take(SYS_EVENT(SYS_EVENT_LTE, 0), sysevt_lte_event, 0);
    sys_atcmd_init();
    sys_wifi_init();

    sys_network_init();
    app_network_init();
    mcu_watchdog_timeout(5);
    OS_WORK_INIT(&main_wk, main_loop2, 0);
    os_run_work_delay(&main_wk, 1000);
    //speedTest_tcp_server(20202);
	//speedTest_tcp_rx_server(20203);
	//Udp_rx_server(20204,1000);
	//Udp_to_ip("192.168.1.100",20202,0);
    return 0;
}
#endif

