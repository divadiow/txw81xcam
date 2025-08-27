#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "osal/string.h"
#include "osal/mutex.h"
#include "osal/work.h"
#include "osal/semaphore.h"
#include "osal/timer.h"
#include "hal/netdev.h"
#include "hal/dma.h"
#include "hal/netdev.h"
#include "lib/heap/sysheap.h"
#include "lib/umac/ieee80211.h"
#include "lib/skb/skb.h"
#include "lib/skb/skb_list.h"
#include "lib/net/utils.h"
#include "syscfg.h"

#ifndef WIFI_DEV_ICMP_MONITOR
#define WIFI_DEV_ICMP_MONITOR 1
#endif

#ifdef WIFI_BRIDGE_DEVLIST
#define ADDR_IDX(addr)          (addr[5]&0x0f)
#define MAC_ENTRY_TMO           (5*60*1000)
#define MAC_ENTRY_COUNT_MAX     (256)
#define MAC_ENTRY_COUNT_WARNING (100)

struct mac_table_entry {
    struct list_head list;
    uint64 lifetime;
    uint8  addr[6];
    struct netdev *ndev;
};
#endif

struct wifi_dev {
    struct netdev wifi;
    netdev_input_cb input_cb;
    void           *input_priv;
    uint8 addr[6];
    uint8 ifidx;
    uint8 icmp_mntr: 1, bridge_mode: 1;
    uint32 no_mem;
    uint32 rx_data, tx_data;

#ifdef WIFI_BRIDGE_DEVLIST
    struct os_task fw_task;
    struct skb_list fw_list;
    struct os_semaphore fw_sema;
    struct os_mutex lock;
    struct os_work work;
    uint32 mac_count;
    struct list_head mac_table[16];
    uint32 brdev_cnt;
    struct netdev *brdev_list[0];
#endif
};

#if WIFI_DEV_ICMP_MONITOR
static void wifi_dev_icmp_monitor(struct wifi_dev *wifi, uint8 *data, uint8 tx)
{
    if (wifi->icmp_mntr) {
        if (get_unaligned_be16(data + 12) == 0x800 && data[23] == 0x1) {
            if (data[34] == 8 || data[34] == 0) {
                uint16 sn = get_unaligned_be16(data + 40);
                os_printf(KERN_NOTICE"wifi: %s imcp %s, sn:%d\r\n", tx ? "TX" : "RX", data[34] == 8 ? "Echo " : "Reply", sn);
            }
        }
    }
}

static uint8 *wifi_dev_scatter_offset(scatter_data *data, uint32 count, uint32 off)
{
    uint8 i;
    for (i = 0; i < count; i++) {
        if (off <= data[i].size) {
            return data[i].addr + off;
        }
        off -= data[i].size;
    }
    return NULL;
}

static void wifi_dev_icmp_monitor_scatter(struct wifi_dev *wifi, scatter_data *data, uint32 count, uint8 tx)
{
    uint8 *ptr;
    uint16 p1, sn;
    uint8  p2, p3;
    if (wifi->icmp_mntr) {
        ptr = wifi_dev_scatter_offset(data, count, 12);
        if (ptr == NULL) return;
        p1 = get_unaligned_be16(ptr);

        ptr = wifi_dev_scatter_offset(data, count, 23);
        if (ptr == NULL) return;
        p2 = *ptr;

        if (p1 == 0x800 && p2 == 0x1) {
            ptr = wifi_dev_scatter_offset(data, count, 34);
            if (ptr == NULL) return;
            p3 = *ptr;
            if (p3 == 8 || p3 == 0) {
                ptr = wifi_dev_scatter_offset(data, count, 40);
                if (ptr == NULL) return;
                sn = get_unaligned_be16(ptr);
                os_printf(KERN_NOTICE"wifi: %s imcp %s, sn:%d\r\n", tx ? "TX" : "RX", p3 == 8 ? "Echo " : "Reply", sn);
            }
        }
    }
}
#else
#define wifi_dev_icmp_monitor(...)
#define wifi_dev_icmp_monitor_scatter(...)
#endif

#ifdef WIFI_BRIDGE_DEVLIST
static int32 wifi_dev_hook_input_data(struct netdev *ndev, uint8 *data, uint32 len)
{
    struct wifi_dev *wifi = (struct wifi_dev *)ndev;
    uint8 ifidx = wifi->ifidx == WIFI_MODE_APSTA ? WIFI_MODE_STA : wifi->ifidx;
    if (IS_MCAST_ADDR(data)) {
        return IEEE80211_PKTHDL_CONTINUE;
    }
    return ieee80211_hook_ext_data(ifidx, 1, data, len);
}

static struct mac_table_entry *wifi_brdev_get_mac_entry(struct wifi_dev *wifi, uint8 *addr)
{
    struct mac_table_entry *entry = NULL;
    struct list_head *head = &wifi->mac_table[ADDR_IDX(addr)];

    if (!list_empty(head)) {
        list_for_each_entry(entry, head, list) {
            if (MAC_EQU(entry->addr, addr)) {
                return entry;
            }
        }
    }
    return NULL;
}

static struct mac_table_entry *wifi_brdev_del_mac_entry(struct wifi_dev *wifi, uint8 *addr)
{
    struct mac_table_entry *entry;

    os_mutex_lock(&wifi->lock, osWaitForever);
    entry = wifi_brdev_get_mac_entry(wifi, addr);
    if (entry) {
        list_del(&entry->list);
        os_free(entry);
        wifi->mac_count--;
    }
    os_mutex_unlock(&wifi->lock);
    return NULL;
}

static void wifi_brdev_clear_mac_entry(struct wifi_dev *wifi)
{
    int32 i = 0;
    struct list_head *head;
    struct mac_table_entry *pos, *n;

    os_mutex_lock(&wifi->lock, osWaitForever);
    for (i = 0; i < 16; i++) {
        head = &wifi->mac_table[i];
        list_for_each_entry_safe(pos, n, head, list) {
            list_del(&pos->list);
            os_free(pos);
            wifi->mac_count--;
        }
    }
    os_mutex_unlock(&wifi->lock);
}

static int32 wifi_brdev_need_forward(struct wifi_dev *wifi, struct netdev *brdev, uint8 *dest)
{
    int32 ret = 0;
    struct mac_table_entry *entry = NULL;

    if (IS_MCAST_ADDR(dest)) {
        return 1;
    }

    os_mutex_lock(&wifi->lock, osWaitForever);
    entry = wifi_brdev_get_mac_entry(wifi, dest);
    ret   = (entry && entry->ndev == brdev);
    os_mutex_unlock(&wifi->lock);
    return ret;
}

static void wifi_brdev_mac_table_expired(struct wifi_dev *wifi)
{
    int32 i = 0;
    struct list_head *head;
    struct mac_table_entry *entry;
    uint32 tmo = wifi->mac_count > MAC_ENTRY_COUNT_WARNING ? 500 : MAC_ENTRY_TMO;

    for (i = 0; i < 16; i++) {
        head  = &wifi->mac_table[i];
        entry = list_first_entry_or_null(head, struct mac_table_entry, list);
        if (entry && TIME_AFTER(os_jiffies(), entry->lifetime + os_msecs_to_jiffies(tmo))) {
            list_del(&entry->list);
            os_free(entry);
            wifi->mac_count--;
        }
    }
}

static void wifi_brdev_update_mac_table(struct wifi_dev *wifi, struct netdev *ndev, uint8 *addr)
{
    struct mac_table_entry *entry;
    struct list_head *head = &wifi->mac_table[ADDR_IDX(addr)];

    os_mutex_lock(&wifi->lock, osWaitForever);
    wifi_brdev_mac_table_expired(wifi);

    entry = wifi_brdev_get_mac_entry(wifi, addr);
    if (entry) {
        entry->ndev = ndev;
        entry->lifetime = os_jiffies();
        list_del(&entry->list);
        list_add(&entry->list, head);
    } else if (wifi->mac_count < MAC_ENTRY_COUNT_MAX) {
        entry = os_malloc(sizeof(struct mac_table_entry));
        if (entry) {
            entry->ndev = ndev;
            entry->lifetime = os_jiffies();
            os_memcpy(entry->addr, addr, 6);
            list_add(&entry->list, head);
            wifi->mac_count++;
        }
    }
    os_mutex_unlock(&wifi->lock);
}

static void wifi_brdev_save_fwdata(struct wifi_dev *wifi, struct netdev *ndev, uint8 *data, uint32 size)
{
    struct sk_buff *skb = alloc_tx_skb(size);
    if (skb) {
        skb->sta = ndev;
        hw_memcpy(skb->data, data, size);
        skb_put(skb, size);
        skb_list_queue(&wifi->fw_list, skb);
        os_sema_up(&wifi->fw_sema);
    }
}

static void wifi_brdev_forward_task(void *arg)
{
    struct sk_buff  *skb;
    struct wifi_dev *wifi = (struct wifi_dev *)arg;

    while (1) {
        os_sema_down(&wifi->fw_sema, osWaitForever);
        skb = skb_list_dequeue(&wifi->fw_list);
        if (skb) {
            netdev_send_data((struct netdev *)skb->sta, skb->data, skb->len);
            kfree_skb(skb);
        }
    }
}

static int32 wifi_brdev_list_forward(struct wifi_dev *wifi, uint8 *data, uint32 len, int32 *err)
{
    int32 i = 0;
    uint8 mcast = IS_MCAST_ADDR(data);

    if (!wifi->bridge_mode) {
        return 0;
    }

    for (i = 0; i < wifi->brdev_cnt; i++) {
        if (wifi->brdev_list[i] && wifi_brdev_need_forward(wifi, wifi->brdev_list[i], data)) {
            *err = netdev_send_data(wifi->brdev_list[i], data, len);
            if (!mcast) {
                return 1;
            }
        }
    }
    return 0;
}

static int32 wifi_brdev_list_forward_scatter(struct wifi_dev *wifi, scatter_data *data, uint32 count, int32 *err)
{
    int32 i = 0;
    uint8 mcast = IS_MCAST_ADDR(data[0].addr);

    if (!wifi->bridge_mode) {
        return 0;
    }

    for (i = 0; i < wifi->brdev_cnt; i++) {
        if (wifi->brdev_list[i] && wifi_brdev_need_forward(wifi, wifi->brdev_list[i], data[0].addr)) {
            *err = netdev_send_scatter_data(wifi->brdev_list[i], data, count);
            if (!mcast) {
                return 1;
            }
        }
    }
    return 0;
}

static void wifi_brdev_input_cb(struct netdev *ndev, uint8 *data, uint32 size, void *priv)
{
    int32 i = 0;
    uint32 mcast;
    void  *input_priv;
    netdev_input_cb input_cb;
    struct wifi_dev *wifi = (struct wifi_dev *)priv;
    uint8 ifidx = (wifi->ifidx == WIFI_MODE_APSTA ? 0 : wifi->ifidx);

    if (!wifi->bridge_mode) {
        return;
    }

    mcast = disable_irq();
    input_cb   = wifi->input_cb;
    input_priv = wifi->input_priv;
    enable_irq(mcast);

    mcast = IS_MCAST_ADDR(data);
    wifi_brdev_update_mac_table(wifi, ndev, data + 6);

    if (input_cb && (mcast || MAC_EQU(data, wifi->addr))) {
        wifi_dev_icmp_monitor(wifi, data, 0);
        input_cb(&wifi->wifi, data, size, input_priv);
        if (!mcast) {
            return;
        }
    }

    for (i = 0; wifi->brdev_cnt > 1 && i < wifi->brdev_cnt; i++) {
        if (ndev != wifi->brdev_list[i] && wifi_brdev_need_forward(wifi, wifi->brdev_list[i], data)) {
            wifi_brdev_save_fwdata(wifi, wifi->brdev_list[i], data, size);
            if (!mcast) {
                return;
            }
        }
    }

    wifi->tx_data++;
    wifi->wifi.tx_bytes += size;
    ieee80211_tx(ifidx, data, size);
}

static int32 wifi_brdev_work(struct os_work *work)
{
    struct wifi_dev *wifi = container_of(work, struct wifi_dev, work);

    os_mutex_lock(&wifi->lock, osWaitForever);
    wifi_brdev_mac_table_expired(wifi);
    os_mutex_unlock(&wifi->lock);
    os_run_work_delay(work, 500);
    return 0;
}

static void wifi_brdev_open_close(struct wifi_dev *wifi, uint8 open)
{
    int32 i = 0;
    for (i = 0; i < wifi->brdev_cnt; i++) {
        if (open) {
            netdev_open(wifi->brdev_list[i], wifi_brdev_input_cb, NULL, wifi);
        } else {
            netdev_close(wifi->brdev_list[i]);
        }
    }
}
#endif

static int32 wifi_dev_open(struct netdev *ndev, netdev_input_cb cb, netdev_event_cb evt_cb, void *priv)
{
    uint32 flags;
    struct wifi_dev *wifi = (struct wifi_dev *)ndev;
    uint8 ifidx = (wifi->ifidx == WIFI_MODE_APSTA ? WIFI_MODE_STA : wifi->ifidx);

    if (cb) {
        flags = disable_irq();
        wifi->input_cb   = cb;
        wifi->input_priv = priv;
        enable_irq(flags);
    }

#ifdef WIFI_BRIDGE_DEVLIST
    wifi_brdev_open_close(wifi, 1);
    os_run_work_delay(&wifi->work, 500);
#endif

    return ieee80211_iface_start(ifidx);
}

static int32 wifi_dev_close(struct netdev *ndev)
{
    struct wifi_dev *wifi = (struct wifi_dev *)ndev;
    uint8 ifidx = (wifi->ifidx == WIFI_MODE_APSTA ? WIFI_MODE_STA : wifi->ifidx);

#ifdef WIFI_BRIDGE_DEVLIST
    wifi_brdev_open_close(wifi, 0);
    wifi_brdev_clear_mac_entry(wifi);
#endif

    return ieee80211_iface_stop(ifidx);
}

static int32 wifi_dev_send(struct netdev *ndev, uint8 *data, uint32 size)
{
    struct wifi_dev *wifi = (struct wifi_dev *)ndev;
    uint8 ifidx = (wifi->ifidx == WIFI_MODE_APSTA ? 0 : wifi->ifidx);

    wifi_dev_icmp_monitor(wifi, data, 1);

#ifdef WIFI_BRIDGE_DEVLIST
    int32 ret = RET_OK;
    if (wifi_brdev_list_forward(wifi, data, size, &ret)) {
        return ret;
    }
#endif

    wifi->tx_data++;
    wifi->wifi.tx_bytes += size;
    return ieee80211_tx(ifidx, (uint8 *)data, size);
}

static int32 wifi_dev_scatter_send(struct netdev *ndev, scatter_data *data, uint32 count)
{
    struct wifi_dev *wifi = (struct wifi_dev *)ndev;
    uint8 ifidx = (wifi->ifidx == WIFI_MODE_APSTA ? 0 : wifi->ifidx);

    wifi_dev_icmp_monitor_scatter(wifi, data, count, 1);

#ifdef WIFI_BRIDGE_DEVLIST
    int32 ret = RET_OK;
    if (wifi_brdev_list_forward_scatter(wifi, data, count, &ret)) {
        return ret;
    }
#endif

    wifi->tx_data++;
    wifi->wifi.tx_bytes += scatter_data_size(data, count);
    return ieee80211_scatter_tx(ifidx, data, count);
}

static int32 wifi_dev_ioctl(struct netdev *ndev, uint32 cmd, uint32 param1, uint32 param2)
{
    int32 ret = RET_OK;
    struct wifi_dev *wifi = (struct wifi_dev *)ndev;
    uint8 ifidx = (wifi->ifidx == WIFI_MODE_APSTA ? WIFI_MODE_STA : wifi->ifidx);

    switch (cmd) {
        case NETDEV_IOCTL_GET_ADDR:
            ieee80211_conf_get_mac(ifidx, wifi->addr);
            os_memcpy((uint8 *)param1, wifi->addr, 6);
            break;
        case NETDEV_IOCTL_SET_ADDR:
            ieee80211_conf_set_mac(ifidx, (uint8 *)param1);
            os_memcpy(wifi->addr, (uint8 *)param1, 6);
            break;
        case NETDEV_IOCTL_GET_LINK_STATUS:
            ret = (ieee80211_conf_get_connstate(ifidx) >= WPA_GROUP_HANDSHAKE);
            break;
        case NETDEV_IOCTL_GET_LINK_SPEED:
            break;
#if WIFI_SINGLE_DEV
        case NETDEV_IOCTL_SET_WIFI_MODE:
            wifi->ifidx = (uint8)param1;
            break;
#endif
#ifdef WIFI_BRIDGE_DEVLIST
        case NETDEV_IOCTL_HOOK_INPUTDATA:
            ret = !wifi_dev_hook_input_data(ndev, (uint8 *)param1, param2);
            break;
        case NETDEV_IOCTL_ENABLE_WIFIBRIDGE:
            wifi->bridge_mode = param1 ? 1 : 0;
            wifi_brdev_open_close(wifi, param1);
            break;
#endif
        case NETDEV_IOCTL_ENABLE_ICMPMNTR:
            wifi->icmp_mntr = param1 ? 1 : 0;
            break;
#ifdef WIFI_BRIDGE_DEVLIST
        case NETDEV_IOCTL_CLEAR_ROUTETBL:
            wifi_brdev_clear_mac_entry(wifi);
            break;
#endif
        default:
            ret = -ENOTSUPP;
            break;
    }
    return ret;
}

int32 sys_wifi_recv(void *priv, uint8 *data, uint32 len, uint32 flags)
{
    struct wifi_dev *wifi  = (struct wifi_dev *)priv;
    int32 ret = RET_OK;
    netdev_input_cb input_cb;
    void           *input_priv;
    uint8 mcast = IS_MCAST_ADDR(data);

    wifi->wifi.rx_bytes += len;
    wifi->rx_data++;

    ret = disable_irq();
    input_cb   = wifi->input_cb;
    input_priv = wifi->input_priv;
    enable_irq(ret);
    ret = RET_OK;

#ifdef WIFI_BRIDGE_DEVLIST
    wifi_brdev_del_mac_entry(wifi, data + 6);
#endif

    if (input_cb && (mcast || MAC_EQU(data, wifi->addr))) {
        wifi_dev_icmp_monitor(wifi, data, 0);
        input_cb(&wifi->wifi, data, len, input_priv);
        if (!mcast) {
            return RET_OK;
        }
    }

#ifdef WIFI_BRIDGE_DEVLIST
    ret = wifi_brdev_list_forward(wifi, data, len, &ret);
    if(ret == 0 && !mcast && wifi->bridge_mode){ //NO Route: send to all interface.
        int8 i = 0;
        for (i = 0; i < wifi->brdev_cnt; i++) {
           netdev_send_data(wifi->brdev_list[i], data, len);
        }
        wifi_dev_icmp_monitor(wifi, data, len);
    }
#endif
    return ret;
}

static const struct netdev_hal_ops wifi_dev_ops = {
    .open      = wifi_dev_open,
    .close     = wifi_dev_close,
    .ioctl     = wifi_dev_ioctl,
    .send_data = wifi_dev_send,
    .send_scatter_data = wifi_dev_scatter_send,
};

__init void *sys_wifi_register(uint32 ifidx)
{
#ifdef WIFI_BRIDGE_DEVLIST
    int32 i = 0;
    uint32 brdev_list[] = {WIFI_BRIDGE_DEVLIST};
    uint32 brdev_size = ARRAY_SIZE(brdev_list) * sizeof(struct netdev *);
#else
    uint32 brdev_size = 0;
#endif

    struct wifi_dev *wifi;
    uint32 dev_id = HG_WIFI0_DEVID + ifidx - 1;

#if WIFI_SINGLE_DEV
    dev_id = HG_WIFI0_DEVID;
    wifi   = (struct wifi_dev *)dev_get(dev_id);
    if (wifi) {
        return wifi;
    }
#endif

    wifi = (struct wifi_dev *)os_zalloc(sizeof(struct wifi_dev) + brdev_size);
    ASSERT(wifi);
    wifi->wifi.dev.ops = (const struct devobj_ops *)&wifi_dev_ops;
    wifi->ifidx        = (uint8)ifidx;

#ifdef WIFI_BRIDGE_DEVLIST
    os_mutex_init(&wifi->lock);
    os_sema_init(&wifi->fw_sema, 0);
    skb_list_init(&wifi->fw_list);
    OS_WORK_INIT(&wifi->work, wifi_brdev_work, 0);
    wifi->bridge_mode = 1;
    wifi->brdev_cnt = ARRAY_SIZE(brdev_list);
    for (i = 0; i < 16; i++) {
        INIT_LIST_HEAD(&wifi->mac_table[i]);
    }
    os_printf(KERN_NOTICE"wifi brigde device:");
    for (i = 0; i < wifi->brdev_cnt; i++) {
        wifi->brdev_list[i] = (struct netdev *)dev_get(brdev_list[i]);
        _os_printf("%d,", brdev_list[i]);
        ASSERT(wifi->brdev_list[i]);
    }
    _os_printf("\r\n");
    OS_TASK_INIT("fwtsk", &wifi->fw_task, wifi_brdev_forward_task, wifi, OS_TASK_PRIORITY_HIGH, 1024);
#endif

    ieee80211_conf_get_mac(wifi->ifidx, wifi->addr);
    dev_register(dev_id, (struct dev_obj *)wifi);
    return wifi;
}

void wifi_dev_status(uint32 dev_id)
{
    struct wifi_dev *wifi = (struct wifi_dev *)dev_get(dev_id);;

    if (wifi == NULL) {
        return;
    }

    os_printf("WiFi Dev Status: no_mem=%u, bridge:%d\r\n", wifi->no_mem, wifi->bridge_mode);
    os_printf("    wifi: tx:%d, rx:%d, tx_bytes:%llu, rx_bytes:%llu\r\n", wifi->tx_data, wifi->rx_data, wifi->wifi.tx_bytes, wifi->wifi.rx_bytes);
    wifi->rx_data = 0;
    wifi->tx_data = 0;
#ifdef WIFI_BRIDGE_DEVLIST
    int32 i = 0;
    os_printf("    fw_list: %d, mac_count:%d (max:%d), brdev_cnt:%d\r\n",
              wifi->fw_list.count, wifi->mac_count, MAC_ENTRY_COUNT_MAX, wifi->brdev_cnt);
    for (i = 0; i < wifi->brdev_cnt; i++) {
        if (wifi->brdev_list[i]) {
            os_printf("    brdev %d: tx:%llu, rx:%llu\r\n",
                      wifi->brdev_list[i]->dev.dev_id, wifi->brdev_list[i]->tx_bytes, wifi->brdev_list[i]->rx_bytes);
        }
    }
#endif
}

#ifdef FMAC_EN
__weak void hgic_custom_driver_data_handler(uint8 *data, uint16 len)
{
    // do nothing default
}
#endif
