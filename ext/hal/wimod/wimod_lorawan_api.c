//------------------------------------------------------------------------------
//
//	File:		wimod_lorawan_API.cpp
//
//	Abstract:	API Layer of LoRaWAN Host Controller Interface
//
//	Version:	0.1
//
//	Date:		18.05.2016
//
//	Disclaimer:	This example code is provided by IMST GmbH on an "AS IS" basis
//				without any warranties.
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Include Files
//
//------------------------------------------------------------------------------

#include "wimod_lorawan_api.h"
#include "wimod_hci_driver.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAKEWORD(lo,hi) ((lo)|((hi) << 8))
#define MAKELONG(lo,hi) ((lo)|((hi) << 16))

const char appEui[] = "70B3D57ED0010F8B";
const char appKey[] = "7F8BC5F8A59B05A2B1BDE8B84E6D62A4";

//------------------------------------------------------------------------------
//
//  Forward Declarations
//
//------------------------------------------------------------------------------

// HCI Message Receiver callback
static wimod_hci_message_t*
wimod_lorawan_process_rx_msg(wimod_hci_message_t* rx_msg);

static void
wimod_lorawan_process_devmgmt_message(wimod_hci_message_t* rx_msg);

static void
wimod_lorawan_devmgmt_firmware_version_rsp(wimod_hci_message_t* rx_msg);

static void
wimod_lorawan_get_opmode_rsp(wimod_hci_message_t* rx_msg);

static void
wimod_lorawan_get_rtc_rsp(wimod_hci_message_t* rx_msg);

static void
wimod_lorawan_get_rtc_alarm_rsp(wimod_hci_message_t* rx_msg);

static void
wimod_lorawan_process_lorawan_message(wimod_hci_message_t* rx_msg);

static void
wimod_lorawan_process_join_tx_indication(wimod_hci_message_t* rx_msg);

static void
wimod_lorawan_process_join_network_indication(wimod_hci_message_t* rx_msg);

static void
wimod_lorawan_process_u_data_rx_indication(wimod_hci_message_t* rx_msg);

static void
wimod_lorawan_process_c_data_rx_indication(wimod_hci_message_t* rx_msg);

static void
wimod_lorawan_show_response(const char* string, const id_string_t* status_table, u8_t status_id);

//------------------------------------------------------------------------------
//
//  Section RAM
//
//------------------------------------------------------------------------------

// reserve one Tx-Message
wimod_hci_message_t tx_message;

// reserve one Rx-Message
wimod_hci_message_t rx_message;

// network join callback function
static join_network_cb join_callback;

//------------------------------------------------------------------------------
//
//  Section Const
//
//------------------------------------------------------------------------------

// Status Codes for DeviceMgmt Response Messages
static const id_string_t wimod_device_mgmt_status_strings[] =
{
    { DEVMGMT_STATUS_OK,                   "ok" },
    { DEVMGMT_STATUS_ERROR,                "error" } ,
    { DEVMGMT_STATUS_CMD_NOT_SUPPORTED,    "command not supported" },
    { DEVMGMT_STATUS_WRONG_PARAMETER,      "wrong parameter" },
    { DEVMGMT_STATUS_WRONG_DEVICE_MODE,    "wrong device mode" },

    // end of table
    { 0, 0 }
};

// Status Codes for LoRaWAN Response Messages
static const id_string_t wimod_lorawan_status_strings[] =
{
    { LORAWAN_STATUS_OK,                    "ok" },
    { LORAWAN_STATUS_ERROR,                 "error" } ,
    { LORAWAN_STATUS_CMD_NOT_SUPPORTED,     "command not supported" },
    { LORAWAN_STATUS_WRONG_PARAMETER,       "wrong parameter" },
    { LORAWAN_STATUS_WRONG_DEVICE_MODE,     "wrong device mode" },
    { LORAWAN_STATUS_NOT_ACTIVATED,         "device not activated" },
    { LORAWAN_STATUS_BUSY,                  "device busy - command rejected" },
    { LORAWAN_STATUS_QUEUE_FULL,            "message queue full - command rejected" },
    { LORAWAN_STATUS_LENGTH_ERROR,          "HCI message length error" },
    { LORAWAN_STATUS_NO_FACTORY_SETTINGS,   "no factory settings available" },
    { LORAWAN_STATUS_CHANNEL_BLOCKED_BY_DC, "error: channel blocked due to duty cycle, please try later again" },
    { LORAWAN_STATUS_CHANNEL_NOT_AVAILABLE, "error: channel not available" },

    // end of table
    { 0, 0 }
};
//------------------------------------------------------------------------------
//
//  Section Code
//
//------------------------------------------------------------------------------


static void memset64( void * dest, u64_t value, uintptr_t size )
{
  uintptr_t i;
  for( i = 0; i < (size & (~7)); i+=8 )
  {
    memcpy( ((char*)dest) + i, &value, 8 );
  }
  for( ; i < size; i++ )
  {
    ((char*)dest)[i] = ((char*)&value)[i&7];
  }
}

//------------------------------------------------------------------------------
//
//  Init
//
//  @brief: init complete interface
//
//------------------------------------------------------------------------------

bool wimod_lorawan_init()
{
    // init HCI layer
    return wimod_hci_init(wimod_lorawan_process_rx_msg, &rx_message);
}


int wimod_lorawan_factory_reset()
{
    // 1. init header
    tx_message.sap_id = LORAWAN_SAP_ID;
    tx_message.msg_id = LORAWAN_MSG_FACTORY_RESET_REQ;
    tx_message.length = 0;

    // 2. send HCI message without payload
    return wimod_hci_send_message(&tx_message);
}

//------------------------------------------------------------------------------
//
//  Ping
//
//  @brief: ping device
//
//------------------------------------------------------------------------------

int wimod_lorawan_send_ping()
{
    // 1. init header
    tx_message.sap_id = DEVMGMT_SAP_ID;
    tx_message.msg_id = DEVMGMT_MSG_PING_REQ;
    tx_message.length = 0;

    // 2. send HCI message without payload
    return wimod_hci_send_message(&tx_message);
}

//------------------------------------------------------------------------------
//
//  GetFirmwareVersion
//
//  @brief: get firmware version
//
//------------------------------------------------------------------------------

int wimod_lorawan_get_firmware_version()
{
    // 1. init header
    tx_message.sap_id = DEVMGMT_SAP_ID;
    tx_message.msg_id = DEVMGMT_MSG_GET_FW_VERSION_REQ;
    tx_message.length = 0;

    // 2. send HCI message without payload
    return wimod_hci_send_message(&tx_message);
}

int wimod_lorawan_set_op_mode()
{
    // 1. init header
    tx_message.sap_id = DEVMGMT_SAP_ID;
    tx_message.msg_id = DEVMGMT_MSG_SET_OPMODE_REQ ;
    tx_message.length = 1;
    tx_message.payload[0] = LORAWAN_OPERATION_MODE_CUSTOMER;

    // 2. send HCI message without payload
    return wimod_hci_send_message(&tx_message);
}

int wimod_lorawan_get_op_mode()
{
    // 1. init header
    tx_message.sap_id = DEVMGMT_SAP_ID;
    tx_message.msg_id = DEVMGMT_MSG_GET_OPMODE_REQ  ;
    tx_message.length = 0;

    // 2. send HCI message without payload
    return wimod_hci_send_message(&tx_message);
}

int wimod_lorawan_get_rtc()
{
    // 1. init header
    tx_message.sap_id = DEVMGMT_SAP_ID;
    tx_message.msg_id = DEVMGMT_MSG_GET_RTC_REQ;
    tx_message.length = 0;

    // 2. send HCI message without payload
    return wimod_hci_send_message(&tx_message);
}

int wimod_lorawan_set_rtc()
{
    // 1. init header
    tx_message.sap_id = DEVMGMT_SAP_ID;
    tx_message.msg_id = DEVMGMT_MSG_SET_RTC_REQ;
    tx_message.length = 4;

    tx_message.payload[0] = 0;
    tx_message.payload[1] = 0;
    tx_message.payload[2] = 0;
    tx_message.payload[3] = 0;

    // 2. send HCI message without payload
    return wimod_hci_send_message(&tx_message);
}

int wimod_lorawan_get_rtc_alarm()
{
    // 1. init header
    tx_message.sap_id = DEVMGMT_SAP_ID;
    tx_message.msg_id = DEVMGMT_MSG_GET_RTC_ALARM_REQ;
    tx_message.length = 0;

    // 2. send HCI message without payload
    return wimod_hci_send_message(&tx_message);
}

int wimod_lorawan_get_device_eui()
{
    // 1. init header
    tx_message.sap_id = LORAWAN_SAP_ID;
    tx_message.msg_id = LORAWAN_MSG_GET_DEVICE_EUI_REQ;
    tx_message.length = 0;

    // 2. send HCI message without payload
    return wimod_hci_send_message(&tx_message);
}


int wimod_lorawan_set_join_param_request()
{
	const char *pos;
	size_t i;
	char buf[5];

    // 1. init header
    tx_message.sap_id = LORAWAN_SAP_ID;
    tx_message.msg_id = LORAWAN_MSG_SET_JOIN_PARAM_REQ;
    tx_message.length = 24;

    pos = appEui;
    for (i = 0; i < 8; i++) {
    	sprintf(buf, "0x%c%c", pos[0+2*i], pos[1+2*i]);
    	tx_message.payload[i] = strtol(buf, NULL, 0);
    }

    pos = appKey;
	for (i = 0; i < 16; i++) {
		sprintf(buf, "0x%c%c", pos[0+2*i], pos[1+2*i]);
		tx_message.payload[8+i] = strtol(buf, NULL, 0);
	}

/*
    memset64(tx_message.payload, 0x8B0F01D07ED5B370, 8);
	memset64(&tx_message.payload[8], 0xA2059BA5F8C58B7F, 8);
	memset64(&tx_message.payload[16], 0xA4626D4EB8E8BDB1, 8);
*/

    // 2. send HCI message with payload
    return wimod_hci_send_message(&tx_message);
}

//------------------------------------------------------------------------------
//
//  JoinNetworkRequest
//
//  @brief: send join radio message
//
//------------------------------------------------------------------------------

int wimod_lorawan_join_network_request(join_network_cb cb)
{
    // 1. init header
    tx_message.sap_id = LORAWAN_SAP_ID;
    tx_message.msg_id = LORAWAN_MSG_JOIN_NETWORK_REQ;
    tx_message.length = 0;

    join_callback = cb;

    // 2. send HCI message with payload
    return wimod_hci_send_message(&tx_message);
}

int wimod_lorawan_get_nwk_status()
{
    // 1. init header
    tx_message.sap_id = LORAWAN_SAP_ID;
    tx_message.msg_id = LORAWAN_MSG_GET_NWK_STATUS_REQ;
    tx_message.length = 0;

    // 2. send HCI message without payload
    return wimod_hci_send_message(&tx_message);
}

int wimod_lorawan_get_rstack_config()
{
    // 1. init header
    tx_message.sap_id = LORAWAN_SAP_ID;
    tx_message.msg_id = LORAWAN_MSG_GET_RSTACK_CONFIG_REQ;
    tx_message.length = 0;

    // 2. send HCI message without payload
    return wimod_hci_send_message(&tx_message);
}

int wimod_lorawan_set_rstack_config()
{
    // 1. init header
    tx_message.sap_id = LORAWAN_SAP_ID;
    tx_message.msg_id = LORAWAN_MSG_SET_RSTACK_CONFIG_REQ;
    tx_message.length = 6;

    tx_message.payload[0] = 0;
    tx_message.payload[1] = 16;
    tx_message.payload[2] = 0x01;
    tx_message.payload[3] = 0x01;
    tx_message.payload[4] = 7;
    tx_message.payload[5] = 1;
    tx_message.payload[6] = 15;

    // 2. send HCI message without payload
    return wimod_hci_send_message(&tx_message);
}

//------------------------------------------------------------------------------
//
//  SendURadioData
//
//  @brief: send unconfirmed radio message
//
//------------------------------------------------------------------------------

int wimod_lorawan_send_u_radio_data(u8_t port, u8_t* src_data, int src_length)
{
    // 1. check length
    if (src_length > (WIMOD_HCI_MSG_PAYLOAD_SIZE - 1))
    {
        // error
        return -1;
    }

    // 2. init header
    tx_message.sap_id = LORAWAN_SAP_ID;
    tx_message.msg_id = LORAWAN_MSG_SEND_UDATA_REQ;
    tx_message.length = 1 + src_length;

    // 3.  init payload
    // 3.1 init port
    tx_message.payload[0] = port;

    // 3.2 init radio message payload
    memcpy(&tx_message.payload[1], src_data, src_length);

    // 4. send HCI message with payload
    return wimod_hci_send_message(&tx_message);
}

//------------------------------------------------------------------------------
//
//  SendCRadioData
//
//  @brief: send confirmed radio message
//
//------------------------------------------------------------------------------

int wimod_lorawan_send_c_radio_data(u8_t  port, u8_t* src_data, int src_length)
{
    // 1. check length
    if (src_length > (WIMOD_HCI_MSG_PAYLOAD_SIZE - 1))
    {
        // error
        return -1;
    }

    // 2. init header
    tx_message.sap_id = LORAWAN_SAP_ID;
    tx_message.msg_id = LORAWAN_MSG_SEND_CDATA_REQ;
    tx_message.length = 1 + src_length;

    // 3.  init payload
    // 3.1 init port
    tx_message.payload[0] = port;

    // 3.2 init radio message payload
    memcpy(&tx_message.payload[1], src_data, src_length);

    // 4. send HCI message with payload
    return wimod_hci_send_message(&tx_message);
}

//------------------------------------------------------------------------------
//
//  Process
//
//  @brief: handle receiver process
//
//------------------------------------------------------------------------------

void wimod_lorawan_process()
{
    // call HCI process
	wimod_hci_process();
}

//------------------------------------------------------------------------------
//
//  Process
//
//  @brief: handle receiver process
//
//------------------------------------------------------------------------------

static wimod_hci_message_t* wimod_lorawan_process_rx_msg(wimod_hci_message_t* rx_msg)
{
    switch(rx_msg->sap_id)
    {
        case DEVMGMT_SAP_ID:
            wimod_lorawan_process_devmgmt_message(rx_msg);
            break;


        case LORAWAN_SAP_ID:
            wimod_lorawan_process_lorawan_message(rx_msg);
            break;
    }
    return &rx_message;
}

//------------------------------------------------------------------------------
//
//  Process_DevMgmt_Message
//
//  @brief: handle received Device Management SAP messages
//
//------------------------------------------------------------------------------

static void wimod_lorawan_process_devmgmt_message(wimod_hci_message_t* rx_msg)
{
    switch(rx_msg->msg_id)
    {
        case    DEVMGMT_MSG_PING_RSP:
                wimod_lorawan_show_response("ping response", wimod_device_mgmt_status_strings, rx_msg->payload[0]);
                break;

        case    DEVMGMT_MSG_GET_FW_VERSION_RSP:
                wimod_lorawan_devmgmt_firmware_version_rsp(rx_msg);
                break;

        case    DEVMGMT_MSG_SET_OPMODE_RSP:
                wimod_lorawan_show_response("set opmode response", wimod_device_mgmt_status_strings, rx_msg->payload[0]);
                break;

        case    DEVMGMT_MSG_GET_OPMODE_RSP:
                wimod_lorawan_get_opmode_rsp(rx_msg);
                break;

        case    DEVMGMT_MSG_GET_RTC_RSP:
				wimod_lorawan_get_rtc_rsp(rx_msg);
				break;

        case    DEVMGMT_MSG_SET_RTC_RSP:
        		wimod_lorawan_show_response("set rtc response", wimod_device_mgmt_status_strings, rx_msg->payload[0]);
				break;

        case    DEVMGMT_MSG_GET_RTC_ALARM_RSP:
				wimod_lorawan_get_rtc_alarm_rsp(rx_msg);
				break;

        default:
                printf("unhandled DeviceMgmt message received - msg_id : 0x%02X\n\r", (u8_t)rx_msg->msg_id);
                break;
    }
}

//------------------------------------------------------------------------------
//
//  wimod_lorawan_DevMgmt_FirmwareVersion_Rsp
//
//  @brief: show firmware version
//
//------------------------------------------------------------------------------

static void wimod_lorawan_devmgmt_firmware_version_rsp(wimod_hci_message_t* rx_msg)
{
    char help[80];

    wimod_lorawan_show_response("firmware version response", wimod_device_mgmt_status_strings, rx_msg->payload[0]);

    if (rx_msg->payload[0] == DEVMGMT_STATUS_OK)
    {
        printf("version: V%d.%d\n\r", (int)rx_msg->payload[2], (int)rx_msg->payload[1]);
        printf("build-count: %d\n\r", (int)MAKEWORD(rx_msg->payload[3], rx_msg->payload[4]));

        memcpy(help, &rx_msg->payload[5], 10);
        help[10] = 0;
        printf("build-date: %s\n\r", help);

        // more information attached ?
        if (rx_msg->length > 15)
        {
            // add string termination
            rx_msg->payload[rx_msg->length] = 0;
            printf("firmware-content: %s\n\r", &rx_msg->payload[15]);
        }
    }
}

static void wimod_lorawan_get_opmode_rsp(wimod_hci_message_t* rx_msg)
{
    wimod_lorawan_show_response("get opmode response", wimod_device_mgmt_status_strings, rx_msg->payload[0]);
    printf("Operation mode: %d\n\r", rx_msg->payload[1]);
}

static void wimod_lorawan_get_rtc_rsp(wimod_hci_message_t* rx_msg)
{
    u32_t rtc_value;

    wimod_lorawan_show_response("get rtc response", wimod_device_mgmt_status_strings, rx_msg->payload[0]);

    if (rx_msg->payload[0] == DEVMGMT_STATUS_OK)
    {
    	rtc_value = MAKELONG(MAKEWORD(rx_msg->payload[4], rx_msg->payload[3]),
    				MAKEWORD(rx_msg->payload[2], rx_msg->payload[1]));

    	printf("RTC: %d\n\r", rtc_value);
    }
}

static void wimod_lorawan_get_rtc_alarm_rsp(wimod_hci_message_t* rx_msg)
{
    wimod_lorawan_show_response("get rtc alarm response", wimod_device_mgmt_status_strings, rx_msg->payload[0]);

    if (rx_msg->payload[0] == DEVMGMT_STATUS_OK)
    {
    	printf("Alarm Status: %d\n\r", rx_msg->payload[1]);
    	printf("Options: %d\n\r", rx_msg->payload[2]);
    }
}

static void wimod_lorawan_device_eui_rsp(wimod_hci_message_t* rx_msg)
{
	u32_t eui_lsb;
	u32_t eui_msb;
	wimod_lorawan_show_response("device EUI response",
			wimod_device_mgmt_status_strings, rx_msg->payload[0]);

	eui_lsb = MAKELONG(MAKEWORD(rx_msg->payload[8], rx_msg->payload[7]),
			MAKEWORD(rx_msg->payload[6], rx_msg->payload[5]));
	eui_msb = MAKELONG(MAKEWORD(rx_msg->payload[4], rx_msg->payload[3]),
			MAKEWORD(rx_msg->payload[2], rx_msg->payload[1]));

	printf("64-Bit Device EUI: 0x%x%x\n", eui_msb, eui_lsb);
}

static void wimod_lorawan_process_nwk_status_rsp(wimod_hci_message_t* rx_msg)
{
	u32_t device_address;

	wimod_lorawan_show_response("network status response",
			wimod_device_mgmt_status_strings, rx_msg->payload[0]);

	printf("Network Status: 0x%02X\n", rx_msg->payload[1]);

	if(rx_msg->length > 2){

		device_address = MAKELONG(MAKEWORD(rx_msg->payload[5], rx_msg->payload[4]),
					MAKEWORD(rx_msg->payload[3], rx_msg->payload[2]));

		printf("Device Address: %d\n", device_address);
		printf("Data Rate Index: %d\n", rx_msg->payload[6]);
		printf("Power Level: %d\n", rx_msg->payload[7]);
		printf("Max Payload Size: %d\n", rx_msg->payload[8]);
	}
}

static void wimod_lorawan_process_rstack_config_rsp(wimod_hci_message_t* rx_msg)
{
	wimod_lorawan_show_response("radio stack config response",
			wimod_device_mgmt_status_strings, rx_msg->payload[0]);

	printf("Default Data Rate Index: %d\n", rx_msg->payload[1]);
	printf("Default TX Power Level: %d\n", rx_msg->payload[2]);
	printf("Options: 0x%02X\n", rx_msg->payload[3]);
	printf("\tAdaptive Data Rate: %s\n", (rx_msg->payload[3] & 0x01) ? "enabled" : "disabled");
	printf("\tDuty Cycle Control: %s\n", (rx_msg->payload[3] & 0x02) ? "enabled" : "disabled");
	printf("\tDevice Class: %s\n", (rx_msg->payload[3] & 0x04) ? "C" : "A");
	printf("\tRF packet output format: %s\n", (rx_msg->payload[3] & 0x40) ? "extended" : "standard");
	printf("\tRx MAC Command Forwarding: %s\n", (rx_msg->payload[3] & 0x80) ? "enabled" : "disabled");
	printf("Power Saving Mode: %s\n", rx_msg->payload[4] ? "automatic" : "off");
	printf("Number of Retransmissions: %d\n", rx_msg->payload[5]);
	printf("Band Index: %d\n", rx_msg->payload[6]);
	// not available in 1.11 specs
	printf("Header MAC Cmd Capacity: %d\n", rx_msg->payload[7] & 0xFF);

}

static void wimod_lorawan_process_send_data_rsp(wimod_hci_message_t* rx_msg)
{
    u32_t time_remaining_ms;

    wimod_lorawan_show_response("send C-Data response",
            wimod_device_mgmt_status_strings, rx_msg->payload[0]);

    if(rx_msg->length > 1){

        time_remaining_ms = MAKELONG(MAKEWORD(rx_msg->payload[1], rx_msg->payload[2]),
                    MAKEWORD(rx_msg->payload[3], rx_msg->payload[4]));

        printf("Channel blocked. Time remaining (ms): %d\n", time_remaining_ms);
    }
}

//------------------------------------------------------------------------------
//
//  Process_LoRaWAN_Message
//
//  @brief: handle received LoRaWAN SAP messages
//
//------------------------------------------------------------------------------

static void wimod_lorawan_process_lorawan_message(wimod_hci_message_t* rx_msg)
{
    switch(rx_msg->msg_id)
    {
    	case    LORAWAN_MSG_GET_DEVICE_EUI_RSP:
				wimod_lorawan_device_eui_rsp(rx_msg);
				break;

    	case    LORAWAN_MSG_JOIN_NETWORK_RSP:
				wimod_lorawan_show_response("join network response", wimod_lorawan_status_strings, rx_msg->payload[0]);
				break;

    	case    LORAWAN_MSG_FACTORY_RESET_RSP:
				wimod_lorawan_show_response("factory reset response", wimod_lorawan_status_strings, rx_msg->payload[0]);
				break;

        case    LORAWAN_MSG_SET_JOIN_PARAM_RSP:
                wimod_lorawan_show_response("set join param response", wimod_lorawan_status_strings, rx_msg->payload[0]);
                break;

        case    LORAWAN_MSG_SET_RSTACK_CONFIG_RSP:
				wimod_lorawan_show_response("set rstack response", wimod_lorawan_status_strings, rx_msg->payload[0]);
				break;

        case    LORAWAN_MSG_SEND_UDATA_RSP:
                wimod_lorawan_show_response("send U-Data response", wimod_lorawan_status_strings, rx_msg->payload[0]);
                break;

        case    LORAWAN_MSG_SEND_CDATA_RSP:
                wimod_lorawan_process_send_data_rsp(rx_msg);
                //wimod_lorawan_show_response("send C-Data response", wimod_lorawan_status_strings, rx_msg->payload[0]);
                break;

        case    LORAWAN_MSG_GET_NWK_STATUS_RSP:
				wimod_lorawan_process_nwk_status_rsp(rx_msg);
				break;

        case    LORAWAN_MSG_GET_RSTACK_CONFIG_RSP:
        		wimod_lorawan_process_rstack_config_rsp(rx_msg);
				break;

        case    LORAWAN_MSG_JOIN_NETWORK_TX_IND:
                wimod_lorawan_process_join_tx_indication(rx_msg);
                break;

        case    LORAWAN_MSG_SEND_UDATA_TX_IND:
        		wimod_lorawan_show_response("send U-Data TX indication", wimod_lorawan_status_strings, rx_msg->payload[0]);
				break;

        case    LORAWAN_MSG_SEND_CDATA_TX_IND:
        		wimod_lorawan_show_response("send C-Data TX indication", wimod_lorawan_status_strings, rx_msg->payload[0]);
				break;

        case    LORAWAN_MSG_JOIN_NETWORK_IND:
                wimod_lorawan_process_join_network_indication(rx_msg);
                break;

        case    LORAWAN_MSG_RECV_UDATA_IND:
                wimod_lorawan_process_u_data_rx_indication(rx_msg);
                break;

        case    LORAWAN_MSG_RECV_CDATA_IND:
                wimod_lorawan_process_c_data_rx_indication(rx_msg);
                break;

        case    LORAWAN_MSG_RECV_NO_DATA_IND:
                printf("no data received indication\n\r");
                if(rx_msg->payload[0] == 1){
                	printf("Error Code: 0x%02X\n\r", rx_msg->payload[1]);
                }
                break;

        default:
                printf("unhandled LoRaWAN SAP message received - msg_id : 0x%02X\n\r", (u8_t)rx_msg->msg_id);
                break;
    }
}

//------------------------------------------------------------------------------
//
//  wimod_lorawan_process_JoinTxIndication
//
//  @brief: show join transmit indicaton
//
//------------------------------------------------------------------------------

static void wimod_lorawan_process_join_tx_indication(wimod_hci_message_t* rx_msg)
{
    if (rx_msg->payload[0] == 0)
    {
        printf("join tx event - Status : ok\n\r");
    }
    // channel info attached ?
    else if(rx_msg->payload[0] == 1)
    {
        printf("join tx event:%d, ChnIdx:%d, DR:%d - Status : ok\n\r", (int)rx_msg->payload[3], (int)rx_msg->payload[1], (int)rx_msg->payload[2]);
    }
    else
    {
        printf("join tx event - Status : error\n\r");
    }
}

//------------------------------------------------------------------------------
//
//  wimod_lorawan_process_JoinNetworkIndication
//
//  @brief: show join network indicaton
//
//------------------------------------------------------------------------------

void wimod_lorawan_process_join_network_indication(wimod_hci_message_t* rx_msg)
{
    if (rx_msg->payload[0] == 0)
    {
        u32_t address = MAKELONG(MAKEWORD(rx_msg->payload[1],rx_msg->payload[2]),
                                  MAKEWORD(rx_msg->payload[3],rx_msg->payload[4]));

        printf("join network accept event - DeviceAddress:0x%08X\n\r", address);

        if(join_callback){
        	(*join_callback)();
        }
    }
    else if (rx_msg->payload[0] == 1)
    {
        u32_t address = MAKELONG(MAKEWORD(rx_msg->payload[1],rx_msg->payload[2]),
                                  MAKEWORD(rx_msg->payload[3],rx_msg->payload[4]));

        printf("join network accept event - DeviceAddress:0x%08X, ChnIdx:%d, DR:%d, RSSI:%d, SNR:%d, RxSlot:%d\n\r", address,
               (int)rx_msg->payload[5], (int)rx_msg->payload[6], (int)rx_msg->payload[7],
               (int)rx_msg->payload[8], (int)rx_msg->payload[9]);

        if(join_callback){
        	(*join_callback)();
        }
    }
    else
    {
        printf("join network timeout event\n\r");
    }
}

//------------------------------------------------------------------------------
//
//  wimod_lorawan_process_U_DataRxIndication
//
//  @brief: show received U-Data
//
//------------------------------------------------------------------------------

void wimod_lorawan_process_u_data_rx_indication(wimod_hci_message_t* rx_msg)
{
    int payload_size = rx_msg->length - 1;

    // rx channel info attached ?
    if (rx_msg->payload[0] & 0x01)
        payload_size -= 5;

    if (payload_size >= 1)
    {
        printf("U-Data rx event: port:0x%02X\n\rapp-payload:", rx_msg->payload[1]);
        for(int i = 1; i < payload_size; i++)
            printf("%02X ", rx_msg->payload[1+i]);
        printf("\n\r");
    }

    if (rx_msg->payload[0] & 0x02)
        printf("ack for uplink packet:yes\n\r");
    else
        printf("ack for uplink packet:no\n\r");

    if (rx_msg->payload[0] & 0x04)
        printf("frame pending:yes\n\r");
    else
        printf("frame pending:no\n\r");

    // rx channel info attached ?
    if (rx_msg->payload[0] & 0x01)
    {
        u8_t* rxInfo = &rx_msg->payload[1 + payload_size];
        printf("ChnIdx:%d, DR:%d, RSSI:%d, SNR:%d, RxSlot:%d\n\r",
              (int)rxInfo[0], (int)rxInfo[1], (int)rxInfo[2],
              (int)rxInfo[3], (int)rxInfo[4]);
    }
}

//------------------------------------------------------------------------------
//
//  wimod_lorawan_process_C_DataRxIndication
//
//  @brief: show received C-Data
//
//------------------------------------------------------------------------------

void wimod_lorawan_process_c_data_rx_indication(wimod_hci_message_t* rx_msg)
{
    int payload_size = rx_msg->length - 1;

    // rx channel info attached ?
    if (rx_msg->payload[0] & 0x01)
        payload_size -= 5;

    if (payload_size >= 1)
    {
        printf("C-Data rx event: port:0x%02X\n\rapp-payload:", rx_msg->payload[1]);
        for(int i = 1; i < payload_size;)
            printf("%02X ", rx_msg->payload[1+i]);
        printf("\n\r");
    }

    if (rx_msg->payload[0] & 0x02)
        printf("ack for uplink packet:yes\n\r");
    else
        printf("ack for uplink packet:no\n\r");

    if (rx_msg->payload[0] & 0x04)
        printf("frame pending:yes\n\r");
    else
        printf("frame pending:no\n\r");

    // rx channel info attached ?
    if (rx_msg->payload[0] & 0x01)
    {
        u8_t* rxInfo = &rx_msg->payload[1 + payload_size];
        printf("ChnIdx:%d, DR:%d, RSSI:%d, SNR:%d, RxSlot:%d\n\r",
              (int)rxInfo[0], (int)rxInfo[1], (int)rxInfo[2],
              (int)rxInfo[3], (int)rxInfo[4]);
    }
}

//------------------------------------------------------------------------------
//
//  wimod_lorawan_show_response
//
//  @brief: show response status as human readable string
//
//------------------------------------------------------------------------------

static void wimod_lorawan_show_response(const char* str, const id_string_t* status_table, u8_t status_id)
{
    while(status_table->string)
    {
        if (status_table->id == status_id)
        {
            printf(str);
            printf(" - Status(0x%02X) : ", status_id);
            printf(status_table->string);
            printf("\n\r");
            return;
        }

        status_table++;
    }
}
//------------------------------------------------------------------------------
// end of file
//------------------------------------------------------------------------------
