  
 /**
 *  HCI Command Builder and official commands
 *
 */

#ifndef HCI_EVENTS_H
#define HCI_EVENTS_H

#include "lib/bluetooth/bluetooth.h"

#if defined __cplusplus
extern "C" {
#endif

 /** 
  * compact HCI Event packet description
  * no subevent_code field -> subevnt_code == 0
  */
  typedef struct {
     uint8_t     event_code;
     uint8_t     subevent_code;
     const char *format;
 } hci_event_t;

 #define HCI_EVENT_MAX_LEN   255
 
 /**
  * construct HCI Event based on template
  *
  * Format:
  *   1,2,3,4: one to four byte value
  *   H: HCI connection handle
  *   B: Bluetooth Baseband Address (BD_ADDR)
  *   D: 8 byte data block
  *   P: 16 byte data block.
  *   Q: 32 byte data block, e.g. for X and Y coordinates of P-256 public key
  *   J: 1-byte length of following variable-length data blob 'V', length is included in packet
  *   K: 1-byte length of following variable-length data blob 'V', length is not included in packet
  *   V: variable-length data blob of len provided in 'J' field
  */
 uint16_t hci_event_create_from_template_and_arglist(uint8_t *hci_buffer, const hci_event_t *event
 , va_list argptr);
 
 
 uint16_t hci_event_create_from_template_and_arguments(uint8_t *hci_buffer, const hci_event_t *
 event, ...);
 
 const hci_event_t hci_event_hardware_error;
 const hci_event_t hci_event_command_complete;
 const hci_event_t hci_event_disconnection_complete;
 const hci_event_t hci_event_number_of_completed_packets_1;
 const hci_event_t hci_event_transport_packet_sent;
 
 /* LE Subevents */
 
 const hci_event_t hci_subevent_le_connection_complete;
 const hci_event_t hci_subevent_le_data_length_change;


 // EVENTS

// Events from host controller to host

/**
 * @brief Custom NOP Event - used for internal testing
 */
#define HCI_EVENT_NOP                                      0x00u

/**
 * @format 1
 * @param status
 */
#define HCI_EVENT_INQUIRY_COMPLETE                         0x01u

/** 
 * @format 1B11132
 * @param num_responses
 * @param bd_addr
 * @param page_scan_repetition_mode
 * @param reserved1
 * @param reserved2
 * @param class_of_device
 * @param clock_offset
 */
#define HCI_EVENT_INQUIRY_RESULT                           0x02u

/**
 * @format 12B11
 * @param status
 * @param connection_handle
 * @param bd_addr
 * @param link_type
 * @param encryption_enabled
 */
#define HCI_EVENT_CONNECTION_COMPLETE                      0x03u
/**
 * @format B31
 * @param bd_addr
 * @param class_of_device
 * @param link_type
 */
#define HCI_EVENT_CONNECTION_REQUEST                       0x04u
/**
 * @format 121
 * @param status
 * @param connection_handle
 * @param reason 
 */
#define HCI_EVENT_DISCONNECTION_COMPLETE                   0x05u
/**
 * @format 12
 * @param status
 * @param connection_handle
 */
#define HCI_EVENT_AUTHENTICATION_COMPLETE                 0x06u

// HCI_EVENT_AUTHENTICATION_COMPLETE_EVENT is deprecated, use HCI_EVENT_AUTHENTICATION_COMPLETE instead
#define HCI_EVENT_AUTHENTICATION_COMPLETE_EVENT HCI_EVENT_AUTHENTICATION_COMPLETE

/**
 * @format 1BN
 * @param status
 * @param bd_addr
 * @param remote_name
 */
#define HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE             0x07u

/**
 * @format 121
 * @param status
 * @param connection_handle
 * @param encryption_enabled 
 */
#define HCI_EVENT_ENCRYPTION_CHANGE                        0x08u

/**
 * @format 12
 * @param status
 * @param connection_handle
 */
#define HCI_EVENT_CHANGE_CONNECTION_LINK_KEY_COMPLETE      0x09u

/**
 * @format 121
 * @param status
 * @param connection_handle
 * @param key_flag 
 */
#define HCI_EVENT_MASTER_LINK_KEY_COMPLETE                 0x0Au

#define HCI_EVENT_READ_REMOTE_SUPPORTED_FEATURES_COMPLETE  0x0Bu

/**
 * @format 12122
 * @param status
 * @param connection_handle
 * @param version
 * @param manufacturer_name
 * @param subversion
 */
#define HCI_EVENT_READ_REMOTE_VERSION_INFORMATION_COMPLETE 0x0Cu

#define HCI_EVENT_QOS_SETUP_COMPLETE                       0x0Du

/**
 * @format 12R
 * @param num_hci_command_packets
 * @param command_opcode
 * @param return_parameters
 */
#define HCI_EVENT_COMMAND_COMPLETE                         0x0Eu
/**
 * @format 112
 * @param status
 * @param num_hci_command_packets
 * @param command_opcode
 */
#define HCI_EVENT_COMMAND_STATUS                           0x0Fu

/**
 * @format 1
 * @param hardware_code
 */
#define HCI_EVENT_HARDWARE_ERROR                           0x10u

/**
 * @format H
 * @param handle
 */
#define HCI_EVENT_FLUSH_OCCURRED                           0x11u

/**
 * @format 1B1
 * @param status
 * @param bd_addr
 * @param role
 */
#define HCI_EVENT_ROLE_CHANGE                              0x12u

// TODO: number_of_handles 1, connection_handle[H*i], hc_num_of_completed_packets[2*i]
#define HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS              0x13u

/**
 * @format 1H12
 * @param status
 * @param handle
 * @param mode
 * @param interval
 */
#define HCI_EVENT_MODE_CHANGE                              0x14u

// TODO: num_keys, bd_addr[B*i], link_key[16 octets * i]
#define HCI_EVENT_RETURN_LINK_KEYS                         0x15u

/**
 * @format B
 * @param bd_addr
 */
#define HCI_EVENT_PIN_CODE_REQUEST                         0x16u

/**
 * @format B
 * @param bd_addr
 */
#define HCI_EVENT_LINK_KEY_REQUEST                         0x17u

// TODO: bd_addr B, link_key 16octets, key_type 1
#define HCI_EVENT_LINK_KEY_NOTIFICATION                    0x18u

// event params contains HCI ccommand
#define HCI_EVENT_LOOPBACK_COMMAND                         0x19u

/**
 * @format 1
 * @param link_type
 */
#define HCI_EVENT_DATA_BUFFER_OVERFLOW                     0x1Au

/**
 * @format H1
 * @param handle
 * @param lmp_max_slots
 */
#define HCI_EVENT_MAX_SLOTS_CHANGED                        0x1Bu

/**
 * @format 1H2
 * @param status
 * @param handle
 * @param clock_offset
 */
#define HCI_EVENT_READ_CLOCK_OFFSET_COMPLETE               0x1Cu

/**
 * @format 1H2
 * @param status
 * @param handle
 * @param packet_types
 * @pnote packet_type is in plural to avoid clash with Java binding Packet.getPacketType()
 */
#define HCI_EVENT_CONNECTION_PACKET_TYPE_CHANGED           0x1Du

/**
 * @format H
 * @param handle
 */
#define HCI_EVENT_QOS_VIOLATION                            0x1Eu

// 0x1f not defined

/**
 * @format H1
 * @param handle
 * @param page_scan_repetition_mode
 */
#define HCI_EVENT_PAGE_SCAN_REPETITION_MODE_CHANGE         0x20u

/**
 * @format 1H1114444
 * @param status
 * @param handle
 * @param unused
 * @param flow_direction
 * @param service_type
 * @param token_rate
 * @param token_bucket_size
 * @param peak_bandwidth
 * @param access_latency
 *
 */
#define HCI_EVENT_FLOW_SPECIFICATION_COMPLETE              0x21u

/**
 * @format 1B11321
 * @param num_responses
 * @param bd_addr
 * @param page_scan_repetition_mode
 * @param reserved
 * @param class_of_device
 * @param clock_offset
 * @param rssi
 */
#define HCI_EVENT_INQUIRY_RESULT_WITH_RSSI                 0x22u

#define HCI_EVENT_READ_REMOTE_EXTENDED_FEATURES_COMPLETE   0x23u

// 0x24..0x2b not defined

/**
 * @format 1HB111221
 * @param status
 * @param handle
 * @param bd_addr
 * @param link_type
 * @param transmission_interval
 * @param retransmission_interval
 * @param rx_packet_length
 * @param tx_packet_length
 * @param air_mode
 */
#define HCI_EVENT_SYNCHRONOUS_CONNECTION_COMPLETE          0x2Cu

/**
 * @format 1H1122
 * @param status
 * @param handle
 * @param transmission_interval
 * @param retransmission_interval
 * @param rx_packet_length
 * @param tx_packet_length
 */
#define HCI_EVENT_SYNCHRONOUS_CONNECTION_CHANGED          0x2Du

/**
 * @format 1H2222
 * @param status
 * @param handle
 * @param max_tx_latency
 * @param max_rx_latency
 * @param min_remote_timeout
 * @param min_local_timeout
 */
#define HCI_EVENT_SNIFF_SUBRATING                         0x2Eu

// TODO: serialize extended_inquiry_response and provide parser

/**
 * @format 1B11321
 * @param num_responses
 * @param bd_addr
 * @param page_scan_repetition_mode
 * @param reserved
 * @param class_of_device
 * @param clock_offset
 * @param rssi
 */
#define HCI_EVENT_EXTENDED_INQUIRY_RESPONSE                0x2Fu

 /** 
  * @format 1H
  * @param status
  * @param handle
  */
#define HCI_EVENT_ENCRYPTION_KEY_REFRESH_COMPLETE          0x30u

 /**
  * @format B
  * @param bd_addr
  */
#define HCI_EVENT_IO_CAPABILITY_REQUEST                    0x31u

/**
 * @format B111
 * @param bd_addr
 * @param io_capability
 * @param oob_data_present
 * @param authentication_requirements
 */
#define HCI_EVENT_IO_CAPABILITY_RESPONSE                   0x32u

/**
 * @format B4
 * @param bd_addr
 * @param numeric_value
 */
#define HCI_EVENT_USER_CONFIRMATION_REQUEST                0x33u

/**
 * @format B
 * @param bd_addr
 */
#define HCI_EVENT_USER_PASSKEY_REQUEST                     0x34u

/**
 * @format B
 * @param bd_addr
 */
#define HCI_EVENT_REMOTE_OOB_DATA_REQUEST                  0x35u

/**
 * @format 1B
 * @param status
 * @param bd_addr
 */
#define HCI_EVENT_SIMPLE_PAIRING_COMPLETE                  0x36u

/**
 * @format H2
 * @param handle
 * @param link_supervision_timeout
 */
#define HCI_EVENT_LINK_SUPERVISION_TIMEOUT_CHANGED         0x38u

/**
 * @format H
 * @param handle
 */
#define HCI_EVENT_ENHANCED_FLUSH_COMPLETE                  0x39u

// 0x03a not defined

/**
 * @format B4
 * @param bd_addr
 * @param numeric_value
 */
#define HCI_EVENT_USER_PASSKEY_NOTIFICATION                0x3Bu

/**
 * @format B1
 * @param bd_addr
 * @param notification_type
 */
#define HCI_EVENT_KEYPRESS_NOTIFICATION                    0x3Cu

#define HCI_EVENT_REMOTE_HOST_SUPPORTED_FEATURES           0x3Du

#define HCI_EVENT_LE_META                                  0x3Eu

// 0x3f..0x47 not defined

#define HCI_EVENT_NUMBER_OF_COMPLETED_DATA_BLOCKS          0x48u

// 0x49..0x58 not defined

/**
 * @format 1211
 * @param status
 * @param connection_handle
 * @param encryption_enabled
 * @param encryption_key_size
 */
#define HCI_EVENT_ENCRYPTION_CHANGE_V2                     0x59u

// last used HCI_EVENT in 5.3 is 0x59u

#define HCI_EVENT_VENDOR_SPECIFIC                          0xFFu

/** 
 * @format 11H11B2221
 * @param subevent_code
 * @param status
 * @param connection_handle
 * @param role
 * @param peer_address_type
 * @param peer_address
 * @param conn_interval
 * @param conn_latency
 * @param supervision_timeout
 * @param master_clock_accuracy
 */
#define HCI_SUBEVENT_LE_CONNECTION_COMPLETE                0x01u

// array of advertisements, not handled by event accessor generator
#define HCI_SUBEVENT_LE_ADVERTISING_REPORT                 0x02u

/**
 * @format 11H222
 * @param subevent_code
 * @param status
 * @param connection_handle
 * @param conn_interval
 * @param conn_latency
 * @param supervision_timeout
 */
 #define HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE         0x03u

/**
 * @format 1HD
 * @param subevent_code
 * @param connection_handle
 * @param le_features
 */
#define HCI_SUBEVENT_LE_READ_REMOTE_FEATURES_COMPLETE 0x04u

/**
 * @format 1HD2
 * @param subevent_code
 * @param connection_handle
 * @param random_number
 * @param encryption_diversifier
 */
#define HCI_SUBEVENT_LE_LONG_TERM_KEY_REQUEST              0x05u

/**
 * @format 1H2222
 * @param subevent_code
 * @param connection_handle
 * @param interval_min
 * @param interval_max
 * @param latency
 * @param timeout
 */
#define HCI_SUBEVENT_LE_REMOTE_CONNECTION_PARAMETER_REQUEST 0x06u

/**
 * @format 1H2222
 * @param subevent_code
 * @param connection_handle
 * @param max_tx_octets
 * @param max_tx_time
 * @param max_rx_octets
 * @param max_rx_time
 */
#define HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE 0x07u

/**
 * @format 11QQ
 * @param subevent_code
 * @param status
 * @param dhkey_x x coordinate of P256 public key
 * @param dhkey_y y coordinate of P256 public key
 */
#define HCI_SUBEVENT_LE_READ_LOCAL_P256_PUBLIC_KEY_COMPLETE 0x08u

 /**
 * @format 11Q
 * @param subevent_code
 * @param status
 * @param dhkey Diffie-Hellman key
 */
#define HCI_SUBEVENT_LE_GENERATE_DHKEY_COMPLETE            0x09u

/**
 * @format 11H11BBB2221
 * @param subevent_code
 * @param status
 * @param connection_handle
 * @param role
 * @param peer_address_type
 * @param peer_addresss
 * @param local_resolvable_private_addres
 * @param peer_resolvable_private_addres
 * @param conn_interval
 * @param conn_latency
 * @param supervision_timeout
 * @param master_clock_accuracy
 */
#define HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE       0x0Au

// array of advertisements, not handled by event accessor generator
#define HCI_SUBEVENT_LE_DIRECT_ADVERTISING_REPORT          0x0Bu

/**
 * @format 11H1
 * @param subevent_code
 * @param status
 * @param connection_handle
 * @param tx_phy
 */
#define HCI_SUBEVENT_LE_PHY_UPDATE_COMPLETE                0x0Cu

// array of advertisements, not handled by event accessor generator
#define HCI_SUBEVENT_LE_EXTENDED_ADVERTISING_REPORT        0x0Du

/**
 * @format 11H11B121
 * @param subevent_code
 * @param status
 * @param sync_handle
 * @param advertising_sid
 * @param advertiser_address_type
 * @param advertiser_address
 * @param advertiser_phy
 * @param periodic_advertising_interval
 * @param advertiser_clock_accuracy
 */
#define HCI_SUBEVENT_LE_PERIODIC_ADVERTISING_SYNC_ESTABLISHMENT 0x0Eu

/**
 * @format 1H1111JV
 * @param subevent_code
 * @param sync_handle
 * @param tx_power
 * @param rssi
 * @param cte_type
 * @param data_status
 * @param data_length
 * @param data
*/
#define HCI_SUBEVENT_LE_PERIODIC_ADVERTISING_REPORT             0x0Fu

/**
 * @format 1H
 * @param subevent_code
 * @param sync_handle
 */
#define HCI_SUBEVENT_LE_PERIODIC_ADVERTISING_SYNC_LOST          0x10u

/**
 * @format 1
 * @param subevent_code
 */
#define HCI_SUBEVENT_LE_SCAN_TIMEOUT                            0x11u

/**
 * @format 111H1
 * @param subevent_code
 * @param status
 * @param advertising_handle
 * @param connection_handle
 * @param num_completed_exteneded_advertising_events
 */
#define HCI_SUBEVENT_LE_ADVERTISING_SET_TERMINATED              0x12u

/**
 * @format 111B
 * @param subevent_code
 * @param advertising_handle
 * @param scanner_address_type
 * @param scanner_address
 */
#define HCI_SUBEVENT_LE_SCAN_REQUEST_RECEIVED                   0x13u

/**
 * @format 1H1
 * @param subevent_code
 * @param connection_handle
 * @param channel_selection_algorithm
 */
#define HCI_SUBEVENT_LE_CHANNEL_SELECTION_ALGORITHM             0x14u

// array of advertisements, not handled by event accessor generator
#define HCI_SUBEVENT_LE_CONNECTIONLESS_IQ_REPORT                0x15u

// array of advertisements, not handled by event accessor generator
#define HCI_SUBEVENT_LE_CONNECTION_IQ_REPORT                    0x16u

/**
 * @format 11H
 * @param subevent_code
 * @param status
 * @param connection_handle
 */
#define HCI_SUBEVENT_LE_LE_CTE_REQUEST_FAILED                   0x17u

/**
 * @format 11H2H11B121
 * @param subevent_code
 * @param status
 * @param connection_handle
 * @param service_data
 * @param sync_handle
 * @param advertising_sid
 * @param advertiser_address_type
 * @param advertiser_address
 * @param advertiser_phy
 * @param periodic_advertising_interval
 * @param advertiser_clock_accuracy
 */
#define HCI_SUBEVENT_LE_PERIODIC_ADVERTISING_SYNC_TRANSFER_RECEIVED 0x18u

/**
 * @format 11H33331111111222
 * @param subevent_code
 * @param status
 * @param connection_handle
 * @param cig_sync_delay
 * @param cis_sync_delay
 * @param transport_latency_c_to_p
 * @param transport_latency_p_to_c
 * @param phy_c_to_p
 * @param phy_p_to_c
 * @param nse
 * @param bn_c_to_p
 * @param bn_p_to_c
 * @param ft_c_to_p
 * @param ft_p_to_c
 * @param max_pdu_c_to_p
 * @param max_pdu_p_to_c
 * @param iso_interval
 */
#define HCI_SUBEVENT_LE_CIS_ESTABLISHED                          0x19u

/**
 * @format 1HH11
 * @param subevent_code
 * @param acl_connection_handle
 * @param cis_connection_handle
 * @param cig_id
 * @param cis_id
 */
#define HCI_SUBEVENT_LE_CIS_REQUEST                            0x1au

// array of advertisements, not handled by event accessor generator
#define HCI_SUBEVENT_LE_CREATE_BIG_COMPLETE                     0x1Bu

/**
 * @format 111
 * @param subevent_code
 * @param big_handle
 * @param reason
 */
#define HCI_SUBEVENT_LE_TERMINATE_BIG_COMPLETE                   0x1Cu

// array of advertisements, not handled by event accessor generator
#define HCI_SUBEVENT_LE_BIG_SYNC_ESTABLISHED                     0x1Du

/**
 * @format 111
 * @param subevent_code
 * @param big_handle
 * @param reason
 */
#define HCI_SUBEVENT_LE_BIG_SYNC_LOST                            0x1Eu

/**
 * @format 11H1
 * @param subevent_code
 * @param status
 * @param connection_handle
 * @param peer_clock_accuracy
 */
#define HCI_SUBEVENT_LE_REQUEST_PEER_SCA_COMPLETE                0x1Fu

/**
 * @format 11H11111
 * @param subevent_code
 * @param status
 * @param connection_handle
 * @param reason
 * @param phy
 * @param tx_power_level
 * @param tx_power_level_flag
 * @param delta
 */
#define HCI_SUBEVENT_LE_TRANSMIT_POWER_REPORTING                 0x21u

/**
 * @format 1H112111232111
 * @param subevent_code
 * @param sync_handle
 * @param num_bis
 * @param nse
 * @param iso_interval
 * @param bn
 * @param pto
 * @param irc
 * @param max_pdu
 * @param sdu_interval
 * @param max_sdu
 * @param phy
 * @param framing
 * @param encryption
 */
#define HCI_SUBEVENT_LE_BIGINFO_ADVERTISING_REPORT                0x22u

/**
 * @format 11H2222
 * @param subevent_code
 * @param status
 * @param connection_handle
 * @param subrate_factor
 * @param peripheral_latency
 * @param continuation_number
 * @param supervision_timeout
 */
#define HCI_SUBEVENT_LE_SUBRATE_CHANGE                            0x23u

/**
 * @format 1
 * @param state
 */
#define BTSTACK_EVENT_STATE                                0x60u

/**
 * @format 1
 * @param number_connections
 */
#define BTSTACK_EVENT_NR_CONNECTIONS_CHANGED               0x61u

/**
 * @format 
 */
#define BTSTACK_EVENT_POWERON_FAILED                       0x62u

/**
 * @format 1
 * @param discoverable
 */
#define BTSTACK_EVENT_DISCOVERABLE_ENABLED                 0x66u

// Daemon Events

/**
 * @format 112
 * @param major
 * @param minor
 @ @param revision
 */
#define DAEMON_EVENT_VERSION                               0x63u

// data: system bluetooth on/off (bool)
/**
 * @format 1
 * param system_bluetooth_enabled
 */
#define DAEMON_EVENT_SYSTEM_BLUETOOTH_ENABLED              0x64u

// data: event (8), len(8), status (8) == 0, address (48), name (1984 bits = 248 bytes)

/* 
 * @format 1BT
 * @param status == 0 to match read_remote_name_request
 * @param address
 * @param name
 */
#define DAEMON_EVENT_REMOTE_NAME_CACHED                    0x65u

// internal - data: event(8)
#define DAEMON_EVENT_CONNECTION_OPENED                     0x67u

// internal - data: event(8)
#define DAEMON_EVENT_CONNECTION_CLOSED                     0x68u

// data: event(8), len(8), local_cid(16), credits(8)
#define DAEMON_EVENT_L2CAP_CREDITS                         0x74u

/**
 * @format 12
 * @param status
 * @param psm
 */
#define DAEMON_EVENT_L2CAP_SERVICE_REGISTERED              0x75u

/**
 * @format 11
 * @param status
 * @param channel_id
 */
#define DAEMON_EVENT_RFCOMM_SERVICE_REGISTERED             0x85u

/**
 * @format 11
 * @param status
 * @param server_channel_id
 */
#define DAEMON_EVENT_RFCOMM_PERSISTENT_CHANNEL             0x86u

/**
  * @format 14
  * @param status
  * @param service_record_handle
  */
#define DAEMON_EVENT_SDP_SERVICE_REGISTERED                0x90u



// additional HCI events

/**
 * @brief Indicates HCI transport enters/exits Sleep mode
 * @format 1
 * @param active
 */
#define HCI_EVENT_TRANSPORT_SLEEP_MODE                     0x69u

/**
 * @brief Transport USB Bluetooth Controller info
 * @format 22JV
 * @param vendor_id
 * @param product_id
 * @param path_len
 * @param path
 */
#define HCI_EVENT_TRANSPORT_USB_INFO                       0x6Au

/**
 * @brief Transport ready 
 */
#define HCI_EVENT_TRANSPORT_READY                          0x6Du

/**
 * @brief Outgoing packet 
 */
#define HCI_EVENT_TRANSPORT_PACKET_SENT                    0x6Eu

/**
 * @format B
 * @param handle
 */
#define HCI_EVENT_SCO_CAN_SEND_NOW                         0x6Fu


// L2CAP EVENTS
    
/**
 * @format 1BH222222111
 * @param status
 * @param address
 * @param handle
 * @param psm
 * @param local_cid
 * @param remote_cid
 * @param local_mtu
 * @param remote_mtu
 * @param flush_timeout
 * @param incoming
 * @param mode
 * @param fcs
 */
#define L2CAP_EVENT_CHANNEL_OPENED                         0x70u

/*
 * @format 2
 * @param local_cid
 */
#define L2CAP_EVENT_CHANNEL_CLOSED                         0x71u

/**
 * @format BH222
 * @param address
 * @param handle
 * @param psm
 * @param local_cid
 * @param remote_cid
 */
#define L2CAP_EVENT_INCOMING_CONNECTION                    0x72u

// ??
// data: event(8), len(8), handle(16)
#define L2CAP_EVENT_TIMEOUT_CHECK                          0x73u

/**
 * @format H2222
 * @param handle
 * @param interval_min
 * @param interval_max
 * @param latencey
 * @param timeout_multiplier
 */
#define L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_REQUEST    0x76u

// data: event(8), len(8), handle(16), result (16) (0 == ok, 1 == fail)
 /** 
  * @format H2
  * @param handle
  * @param result
  */
#define L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE   0x77u

 /*
 * @format H22
 * @param con_handle
 * @param extended_feature_mask
 * @param fixed_channels_supported
 */
#define L2CAP_EVENT_INFORMATION_RESPONSE                   0x78u

/**
 * @format 2
 * @param local_cid
 */
#define L2CAP_EVENT_CAN_SEND_NOW                           0x79u

/*
 * @format 2
 * @param local_cid
 */
#define L2CAP_EVENT_PACKET_SENT                            0x7au

/*
 * @format 2
 * @param local_cid
 */
#define L2CAP_EVENT_ERTM_BUFFER_RELEASED                   0x7bu

// L2CAP Channel in LE Credit-based Flow-Control Mode (CBM)

/**
 * @format 1BH2222
 * @param address_type
 * @param address
 * @param handle
 * @param psm
 * @param local_cid
 * @param remote_cid
 * @param remote_mtu
 */
#define L2CAP_EVENT_CBM_INCOMING_CONNECTION                 0x7cu

/**
 * @format 11BH122222
 * @param status
 * @param address_type
 * @param address
 * @param handle
 * @param incoming
 * @param psm
 * @param local_cid
 * @param remote_cid
 * @param local_mtu
 * @param remote_mtu
 */
#define L2CAP_EVENT_CBM_CHANNEL_OPENED                      0x7du

/*
 * @format
 */
#define L2CAP_EVENT_TRIGGER_RUN                             0x7eu

/**
 * @format 1BH212
 * @param address_type
 * @param address
 * @param handle
 * @param psm
 * @param num_channels
 * @param local_cid first new cid
 */
#define L2CAP_EVENT_ECBM_INCOMING_CONNECTION               0x7fu

/**
 * @format 11BH122222
 * @param status
 * @param address_type
 * @param address
 * @param handle
 * @param incoming
 * @param psm
 * @param local_cid
 * @param remote_cid
 * @param local_mtu
 * @param remote_mtu
 */
#define L2CAP_EVENT_ECBM_CHANNEL_OPENED              0x8au

/*
 * @format 222
 * @param remote_cid
 * @param mtu
 * @param mps
 */
#define L2CAP_EVENT_ECBM_RECONFIGURED                0x8bu

/*
 * @format 22
 * @param local_cid
 * @param reconfigure_result
 */
#define L2CAP_EVENT_ECBM_RECONFIGURATION_COMPLETE    0x8cu


// RFCOMM EVENTS

/**
 * @format 1B21221
 * @param status
 * @param bd_addr
 * @param con_handle
 * @param server_channel
 * @param rfcomm_cid
 * @param max_frame_size
 * @param incoming
 */
#define RFCOMM_EVENT_CHANNEL_OPENED                        0x80u

/**
 * @format 2
 * @param rfcomm_cid
 */
#define RFCOMM_EVENT_CHANNEL_CLOSED                        0x81u

/**
 * @format B12H
 * @param bd_addr
 * @param server_channel
 * @param rfcomm_cid
 * @param con_handle

 */
#define RFCOMM_EVENT_INCOMING_CONNECTION                   0x82u

/**
 * @format 21
 * @param rfcomm_cid
 * @param line_status
 */
#define RFCOMM_EVENT_REMOTE_LINE_STATUS                    0x83u
        
/**
 * @format 21
 * @param rfcomm_cid
 * @param modem_status
 */
#define RFCOMM_EVENT_REMOTE_MODEM_STATUS                   0x87u

/**
 * note: port configuration not parsed by stack, getters provided by rfcomm.h
 * param rfcomm_cid
 * param remote - 0 for local port, 1 for remote port
 * param baud_rate
 * param data_bits
 * param stop_bits
 * param parity
 * param flow_control
 * param xon
 * param xoff
 */
#define RFCOMM_EVENT_PORT_CONFIGURATION                    0x88u

/**
 * @format 2
 * @param rfcomm_cid
 */
#define RFCOMM_EVENT_CAN_SEND_NOW                          0x89u


/**
 * @format 1
 * @param status
 */
#define SDP_EVENT_QUERY_COMPLETE                                 0x91u 

/**
 * @format 1T
 * @param rfcomm_channel
 * @param name
 */
#define SDP_EVENT_QUERY_RFCOMM_SERVICE                           0x92u

/**
 * @format 22221
 * @param record_id
 * @param attribute_id
 * @param attribute_length
 * @param data_offset
 * @param data
 */
#define SDP_EVENT_QUERY_ATTRIBUTE_BYTE                           0x93u

/**
 * @format 22LV
 * @param record_id
 * @param attribute_id
 * @param attribute_length
 * @param attribute_value
 */
#define SDP_EVENT_QUERY_ATTRIBUTE_VALUE                          0x94u

/**
 * @format 224
 * @param total_count
 * @param record_index
 * @param record_handle
 * @note Not provided by daemon, only used for internal testing
 */
#define SDP_EVENT_QUERY_SERVICE_RECORD_HANDLE                    0x95u

/**
 * @format H1
 * @param handle
 * @param att_status  see ATT errors in bluetooth.h  
 */
#define GATT_EVENT_QUERY_COMPLETE                                0xA0u

/**
 * @format HX
 * @param handle
 * @param service
 */
#define GATT_EVENT_SERVICE_QUERY_RESULT                          0xA1u

/**
 * @format HY
 * @param handle
 * @param characteristic
 */
#define GATT_EVENT_CHARACTERISTIC_QUERY_RESULT                   0xA2u

/**
 * @format H2X
 * @param handle
 * @param include_handle
 * @param service
 */
#define GATT_EVENT_INCLUDED_SERVICE_QUERY_RESULT                 0xA3u

/**
 * @format HZ
 * @param handle
 * @param characteristic_descriptor
 */
#define GATT_EVENT_ALL_CHARACTERISTIC_DESCRIPTORS_QUERY_RESULT   0xA4u

/**
 * @format H2LV
 * @param handle
 * @param value_handle
 * @param value_length
 * @param value
 */
#define GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT             0xA5u

/**
 * @format H22LV
 * @param handle
 * @param value_handle
 * @param value_offset
 * @param value_length
 * @param value
 */
#define GATT_EVENT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT        0xA6u

/**
 * @format H2LV
 * @param handle
 * @param value_handle
 * @param value_length
 * @param value
 */
#define GATT_EVENT_NOTIFICATION                                  0xA7u

/**
 * @format H2LV
 * @param handle
 * @param value_handle
 * @param value_length
 * @param value
 */
#define GATT_EVENT_INDICATION                                    0xA8u

/**
 * @format H2LV
 * @param handle
 * @param descriptor_handle
 * @param descriptor_length
 * @param descriptor
 */
#define GATT_EVENT_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT        0xA9u

/**
 * @format H2LV
 * @param handle
 * @param descriptor_offset
 * @param descriptor_length
 * @param descriptor
 */
#define GATT_EVENT_LONG_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT   0xAAu

/** 
 * @format H2
 * @param handle
 * @param MTU
 */    
#define GATT_EVENT_MTU                                           0xABu

/**
 * @format H
 * @param handle
 */
#define GATT_EVENT_CAN_WRITE_WITHOUT_RESPONSE                    0xACu


/** 
 * @format 1BH
 * @param address_type
 * @param address
 * @param handle
 */    
#define ATT_EVENT_CONNECTED                                      0xB3u

/** 
 * @format H
 * @param handle
 */    
#define ATT_EVENT_DISCONNECTED                                   0xB4u

/** 
 * @format H2
 * @param handle
 * @param MTU
 */    
#define ATT_EVENT_MTU_EXCHANGE_COMPLETE                          0xB5u

 /**
  * @format 1H2
  * @param status
  * @param conn_handle
  * @param attribute_handle
  */
#define ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE               0xB6u

/**
 * @format
 */
#define ATT_EVENT_CAN_SEND_NOW                                   0xB7u

// TODO: daemon only event

/**
 * @format 12
 * @param status
 * @param service_uuid
 */
 #define BNEP_EVENT_SERVICE_REGISTERED                           0xC0u

/**
 * @format 12222BH
 * @param status
 * @param bnep_cid
 * @param source_uuid
 * @param destination_uuid
 * @param mtu
 * @param remote_address
 * @param con_handle
 */
 #define BNEP_EVENT_CHANNEL_OPENED                               0xC1u

/**
 * @format 222B
 * @param bnep_cid
 * @param source_uuid
 * @param destination_uuid
 * @param remote_address
 */
 #define BNEP_EVENT_CHANNEL_CLOSED                               0xC2u

/**
 * @format 222B1
 * @param bnep_cid
 * @param source_uuid
 * @param destination_uuid
 * @param remote_address
 * @param channel_state
 */
#define BNEP_EVENT_CHANNEL_TIMEOUT                               0xC3u    
    
/**
 * @format 222B
 * @param bnep_cid
 * @param source_uuid
 * @param destination_uuid
 * @param remote_address
 */
 #define BNEP_EVENT_CAN_SEND_NOW                                 0xC4u

 /**
  * @format H1B1
  * @param handle
  * @param addr_type
  * @param address
  * @param secure_connection - set to 1 if LE Secure Connection pairing will be used
  */
#define SM_EVENT_JUST_WORKS_REQUEST                              0xC8u

 /**
  * @format H1B14
  * @param handle
  * @param addr_type
  * @param address
  * @param secure_connection - set to 1 if LE Secure Connection pairing will be used
  * @param passkey
  */
#define SM_EVENT_PASSKEY_DISPLAY_NUMBER                          0xC9u

 /**
  * @format H1B
  * @param handle
  * @param addr_type
  * @param address
  */
#define SM_EVENT_PASSKEY_DISPLAY_CANCEL                          0xCAu

 /**
  * @format H1B1
  * @param handle
  * @param addr_type
  * @param address
  * @param secure_connection - set to 1 if LE Secure Connection pairing will be used
  */
#define SM_EVENT_PASSKEY_INPUT_NUMBER                            0xCBu

 /**
  * @format H1B14
  * @param handle
  * @param addr_type
  * @param address
  * @param secure_connection - set to 1 if LE Secure Connection pairing will be used
  * @param passkey
  */
#define SM_EVENT_NUMERIC_COMPARISON_REQUEST                      0xCCu

 /**
  * @format H1B
  * @param handle
  * @param addr_type
  * @param address
  */
#define SM_EVENT_IDENTITY_RESOLVING_STARTED                      0xCDu

 /**
  * @format H1B
  * @param handle
  * @param addr_type
  * @param address
  */
#define SM_EVENT_IDENTITY_RESOLVING_FAILED                       0xCEu

 /**
  * @brief Identify resolving succeeded
  *
  * @format H1B1B2
  * @param handle
  * @param addr_type
  * @param address
  * @param identity_addr_type
  * @param identity_address
  * @param index
  *
  */
#define SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED                    0xCFu

 /**
  * @format H1B
  * @param handle
  * @param addr_type
  * @param address
  */
#define SM_EVENT_AUTHORIZATION_REQUEST                           0xD0u

 /**
  * @format H1B1
  * @param handle
  * @param addr_type
  * @param address
  * @param authorization_result
  */
#define SM_EVENT_AUTHORIZATION_RESULT                            0xD1u


 /**
  * @format H1
  * @param handle
  * @param action see SM_KEYPRESS_*
  */
#define SM_EVENT_KEYPRESS_NOTIFICATION                           0xD2u

 /**
  * @brief Emitted during pairing to inform app about address used as identity
  *
  * @format H1B1B2
  * @param handle
  * @param addr_type
  * @param address
  * @param identity_addr_type
  * @param identity_address
  * @param index
  */
#define SM_EVENT_IDENTITY_CREATED                                0xD3u

/**
 * @brief Emitted to inform app that pairing has started.
 * @format H1B
 * @param handle
 * @param addr_type
 * @param address
 */
#define SM_EVENT_PAIRING_STARTED                                 0xD4u

/**
  * @brief Emitted to inform app that pairing is complete. Possible status values:
  *        ERROR_CODE_SUCCESS                            -> pairing success
  *        ERROR_CODE_CONNECTION_TIMEOUT                 -> timeout
  *        ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION  -> disconnect
  *        ERROR_CODE_AUTHENTICATION_FAILURE             -> SM protocol error, see reason field with SM_REASON_* from bluetooth.h
  *
  * @format H1B11
  * @param handle
  * @param addr_type
  * @param address
  * @param status
  * @param reason if status == ERROR_CODE_AUTHENTICATION_FAILURE
  */
#define SM_EVENT_PAIRING_COMPLETE                                0xD5u


/**
 * @brief Proactive Authentication for bonded devices started.
 * @format H1B
 * @param handle
 * @param addr_type
 * @param address
 */
#define SM_EVENT_REENCRYPTION_STARTED                            0xD6u

/**
 * @brief Proactive Authentication for bonded devices complete. Possible status values:
 *         ERROR_CODE_SUCCESS                           -> connection secure
 *         ERROR_CODE_CONNECTION_TIMEOUT                -> timeout
 *         ERROR_CODE_PIN_OR_KEY_MISSING                -> remote did not provide (as Peripheral) or use LTK (as Central)
 * @format H1B1
 * @param handle
 * @param addr_type
 * @param address
 * @param status
 */
#define SM_EVENT_REENCRYPTION_COMPLETE                           0xD7u

// GAP

/**
 * @format H1
 * @param handle
 * @param security_level
 */
#define GAP_EVENT_SECURITY_LEVEL                                 0xD8u

/**
 * @format 1B
 * @param status
 * @param address
 */
#define GAP_EVENT_DEDICATED_BONDING_COMPLETED                    0xD9u

/**
 * @format 11B1JV
 * @param advertising_event_type
 * @param address_type
 * @param address
 * @param rssi
 * @param data_length
 * @param data
 */
#define GAP_EVENT_ADVERTISING_REPORT                             0xDAu

/**
 * @format 21B1111121BJV
 * @param advertising_event_type
 * @param address_type
 * @param address
 * @param primary_phy
 * @param secondary_phy
 * @param advertising_sid
 * @param tx_power
 * @param rssi
 * @param periodic_advertising_interval
 * @param direct_address_type
 * @param direct_address
 * @param data_length
 * @param data
 */
#define GAP_EVENT_EXTENDED_ADVERTISING_REPORT                    0xDBu

 /**
 * @format B13211122221JV
 * @param bd_addr
 * @param page_scan_repetition_mode
 * @param class_of_device
 * @param clock_offset
 * @param rssi_available
 * @param rssi
 * @param device_id_available
 * @param device_id_vendor_id_source
 * @param device_id_vendor_id
 * @param device_id_product_id
 * @param device_id_version
 * @param name_available
 * @param name_len
 * @param name
 */
#define GAP_EVENT_INQUIRY_RESULT                                 0xDCu

/**
 * @format 1
 * @param status
 */
#define GAP_EVENT_INQUIRY_COMPLETE                               0xDDu

/**
 * @format H1
 * @param con_handle
 * @param rssi (signed integer -127..127)
 * @note Classic: rssi is in dB relative to Golden Receive Power Range
 * @note LE: rssi is absolute dBm
 */
#define GAP_EVENT_RSSI_MEASUREMENT                               0xDEu

/**
 * @format 1KKKK
 * @param oob_data_present 0 = none, 1 = p_192, 2 = p_256, 3 = both
 * @param c_192 Simple Pairing Hash C derived from P-192 public key
 * @param r_192 Simple Pairing Randomizer derived from P-192 public key
 * @param c_256 Simple Pairing Hash C derived from P-256 public key
 * @param r_256 Simple Pairing Randomizer derived from P-256 public key
 */
#define GAP_EVENT_LOCAL_OOB_DATA                                 0xDFu


/**
 * @format HB11
 * @param con_handle
 * @param bd_addr
 * @param ssp
 * @param initiator
 */
#define GAP_EVENT_PAIRING_STARTED                                0xE0u

/**
 * @format HB1
 * @param con_handle
 * @param bd_addr
 * @param status
 */
#define GAP_EVENT_PAIRING_COMPLETE                               0xE1u

// Meta Events, see below for sub events
#define HCI_EVENT_META_GAP                                       0xE7u
#define HCI_EVENT_HSP_META                                       0xE8u
#define HCI_EVENT_HFP_META                                       0xE9u
#define HCI_EVENT_ANCS_META                                      0xEAu
#define HCI_EVENT_AVDTP_META                                     0xEBu
#define HCI_EVENT_AVRCP_META                                     0xECu
#define HCI_EVENT_GOEP_META                                      0xEDu
#define HCI_EVENT_PBAP_META                                      0xEEu
#define HCI_EVENT_HID_META                                       0xEFu
#define HCI_EVENT_A2DP_META                                      0xF0u
#define HCI_EVENT_HIDS_META                                      0xF1u
#define HCI_EVENT_GATTSERVICE_META                               0xF2u
#define HCI_EVENT_BIP_META                                       0xF3u
#define HCI_EVENT_MAP_META                                       0xF4u
#define HCI_EVENT_MESH_META                                      0xF5u

#if defined __cplusplus
}
#endif

#endif // HCI_CMDS_H
