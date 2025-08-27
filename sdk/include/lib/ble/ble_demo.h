/**
  ******************************************************************************
  * @file    sdk\include\lib\ble
  * @author  TAIXIN-SEMI Application Team
  * @version V1.0.0
  * @date    24-07-2023
  * @brief   This file contains three BLE configuration network features.
  ******************************************************************************
  * @attention
  *
  * COPYRIGHT 2023 TAIXIN-SEMI
  *
  ******************************************************************************
  */ 
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BLE_DEMO_H
#define __BLE_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "lib/ble/ble_def.h"

enum switch_type
{
    WIRELESS_WIFI,
    WIRELESS_BLE,
};

int32 ble_set_coexist_en(struct bt_ops *btops, uint8 coexist, uint8 dec_duty);
int32 ble_get_en_status(struct bt_ops *btops);

void  ble_adv_parse_param(uint8 *data, int len);
int32 ble_demo_mode1_init(struct bt_ops *bt_ops);
int32 ble_demo_mode2_init(struct bt_ops *bt_ops);
int32 ble_demo_mode3_init(struct bt_ops *bt_ops);
int32 ble_demo_start(struct bt_ops *bt_ops, uint8 type);
int32 ble_demo_stop(struct bt_ops *bt_ops);

#ifdef __cplusplus
}
#endif
#endif
/*************************** (C) COPYRIGHT 2023 TAIXIN-SEMI ***** END OF FILE *****/
