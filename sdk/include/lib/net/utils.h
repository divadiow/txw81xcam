#ifndef _HGSDK_NETUTILS_H_
#define _HGSDK_NETUTILS_H_
#ifdef __cplusplus
extern "C" {
#endif

struct udpdata_info {
    uint8 *dest, *src;
    uint32 dest_ip, dest_port;
    uint32 src_ip, src_port;
    uint8 *data;
    uint32 len;
};

struct icmp_info {
    uint8 *dest, *src;
    uint32 dest_ip;
    uint32 src_ip;
    uint8 *data;
    uint32 len;
    uint8  type;
    uint16 sn;
};

uint16 net_checksum(uint16 initcksum, uint8 *data, int32 len);
uint16 net_pseudo_checksum(uint8 proto, uint8 *data, uint16 len, uint32 *srcaddr, uint32 *destaddr);
uint16 net_setup_udpdata(struct udpdata_info *data, uint8 *udp);
int32 net_setup_icmp(struct icmp_info *data, uint8 *icmp);
void wnb_ota_init(void);
void net_atcmd_init(void);
void netlog_init(uint16 port);
int32 sntp_client_init(char *ntp_server, uint16 update_interval);
void sntp_client_fresh(void);
void sntp_set_server(char *ntp_server);
void sntp_set_interval(uint16 interval);
void sntp_client_disable(int8 disable);
void sntp_set_sport(uint16 server_port);
int32 dns_redirect_init(void);
int32 dns_redirect_add(const char *domain, const char *ifname);
uint32 scatter_data_size(scatter_data *data, uint32 count);
void auto_wds_init(void);
void auto_wds_update(uint8 ifidx, uint32 valid_ip);

#ifdef __cplusplus
}
#endif
#endif
