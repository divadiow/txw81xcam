/*
 * Copyright (C) 2014 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at 
 * contact@bluekitchen-gmbh.com
 *
 */

/**
 * @title Host Controler Interface (HCI)
 *
 */

#ifndef HCI_H
#define HCI_H

#include "lib/bluetooth/hci_cmd.h"

#if defined __cplusplus
extern "C" {
#endif
     
// packet buffer sizes
#define HCI_CMD_HEADER_SIZE          3
#define HCI_ACL_HEADER_SIZE          4
#define HCI_SCO_HEADER_SIZE          3
#define HCI_EVENT_HEADER_SIZE        2
#define HCI_ISO_HEADER_SIZE          4

#define HCI_EVENT_PAYLOAD_SIZE     255
#define HCI_CMD_PAYLOAD_SIZE       255

//#define little_endian_read_16(p,o) get_unaligned_le16((p)+(o))

#define IS_COMMAND(packet, command) ( little_endian_read_16(packet,0) == command.opcode )

// check if command complete event for given command
#define HCI_EVENT_IS_COMMAND_COMPLETE(event,cmd) ( (event[0] == HCI_EVENT_COMMAND_COMPLETE) && (little_endian_read_16(event,3) == cmd.opcode) )
#define HCI_EVENT_IS_COMMAND_STATUS(event,cmd)   ( (event[0] == HCI_EVENT_COMMAND_STATUS)   && (little_endian_read_16(event,4) == cmd.opcode) )

// Code+Len=2, Pkts+Opcode=3; total=5
#define OFFSET_OF_DATA_IN_COMMAND_COMPLETE 5

// ACL Packet
#define READ_ACL_CONNECTION_HANDLE( buffer ) ( little_endian_read_16(buffer,0) & 0x0fff)
#define READ_SCO_CONNECTION_HANDLE( buffer ) ( little_endian_read_16(buffer,0) & 0x0fff)
#define READ_ISO_CONNECTION_HANDLE( buffer ) ( little_endian_read_16(buffer,0) & 0x0fff)
#define READ_ACL_FLAGS( buffer )      ( buffer[1] >> 4 )
#define READ_ACL_LENGTH( buffer )     (little_endian_read_16(buffer, 2))

// Sneak peak into L2CAP Packet
#define READ_L2CAP_LENGTH(buffer)     ( little_endian_read_16(buffer, 4))
#define READ_L2CAP_CHANNEL_ID(buffer) ( little_endian_read_16(buffer, 6))

/**
 * LE connection parameter update state
 */ 

typedef enum {
    CON_PARAMETER_UPDATE_NONE,
    // L2CAP
    CON_PARAMETER_UPDATE_SEND_REQUEST,
    CON_PARAMETER_UPDATE_SEND_RESPONSE,
    CON_PARAMETER_UPDATE_CHANGE_HCI_CON_PARAMETERS,
    CON_PARAMETER_UPDATE_DENY,
    // HCI - in respnose to HCI_SUBEVENT_LE_REMOTE_CONNECTION_PARAMETER_REQUEST
    CON_PARAMETER_UPDATE_REPLY,
    CON_PARAMETER_UPDATE_NEGATIVE_REPLY,
} le_con_parameter_update_state_t;

// Authentication flags
typedef enum {
    AUTH_FLAG_NONE                                = 0x0000,
    AUTH_FLAG_HANDLE_LINK_KEY_REQUEST             = 0x0001,
    AUTH_FLAG_DENY_PIN_CODE_REQUEST               = 0x0002,
    AUTH_FLAG_RECV_IO_CAPABILITIES_REQUEST        = 0x0004,
    AUTH_FLAG_RECV_IO_CAPABILITIES_RESPONSE       = 0x0008,
    AUTH_FLAG_SEND_IO_CAPABILITIES_REPLY          = 0x0010,
    AUTH_FLAG_SEND_IO_CAPABILITIES_NEGATIVE_REPLY = 0x0020,
    AUTH_FLAG_SEND_USER_CONFIRM_REPLY             = 0x0040,
    AUTH_FLAG_SEND_USER_CONFIRM_NEGATIVE_REPLY    = 0x0080,
    AUTH_FLAG_SEND_USER_PASSKEY_REPLY             = 0x0100,

    // Classic OOB
    AUTH_FLAG_SEND_REMOTE_OOB_DATA_REPLY          = 0x0200,

    // pairing status
    AUTH_FLAG_LEGACY_PAIRING_ACTIVE               = 0x0400,
    AUTH_FLAG_SSP_PAIRING_ACTIVE                  = 0x0800,
    AUTH_FLAG_PAIRING_ACTIVE_MASK                 = (AUTH_FLAG_LEGACY_PAIRING_ACTIVE | AUTH_FLAG_SSP_PAIRING_ACTIVE),

    // connection status
    AUTH_FLAG_CONNECTION_AUTHENTICATED            = 0x1000,
    AUTH_FLAG_CONNECTION_ENCRYPTED                = 0x2000,

} hci_authentication_flags_t;

// GAP Connection Tasks
#define GAP_CONNECTION_TASK_WRITE_AUTOMATIC_FLUSH_TIMEOUT 0x0001u
#define GAP_CONNECTION_TASK_WRITE_SUPERVISION_TIMEOUT     0x0002u
#define GAP_CONNECTION_TASK_READ_RSSI                     0x0004u

/**
 * Connection State 
 */
typedef enum {
    SEND_CREATE_CONNECTION = 0,
    SENT_CREATE_CONNECTION,
    SEND_CANCEL_CONNECTION,
    SENT_CANCEL_CONNECTION,
    RECEIVED_CONNECTION_REQUEST,
    ACCEPTED_CONNECTION_REQUEST,
    REJECTED_CONNECTION_REQUEST,
    OPEN,
    SEND_DISCONNECT,
    SENT_DISCONNECT,
    RECEIVED_DISCONNECTION_COMPLETE
} CONNECTION_STATE;

// bonding flags
enum {
    // remote features
    BONDING_REMOTE_FEATURES_QUERY_ACTIVE      =  0x0001,
    BONDING_REQUEST_REMOTE_FEATURES_PAGE_0    =  0x0002,
    BONDING_REQUEST_REMOTE_FEATURES_PAGE_1    =  0x0004,
    BONDING_REQUEST_REMOTE_FEATURES_PAGE_2    =  0x0008,
    BONDING_RECEIVED_REMOTE_FEATURES          =  0x0010,
    BONDING_REMOTE_SUPPORTS_SSP_CONTROLLER    =  0x0020,
    BONDING_REMOTE_SUPPORTS_SSP_HOST          =  0x0040,
    BONDING_REMOTE_SUPPORTS_SC_CONTROLLER     =  0x0080,
    BONDING_REMOTE_SUPPORTS_SC_HOST           =  0x0100,
    // other
    BONDING_DISCONNECT_SECURITY_BLOCK         =  0x0200,
    BONDING_DISCONNECT_DEDICATED_DONE         =  0x0400,
    BONDING_SEND_AUTHENTICATE_REQUEST         =  0x0800,
    BONDING_SENT_AUTHENTICATE_REQUEST         =  0x1000,
    BONDING_SEND_ENCRYPTION_REQUEST           =  0x2000,
    BONDING_SEND_READ_ENCRYPTION_KEY_SIZE     =  0x4000,
    BONDING_DEDICATED                         =  0x8000,
    BONDING_EMIT_COMPLETE_ON_DISCONNECT       = 0x10000,
};

typedef enum {
    BLUETOOTH_OFF = 1,
    BLUETOOTH_ON,
    BLUETOOTH_ACTIVE
} BLUETOOTH_STATE;

typedef enum {
    LE_CONNECTING_IDLE,
    LE_CONNECTING_CANCEL,
    LE_CONNECTING_DIRECT,
    LE_CONNECTING_WHITELIST,
} le_connecting_state_t;

//
// SM internal types and globals
//

typedef enum {

    // general states
    SM_GENERAL_IDLE,
    SM_GENERAL_SEND_PAIRING_FAILED,
    SM_GENERAL_TIMEOUT, // no other security messages are exchanged
    SM_GENERAL_REENCRYPTION_FAILED,

    // Phase 1: Pairing Feature Exchange
    SM_PH1_W4_USER_RESPONSE,

    // Phase 2: Authenticating and Encrypting

    // get random number for use as TK Passkey if we show it 
    SM_PH2_GET_RANDOM_TK,
    SM_PH2_W4_RANDOM_TK,

    // get local random number for confirm c1
    SM_PH2_C1_GET_RANDOM_A,
    SM_PH2_C1_W4_RANDOM_A,
    SM_PH2_C1_GET_RANDOM_B,
    SM_PH2_C1_W4_RANDOM_B,

    // calculate confirm value for local side
    SM_PH2_C1_GET_ENC_A,
    SM_PH2_C1_W4_ENC_A,

    // calculate confirm value for remote side
    SM_PH2_C1_GET_ENC_C,
    SM_PH2_C1_W4_ENC_C,

    SM_PH2_C1_SEND_PAIRING_CONFIRM,
    SM_PH2_SEND_PAIRING_RANDOM,

    // calc STK
    SM_PH2_CALC_STK,
    SM_PH2_W4_STK,

    SM_PH2_W4_CONNECTION_ENCRYPTED,

    // Phase 3: Transport Specific Key Distribution
    // calculate DHK, Y, EDIV, and LTK
    SM_PH3_Y_GET_ENC,
    SM_PH3_Y_W4_ENC,
    SM_PH3_LTK_GET_ENC,

    // exchange keys
    SM_PH3_DISTRIBUTE_KEYS,
    SM_PH3_RECEIVE_KEYS,

    // Phase 4: re-establish previously distributed LTK
    SM_PH4_W4_CONNECTION_ENCRYPTED,

    // RESPONDER ROLE
    SM_RESPONDER_IDLE,
    SM_RESPONDER_SEND_SECURITY_REQUEST,
    SM_RESPONDER_PH0_RECEIVED_LTK_REQUEST,
    SM_RESPONDER_PH0_RECEIVED_LTK_W4_IRK,
    SM_RESPONDER_PH0_SEND_LTK_REQUESTED_NEGATIVE_REPLY,
    SM_RESPONDER_PH1_W4_PAIRING_REQUEST,
    SM_RESPONDER_PH1_PAIRING_REQUEST_RECEIVED,
    SM_RESPONDER_PH1_PAIRING_REQUEST_RECEIVED_W4_IRK,
    SM_RESPONDER_PH1_SEND_PAIRING_RESPONSE,
    SM_RESPONDER_PH1_W4_PAIRING_CONFIRM,
    SM_RESPONDER_PH2_W4_PAIRING_RANDOM,
    SM_RESPONDER_PH2_W4_LTK_REQUEST,
    SM_RESPONDER_PH2_SEND_LTK_REPLY,
    SM_RESPONDER_PH4_Y_W4_ENC,
    SM_RESPONDER_PH4_SEND_LTK_REPLY,

    // INITIATOR ROLE
    SM_INITIATOR_CONNECTED,
    SM_INITIATOR_PH1_W2_SEND_PAIRING_REQUEST,
    SM_INITIATOR_PH1_W4_PAIRING_RESPONSE,
    SM_INITIATOR_PH2_W4_PAIRING_CONFIRM,
    SM_INITIATOR_PH2_W4_PAIRING_RANDOM,
    SM_INITIATOR_PH3_SEND_START_ENCRYPTION,
    SM_INITIATOR_PH4_HAS_LTK,

    // LE Secure Connections
    SM_SC_RECEIVED_LTK_REQUEST,
    SM_SC_SEND_PUBLIC_KEY_COMMAND,
    SM_SC_W4_PUBLIC_KEY_COMMAND,
    SM_SC_W4_LOCAL_NONCE,
    SM_SC_W2_CMAC_FOR_CONFIRMATION,
    SM_SC_W4_CMAC_FOR_CONFIRMATION,
    SM_SC_SEND_CONFIRMATION,
    SM_SC_W2_CMAC_FOR_CHECK_CONFIRMATION,    
    SM_SC_W4_CMAC_FOR_CHECK_CONFIRMATION,    
    SM_SC_W4_CONFIRMATION,
    SM_SC_SEND_PAIRING_RANDOM,
    SM_SC_W4_PAIRING_RANDOM,
    SM_SC_W2_CALCULATE_G2,
    SM_SC_W4_CALCULATE_G2,
    SM_SC_W4_CALCULATE_DHKEY,
    SM_SC_W2_CALCULATE_F5_SALT,
    SM_SC_W4_CALCULATE_F5_SALT,
    SM_SC_W2_CALCULATE_F5_MACKEY,
    SM_SC_W4_CALCULATE_F5_MACKEY,
    SM_SC_W2_CALCULATE_F5_LTK,
    SM_SC_W4_CALCULATE_F5_LTK,
    SM_SC_W2_CALCULATE_F6_FOR_DHKEY_CHECK,
    SM_SC_W4_CALCULATE_F6_FOR_DHKEY_CHECK,
    SM_SC_W2_CALCULATE_F6_TO_VERIFY_DHKEY_CHECK,
    SM_SC_W4_CALCULATE_F6_TO_VERIFY_DHKEY_CHECK,
    SM_SC_W4_USER_RESPONSE,
    SM_SC_SEND_DHKEY_CHECK_COMMAND,
    SM_SC_W4_DHKEY_CHECK_COMMAND,
    SM_SC_W4_LTK_REQUEST_SC,
    SM_SC_W2_CALCULATE_ILK_USING_H6,
	SM_SC_W2_CALCULATE_ILK_USING_H7,
    SM_SC_W4_CALCULATE_ILK,
    SM_SC_W2_CALCULATE_BR_EDR_LINK_KEY,
    SM_SC_W4_CALCULATE_BR_EDR_LINK_KEY,

    // Classic
    SM_BR_EDR_W4_ENCRYPTION_COMPLETE,
    SM_BR_EDR_INITIATOR_SEND_PAIRING_REQUEST,
    SM_BR_EDR_INITIATOR_W4_PAIRING_RESPONSE,
    SM_BR_EDR_RESPONDER_W4_PAIRING_REQUEST,
    SM_BR_EDR_RESPONDER_PAIRING_REQUEST_RECEIVED,
    SM_BR_EDR_DISTRIBUTE_KEYS,
    SM_BR_EDR_RECEIVE_KEYS,
    SM_BR_EDR_W2_CALCULATE_ILK_USING_H6,
    SM_BR_EDR_W2_CALCULATE_ILK_USING_H7,
    SM_BR_EDR_W4_CALCULATE_ILK,
    SM_BR_EDR_W2_CALCULATE_LE_LTK,
    SM_BR_EDR_W4_CALCULATE_LE_LTK,
} security_manager_state_t;

typedef enum {
    IRK_LOOKUP_IDLE,
    IRK_LOOKUP_W4_READY,
    IRK_LOOKUP_STARTED,
    IRK_LOOKUP_SUCCEEDED,
    IRK_LOOKUP_FAILED
} irk_lookup_state_t;

typedef uint8_t sm_pairing_packet_t[7];

// connection info available as long as connection exists

#if defined __cplusplus
}
#endif

#endif // HCI_H
