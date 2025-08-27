/**
  ******************************************************************************
  * @file    dhcpd_eloop.c
  * @author  HUGE-IC Application Team
  * @version V1.0.0
  * @date    2021-07-16
  * @brief   a simple dhcp server
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2021 HUGE-IC</center></h2>
  *
  *
  ******************************************************************************
  */
#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "osal/task.h"
#include "osal/string.h"
#include "osal/sleep.h"
#include "lib/common/sysevt.h"
#include "lib/net/utils.h"
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/inet_chksum.h"
#include "netif/etharp.h"
#include "netif/ethernetif.h"
#include "lwip/ip.h"
#include "lwip/init.h"
#include "lwip/prot/dhcp.h"
#include "dhcpd_eloop.h"
#include "event.h"


#define dhcpd_dbg(fmt, ...) os_printf(fmt, ##__VA_ARGS__)
#define dhcpd_err(fmt, ...) os_printf(fmt, ##__VA_ARGS__)

#define DHCPD_RECVBUF_SIZE (1024)

struct dhcpd_ipaddr {
    struct list_head list;
    uint32 ip;
    uint64 lifetime;
    uint8  mac[6];
};

struct dhcpd_optinfo {
    uint32 request_ip;
    uint8  msg_type;
};

struct dhcpd_mgr {
    uint8  stop: 1;
    uint8  recvbuf[DHCPD_RECVBUF_SIZE];
    int32  sock;
    struct dhcpd_param param;
    struct dhcpd_optinfo info;
    struct list_head addr_list;
    uint32 nextip;   // LITTLE_ENDIAN
    struct netif *netif;

    void *dhcp_event;
    void *dhcp_timer_event;
};

static struct dhcpd_mgr *dhcpd;

#if 0
static int32 dhcpd_recv(uint32 tmo)
{
    uint32 ret = 0;
    fd_set rset;
    socklen_t len = 0;
    struct timeval tv;
    struct sockaddr_in addr;

    FD_ZERO(&rset);
    FD_SET(dhcpd->sock, &rset);
    tv.tv_sec = tmo / 1000;
    tv.tv_usec = (tmo % 1000) * 1000;
    ret = select(dhcpd->sock + 1, &rset, NULL, NULL, &tv);
    if (ret > 0) {
        ret = recvfrom(dhcpd->sock, dhcpd->recvbuf, DHCPD_RECVBUF_SIZE, 0, (struct sockaddr *)&addr, &len);
    }
    return ret;
}
#endif

static void dhcpd_send(uint8 *dest, uint32 dstip, uint8 *data, uint32 len)
{
    struct udpdata_info udp;
    uint8  bcast_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    struct pbuf *buf = pbuf_alloc(PBUF_RAW, len + 14 + 20 + 12, PBUF_POOL);
    if (buf) {
        udp.dest      = dest ? dest : bcast_addr;
        udp.src       = dhcpd->netif->hwaddr;
        udp.dest_ip   = ntohl(dstip);
        udp.src_ip    = ntohl(dhcpd->netif->ip_addr.addr);
        udp.dest_port = 68;
        udp.src_port  = 67;
        udp.data      = data;
        udp.len       = len;
        net_setup_udpdata(&udp, buf->payload);
        dhcpd->netif->linkoutput(dhcpd->netif, buf);
        pbuf_free(buf);
    }
}

static void dhcpd_check_expired(void *ei, void *d)
{
    uint64 jiff = os_jiffies();
    struct dhcpd_ipaddr *entry = NULL;
    struct dhcpd_ipaddr *nxt   = NULL;
    struct dhcpd_mgr *dhcpd = (struct dhcpd_mgr *)d;
    struct list_head    *head  = &dhcpd->addr_list;
    
    list_for_each_entry_safe(entry, nxt, head, list) {
        if (TIME_AFTER(jiff, entry->lifetime + os_msecs_to_jiffies(dhcpd->param.lease_time * 1000))) {
            list_del(&entry->list);
            dhcpd_dbg("ip (%s-"MACSTR") expired, del it\r\n", inet_ntoa(entry->ip), MAC2STR(entry->mac));
            os_free(entry);
        }
    }
}

void dhcpd_dump_ippool(void)
{
    struct dhcpd_ipaddr *entry = NULL;
    struct list_head    *head  = &dhcpd->addr_list;

    dhcpd_dbg("IP Pool:\r\n");
    list_for_each_entry(entry, head, list) {
        dhcpd_dbg("    ip:%s - "MACSTR"\r\n", inet_ntoa(entry->ip), MAC2STR(entry->mac));
    }
}

bool dhcpd_get_mac_by_ip(uint32 ip,uint8 *mac){
    struct dhcpd_ipaddr *entry = NULL;
    struct list_head    *head  = &dhcpd->addr_list;
    list_for_each_entry(entry, head, list) {
        if(ip == entry->ip){
            memcpy(mac,entry->mac,6);
            return RET_OK;
        }
    }
    return RET_ERR;
}

static uint32 dhcpd_next_ip(uint32 ip, uint32 mask)
{
    uint32 subip = (ip&~mask)+1;
    uint32 newip = (ip&mask)+(subip&~mask);
    if(newip < ntohl(dhcpd->param.start_ip) || newip > ntohl(dhcpd->param.end_ip)){
        newip = ntohl(dhcpd->param.start_ip);
    }
    dhcpd_dbg("Next IP: "IPSTR"\r\n", IP2STR_H(newip));
    return newip;
}

static struct dhcpd_ipaddr *dhcpd_find_ip(uint32 ip, uint8 *mac)
{
    struct dhcpd_ipaddr *entry = NULL;
    struct list_head    *head  = &dhcpd->addr_list;

    if (ip || mac) {
        list_for_each_entry(entry, head, list) {
            if ((ip && htonl(ip) == entry->ip) || (mac && os_memcmp(mac, entry->mac, 6) == 0)) {
                return entry;
            }
        }
    }
    return NULL;
}

static uint32 dhcpd_check_ip(uint32 ip, uint8 *mac)
{
    struct dhcpd_ipaddr *entry = dhcpd_find_ip(ip, 0);
    return (entry ? (os_memcmp(entry->mac, mac, 6) == 0) : 1);
}

static uint32 dhcpd_netcmp(uint32 ip)
{
    /* !0 if the network identifiers of both address match */
    return ((ip & ntohl(dhcpd->param.netmask)) == (ntohl(dhcpd->param.start_ip) & ntohl(dhcpd->param.netmask)));
}

static uint32 dhcpd_free_ipaddr()
{
    uint32 ip = dhcpd->nextip;

    while(dhcpd_find_ip(dhcpd->nextip, 0)){
        dhcpd->nextip = dhcpd_next_ip(dhcpd->nextip, ntohl(dhcpd->param.netmask));
        if(dhcpd->nextip == ip){
            return 0;
        }
    }
    return dhcpd->nextip;
}

static uint32 dhcpd_get_ip(uint32 ip, uint8 *mac)
{
    struct dhcpd_ipaddr *entry = dhcpd_find_ip(0, mac);
    if (entry == NULL) {
        if(ip == 0){
            if(!dhcpd_free_ipaddr()){
                SYSEVT_NEW_NETWORK_EVT(SYSEVT_DHCPD_IPPOOL_FULL, 0);
                return 0;
            }
            ip = dhcpd->nextip;
        }
        entry = os_zalloc(sizeof(struct dhcpd_ipaddr));
        if (entry) {
            list_add(&entry->list, &dhcpd->addr_list);
            entry->ip = htonl(ip);
            os_memcpy(entry->mac, mac, 6);
            entry->lifetime = os_jiffies();
            dhcpd->nextip   = dhcpd_next_ip(dhcpd->nextip, ntohl(dhcpd->param.netmask));
        }
    }else{
        if(ip) entry->ip = htonl(ip);
        entry->lifetime = os_jiffies();
    }
    return (entry ? entry->ip: 0);
}

static void dhcpd_send_nak(struct dhcp_msg *msg)
{
    uint16 flags = get_unaligned_be16(&msg->flags);
    uint8 *opt = (uint8 *)msg + DHCP_OPTIONS_OFS;
    msg->op = 2;

    *opt++ = DHCP_OPTION_MESSAGE_TYPE;
    *opt++ = DHCP_OPTION_MESSAGE_TYPE_LEN;
    *opt++ = DHCP_NAK;

    *opt++ = DHCP_OPTION_END;
    dhcpd_dbg("send DHCP_NAK to "MACSTR"\r\n", MAC2STR(msg->chaddr));
    dhcpd_send((flags&0x8000) ? NULL : msg->chaddr, 0xffffffff, (uint8 *)msg, (uint32)opt - (uint32)msg);
}

static void dhcpd_reply(struct dhcp_msg *msg, uint8 reply)
{
    uint8 *opt = (uint8 *)msg + DHCP_OPTIONS_OFS;
    uint32 ip  = dhcpd_get_ip(dhcpd->info.request_ip, msg->chaddr);
    uint16 flags = get_unaligned_be16(&msg->flags);

    /* dhcp server don't need setup client ip addr */
    put_unaligned_le32(0, &msg->ciaddr);
    if (ip == 0) {
        dhcpd_send_nak(msg);
        dhcpd_err("no free ip address\r\n");
        return;
    }

    // little order IP use IP2STR_H, and can not use inet_ntoa() at same statement
    dhcpd_err("Assign IP "IPSTR" for "MACSTR", flags=%x (next:"IPSTR")\r\n", 
              IP2STR_N(ip), MAC2STR(msg->chaddr), flags, IP2STR_H(dhcpd->nextip));
    msg->op = 2;
    put_unaligned_le32(ip, &msg->yiaddr);

    *opt++ = DHCP_OPTION_MESSAGE_TYPE;
    *opt++ = 1;
    *opt++ = reply;

    *opt++ = DHCP_OPTION_SERVER_ID;
    *opt++ = 4;
    put_unaligned_le32(dhcpd->netif->ip_addr.addr, opt); opt += 4;

    *opt++ = DHCP_OPTION_LEASE_TIME;
    *opt++ = 4;
    put_unaligned_be32(dhcpd->param.lease_time, opt); opt += 4;

    *opt++ = DHCP_OPTION_SUBNET_MASK;
    *opt++ = 4;
    put_unaligned_le32(dhcpd->param.netmask, opt); opt += 4;

    if (dhcpd->param.router) {
        *opt++ = DHCP_OPTION_ROUTER;
        *opt++ = 4;
        put_unaligned_le32(dhcpd->param.router, opt); opt += 4;
    }

    if (dhcpd->param.dns1) {
        *opt++ = DHCP_OPTION_DNS_SERVER;
        *opt++ = 8;
        put_unaligned_le32(dhcpd->param.dns1, opt); opt += 4;
        put_unaligned_le32(dhcpd->param.dns2, opt); opt += 4;
    }

    *opt++ = DHCP_OPTION_END;
    dhcpd_send((flags&0x8000) ? NULL : msg->chaddr, (flags&0x8000) ? 0xffffffff : ip, (uint8 *)msg, (uint32)opt - (uint32)msg);
}

static void dhcpd_send_ack(struct dhcp_msg *msg)
{
    dhcpd_dbg("send DHCP_ACK ...\r\n");
    return dhcpd_reply(msg, DHCP_ACK);
}

static void dhcpd_send_offer(struct dhcp_msg *msg)
{
    dhcpd_dbg("send DHCP_OFFER ...\r\n");
    return dhcpd_reply(msg, DHCP_OFFER);
}

static void dhcpd_parse_opt(uint8 *dhcp_opt, int32 len, struct dhcpd_optinfo *info)
{
    uint8 *opts = dhcp_opt;
    uint8 option;
    uint8 opt_len;

    info->request_ip = 0;
    info->msg_type   = 0;
    while (opts < dhcp_opt + len) {
        option  = *opts++;
        opt_len = *opts++;
        switch (option) {
            case DHCP_OPTION_REQUESTED_IP:
                info->request_ip = get_unaligned_be32(opts);
                break;
            case DHCP_OPTION_MESSAGE_TYPE:
                info->msg_type = *opts;
                break;
            case DHCP_OPTION_END:
                return;
            default:
                break;
        }
        opts += opt_len;
    }
}

static void dhcpd_task_eloop(void *ei, void *d)
{
    int32  ret = 0;
    uint8 *dhcp_opt;
    socklen_t len = 0;
    struct sockaddr_in addr;
    struct dhcp_msg *msg;
    struct dhcpd_mgr *dhcpd = (struct dhcpd_mgr *)d;
    ret = recvfrom(dhcpd->sock, dhcpd->recvbuf, DHCPD_RECVBUF_SIZE, 0, (struct sockaddr *)&addr, &len);
    if (ret >= (int32)DHCP_OPTIONS_OFS && !dhcpd->stop) {
        msg = (struct dhcp_msg *)dhcpd->recvbuf;
        if (msg->op == DHCP_BOOTREQUEST && msg->cookie == PP_HTONL(DHCP_MAGIC_COOKIE)) {
            os_memset(&dhcpd->info, 0, sizeof(dhcpd->info));
            dhcp_opt = dhcpd->recvbuf + DHCP_OPTIONS_OFS;
            dhcpd_parse_opt(dhcp_opt, ret - DHCP_OPTIONS_OFS, &dhcpd->info);
            if (dhcpd->info.msg_type == DHCP_DISCOVER) {
                if (!dhcpd_netcmp(dhcpd->info.request_ip) || !dhcpd_check_ip(dhcpd->info.request_ip, msg->chaddr)) {
                    dhcpd->info.request_ip = 0;
                }
                dhcpd_send_offer(msg);
            } else if (dhcpd->info.msg_type == DHCP_REQUEST) {
                /* request_ip is empty then check client ip addr */
                if (dhcpd->info.request_ip == 0) {
                    dhcpd->info.request_ip = ntohl(msg->ciaddr.addr);
                }
                if(dhcpd->info.request_ip && dhcpd_netcmp(dhcpd->info.request_ip)){
                    if(!dhcpd_check_ip(dhcpd->info.request_ip, msg->chaddr)){
                        dhcpd_send_nak(msg);
                    }
                    else{
                        dhcpd_send_ack(msg);
                        SYSEVT_NEW_NETWORK_EVT(SYSEVT_DHCPD_NEW_IP, dhcpd->info.request_ip);
                        dhcpd_dump_ippool();
                    }
                }
            } else {
                dhcpd_dbg("unsupport msg:%d\r\n", dhcpd->info.msg_type);
            }
        }
    }
    //dhcpd_check_expired();
 
}

__init static int32 _dhcpd_start(char *ifname, struct dhcpd_param *param)
{
    int32 ret  = -1;
    int32 sock = -1;
    int32 optval = 1;
    struct sockaddr_in local_addr;
    struct ifreq nif;

    dhcpd = os_zalloc(sizeof(struct dhcpd_mgr));
    if(dhcpd == NULL)
        return -ENOMEM;
    
    INIT_LIST_HEAD(&dhcpd->addr_list);
    dhcpd->netif = netif_find(ifname);
    if (dhcpd->netif == NULL) {
        dhcpd_err("netif:%s is not exist\r\n", ifname);
        return RET_ERR;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        dhcpd_err("create socket failed\r\n");
        return RET_ERR;
    }

    fcntl(sock, F_SETFL, O_NONBLOCK);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));    
    os_strncpy(nif.ifr_name, dhcpd->netif->name, 2);
    setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, &nif, sizeof(nif));    
    os_memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(67);
    local_addr.sin_addr.s_addr = dhcpd->netif->ip_addr.addr;
    ret = bind(sock, (struct sockaddr *)&local_addr, sizeof(struct sockaddr));
    if (ret == -1) {
        dhcpd_err("bind fail\r\n");
        closesocket(sock);
        return RET_ERR;
    }

    dhcpd->sock   = sock;
    os_memcpy(&dhcpd->param, param, sizeof(struct dhcpd_param));
    dhcpd->nextip = ntohl(dhcpd->param.start_ip);
    //OS_TASK_INIT("dhcpd", &dhcpd->task, dhcpd_task, NULL, OS_TASK_PRIORITY_NORMAL, 1024);
    dhcpd->dhcp_event = (void*) eloop_add_fd( dhcpd->sock, EVENT_READ, EVENT_F_ENABLED, dhcpd_task_eloop, dhcpd );
    dhcpd->dhcp_timer_event = (void*) eloop_add_timer(500, EVENT_F_ENABLED, dhcpd_check_expired, dhcpd);
    return RET_OK;
}

int32 dhcpd_start_eloop(char *ifname, struct dhcpd_param *param)
{
    int32 ret = 0;

    if (ifname == NULL || param == NULL) {
        dhcpd_err("invalid param!\r\n");
        return RET_ERR;
    }
    
    if(dhcpd){
        dhcpd->stop = 1;
        if(dhcpd->dhcp_event)
        {
            eloop_remove_event(dhcpd->dhcp_event);    
        }

        if(dhcpd->dhcp_timer_event)
        {
            eloop_remove_event(dhcpd->dhcp_timer_event);    
        }

        if(dhcpd->sock)
        {
            closesocket(dhcpd->sock);
        }
        dhcpd_err("dhcpd already running ...,first stop last dhcp\r\n");
        //return RET_OK;
        os_free(dhcpd);
        dhcpd = NULL;
    }
    
    ret = _dhcpd_start(ifname, param);
    ASSERT(!ret);
    return ret;
}

void dhcpd_stop_eloop(char *ifname)
{
    if(dhcpd){
        dhcpd->stop = 1;
        if(dhcpd->dhcp_event)
        {
            eloop_remove_event(dhcpd->dhcp_event);    
        }

        if(dhcpd->dhcp_timer_event)
        {
            eloop_remove_event(dhcpd->dhcp_timer_event);    
        }

        if(dhcpd->sock)
        {
            closesocket(dhcpd->sock);
        }
        dhcpd_err("stop last dhcp\r\n");
        //return RET_OK;
        os_free(dhcpd);
        dhcpd = NULL;
    }
}


