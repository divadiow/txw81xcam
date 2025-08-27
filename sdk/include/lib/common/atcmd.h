
#ifndef _COMMON_ATCMD_H_
#define _COMMON_ATCMD_H_

int32 sys_atcmd_loaddef(const char *cmd, char *argv[], uint32 argc);
int32 sys_atcmd_reset(const char *cmd, char *argv[], uint32 argc);
int32 sys_atcmd_jtag(const char *cmd, char *argv[], uint32 argc);
int32 sys_syscfg_dump_hdl(const char *cmd, char *argv[], uint32 argc);
int32 sys_heap_dump_hdl(const char *cmd, char *argv[], uint32 argc);

int32 sys_wifi_atcmd_set_channel(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_set_encrypt(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_set_ssid(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_set_key(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_set_wifimode(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_loaddef(const char *cmd, char *argv[], uint32 argc);

int32 sys_wifi_atcmd_scan(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_pair(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_aphide(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_hwmode(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_ft(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_passwd(const char *cmd, char *argv[], uint32 argc);

int32 sys_ble_atcmd_blenc(const char *cmd, char *argv[], uint32 argc);
int32 sys_ble_atcmd_set_coexist_en(const char *cmd, char *argv[], uint32 argc);

int32 sys_atcmd_ping(const char *cmd, char *argv[], uint32 argc);
int32 sys_atcmd_icmp_mntr(const char *cmd, char *argv[], uint32 argc);
int32 sys_atcmd_iperf2(const char *cmd, char *argv[], uint32 argc);
int32 sys_atcmd_goto_boot(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_reboot_test_mode(const char *cmd, char *argv[], uint32 argc);
int32 sys_wifi_atcmd_wificsa(const char *cmd, char *argv[], uint32 argc);

int32 udp_test_atcmd_hdl(const char *cmd, char *argv[], uint32 argc);
int32 tcp_test_atcmd_hdl(const char *cmd, char *argv[], uint32 argc);

#endif
