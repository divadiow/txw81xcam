#include "typesdef.h"
#include "list.h"
#include "sys_config.h"
#include "errno.h"
#include "dev.h"
#include "devid.h"

#include "osal/string.h"
#include "syscfg.h"
#include "osal/task.h"
#include "osal/sleep.h"

#include "hal/dma.h"
#include "hal/crc.h"

#include "lib/ble/ble_adv.h"
#include "lib/lmac/lmac.h"
#include "lib/common/common.h"
#include "lib/heap/sysheap.h"
#include "lib/ble/ble_def.h"
#include "lib/bluetooth/bluetooth.h"
#include "lib/bluetooth/hci.h"
#include "lib/bluetooth/hci_cmd.h"
#include "lib/bluetooth/hci_event.h"
#include "lib/skb/skb.h"
#include "lib/umac/ieee80211.h"
#include "lib/umac/umac.h"
#include "lib/ble/ble_demo.h"

struct ble_rx_ctrl *ble_ctrl;
const uint8 adv_identify_info[ADV_IDENTIFY_SET_LEN] = {0x9f, 0x1a, 0x41, 0x10, 0x7d, 0xfd, 0xf0, 0x73}; // 识别头信息
const uint8 adv_addr[6] = {0x70, 0xaf, 0x1e, 0x7f, 0x0e, 0x7e}; // 广播地址

int32 ble_adv_parse_data(void *p, uint8 *data, int32 len)
{
	if(len <= ADV_DATA_MAX_LEN) {
		memcpy(&ble_ctrl->adv_info, data, len);
	}else {
		return 0;
	}
	if(ble_ctrl->adv_info.header_info.pdu_type != ADV_DISCOVER_TYPE) {   
		return 0;
	} 
	if(ble_ctrl->adv_info.payload_info.manufacturer_id != ADV_MANUFACTURER_ID) { 
		return 0;
	}
	#if (ADV_IDENTIFY_SET_LEN > 0 && ADV_IDENTIFY_SET_LEN < ADV_IDENTIFY_MAX_LEN)
	if(memcmp(ble_ctrl->adv_info.payload_info.identify_info, adv_identify_info, ADV_IDENTIFY_SET_LEN) != 0) {
		return 0;
	}
	#endif
	
	//接收超时
	if(os_jiffies_to_msecs(os_jiffies())-ble_ctrl->adv_pro.cur_tick >= ble_ctrl->adv_pro.rec_overtime) {
		if(ble_ctrl->adv_info.payload_info.section_idx == 0) {
			ble_ctrl->adv_pro.cur_tick = os_jiffies_to_msecs(os_jiffies());
			ble_ctrl->adv_pro.section_num = ble_ctrl->adv_info.payload_info.section_num;
			ble_ctrl->adv_pro.rec_overtime = (ble_ctrl->adv_pro.section_num+1)*1000; //ms
			ble_ctrl->adv_pro.section_idx = 0;
			ble_ctrl->adv_pro.byte_offset_len = 0;
			ble_ctrl->adv_pro.len = 0;
			ble_ctrl->adv_pro.start_flag = 1;
		}else {
			ble_ctrl->adv_pro.start_flag = 0;
		}
	}
	if((ble_ctrl->adv_pro.start_flag == 1) && (ble_ctrl->adv_info.payload_info.section_num == ble_ctrl->adv_pro.section_num))   {
		if(ble_ctrl->adv_info.payload_info.section_idx == ble_ctrl->adv_pro.section_idx) {
			memcpy(ble_ctrl->adv_pro.data + ble_ctrl->adv_pro.byte_offset_len, ble_ctrl->adv_info.payload_info.data, ble_ctrl->adv_info.payload_info.byte_len);
			ble_ctrl->adv_pro.byte_offset_len += ble_ctrl->adv_info.payload_info.byte_len;
			++ble_ctrl->adv_pro.section_idx;
		}
		if(ble_ctrl->adv_info.payload_info.section_num == ble_ctrl->adv_pro.section_idx) {
			ble_ctrl->adv_pro.len = ble_ctrl->adv_pro.byte_offset_len;
			ble_ctrl->adv_pro.start_flag = 0;
			return 1;
		}
	}
	return 0;
}

int32 ble_adv_get_data(uint8 **data)
{
	if(ble_ctrl->adv_pro.data == NULL) {
		return 0;
	}else {
		*data = ble_ctrl->adv_pro.data;
		return ble_ctrl->adv_pro.len;
	}
}
	
int32 ble_adv_tx_data(void *p, uint8 *data, int32 len)
{
	uint8 section_num = 0;
	uint8 section_idx = 0;
	uint16 last_section_len = 0;
	uint16 start_pos = 0;
	uint16 cur_section_len = 0;
	struct ble_adv_info adv_info;
	struct sk_buff *skb;

	if((data == NULL) || (len <= 0) || (len > 250 * ADV_MAX_SECTION_LEN)) { //section_num:1byte(255)
		return RET_ERR;
	}
	
	section_num = (len + ADV_MAX_SECTION_LEN - 1) / ADV_MAX_SECTION_LEN;
	last_section_len = len - (section_num - 1) * ADV_MAX_SECTION_LEN;
	adv_info.header_info.pdu_type = ADV_DISCOVER_TYPE;
	adv_info.header_info.tx_add = 1;
	adv_info.header_info.rx_add = 0;
	memcpy(adv_info.payload_info.addr, adv_addr, 6);
	adv_info.payload_info.ad_type = 0xFF;
	adv_info.payload_info.manufacturer_id = ADV_MANUFACTURER_ID;
	memcpy(adv_info.payload_info.identify_info, adv_identify_info, ADV_IDENTIFY_SET_LEN);

	
	while(1) {
		start_pos = section_idx * ADV_MAX_SECTION_LEN;
		if(section_idx < section_num - 1) {
			cur_section_len = ADV_MAX_SECTION_LEN;
		}else {
			cur_section_len = last_section_len;
		}
		adv_info.header_info.length = 6 + 4 + ADV_IDENTIFY_SET_LEN + 3 + cur_section_len; //addr+(1E FF 04 41)+ADV_IDENTIFY_SET_LEN+section_num+section_idx+byte_len+data
		adv_info.payload_info.ad_len = 3 + ADV_IDENTIFY_SET_LEN + 3 + cur_section_len;   //(FF 04 41)+ADV_IDENTIFY_SET_LEN+section_num+section_idx+byte_len+data
		adv_info.payload_info.section_num = section_num;
		adv_info.payload_info.section_idx = section_idx;
		adv_info.payload_info.byte_len = cur_section_len;
		memcpy(adv_info.payload_info.data, data + start_pos, cur_section_len);
		//lmac_ble_tx(p, &adv_info, adv_info.header_info.length + 2); // adv_header(2byte)
		
		skb = alloc_tx_skb(adv_info.header_info.length + 2);	// adv_header(2byte)
		if (ble_ctrl->ops && skb) {
			os_memcpy(skb->data, &adv_info, adv_info.header_info.length + 2);
			skb->len = adv_info.header_info.length + 2;
			if (ble_ctrl->ops->tx(ble_ctrl->ops, skb)) {
				kfree_skb(skb);
				return RET_ERR;
			}
			kfree_skb(skb);
		}
		section_idx = section_idx + 1;
		if(section_idx >= section_num) {
			break;
		}
	}
	return RET_OK;
}

static int32 ble_adv_rx_data(void *priv, struct sk_buff *skb)
{
	//struct lmac_host *lmachost = (struct lmac_host *)priv;
    struct bt_rx_info  *rxinfo = (struct bt_rx_info *)skb->cb;
    uint8 *p = NULL;
	uint8 *ncdata = NULL;
    uint32 data_len = 0;

	//os_printf("ble_adv_rx_data\r\n");
    
    if ((rxinfo->frm_type & 0x80)) {
		//dump_hex("ble adv rx adv_data:\r\n", skb->data, skb->len, 1);
    	if (ble_adv_parse_data(p,skb->data, skb->len)) {
	   		data_len = ble_adv_get_data(&ncdata);
	   		if (data_len && ncdata) {
	       		ble_adv_parse_param(ncdata, data_len);
	   		}
   		}
    }
    return RET_OK;
}

int32 ble_adv_init(struct bt_ops *ops)
{
	if (ble_ctrl == NULL) {
		ble_ctrl = (struct ble_rx_ctrl *)os_zalloc(sizeof(struct ble_rx_ctrl));
	}
	if (ops) {
        ble_ctrl->ops = ops;
        ops->rx = ble_adv_rx_data;
        ops->priv = ble_ctrl;
    }

	return ble_ctrl ? RET_OK : RET_ERR;
}

int32 ble_adv_deinit(void)
{
	if(ble_ctrl) {
		os_free(ble_ctrl);
	}
	return RET_OK;
}
