#ifndef _SDK_SKMONITOR_H_
#define _SDK_SKMONITOR_H_

typedef enum {
    SOCK_MONITOR_READ   = BIT(0),
    SOCK_MONITOR_WRITE  = BIT(1),
    SOCK_MONITOR_ERROR  = BIT(2),
} skmonitor_flags;

typedef void (*skmonitor_cb)(uint16 sock, skmonitor_flags flags, uint32 priv);

int32 sock_monitor_init(void);
int32 sock_monitor_add(uint16 sock, skmonitor_flags flags, skmonitor_cb cb, uint32 priv);
void sock_monitor_del(uint16 sock);
void sock_monitor_disable(uint16 sock, uint8 disable);

#endif
