#ifndef _DHCPD_ELOOP_H_
#define _DHCPD_ELOOP_H_
#include "lib/net/dhcpd/dhcpd.h"	//引用dhcp的结构体

int32 dhcpd_start_eloop(char *ifname, struct dhcpd_param *param);
void dhcpd_stop_eloop(char *ifname);

#endif

