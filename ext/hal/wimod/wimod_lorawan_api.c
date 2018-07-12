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

#define MAKEWORD(lo,hi) ((lo)|((hi) << 8))
#define MAKELONG(lo,hi) ((lo)|((hi) << 16))

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

//------------------------------------------------------------------------------
//
//  JoinNetworkRequest
//
//  @brief: send join radio message
//
//------------------------------------------------------------------------------

int wimod_lorawan_join_network_request()
{
    // 1. init header
    tx_message.sap_id = LORAWAN_SAP_ID;
    tx_message.msg_id = LORAWAN_MSG_JOIN_NETWORK_REQ;
    tx_message.length = 0;

    // 2. send HCI message with payload
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
        case    LORAWAN_MSG_JOIN_NETWORK_RSP:
                wimod_lorawan_show_response("join network response", wimod_lorawan_status_strings, rx_msg->payload[0]);
                break;

        case    LORAWAN_MSG_SEND_UDATA_RSP:
                wimod_lorawan_show_response("send U-Data response", wimod_lorawan_status_strings, rx_msg->payload[0]);
                break;

        case    LORAWAN_MSG_SEND_CDATA_RSP:
                wimod_lorawan_show_response("send C-Data response", wimod_lorawan_status_strings, rx_msg->payload[0]);
                break;

        case    LORAWAN_MSG_JOIN_TRANSMIT_IND:
                wimod_lorawan_process_join_tx_indication(rx_msg);
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

        case    LORAWAN_MSG_RECV_NODATA_IND:
                printf("no data received indication\n\r");
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
    }
    else if (rx_msg->payload[0] == 1)
    {
        u32_t address = MAKELONG(MAKEWORD(rx_msg->payload[1],rx_msg->payload[2]),
                                  MAKEWORD(rx_msg->payload[3],rx_msg->payload[4]));

        printf("join network accept event - DeviceAddress:0x%08X, ChnIdx:%d, DR:%d, RSSI:%d, SNR:%d, RxSlot:%d\n\r", address,
               (int)rx_msg->payload[5], (int)rx_msg->payload[6], (int)rx_msg->payload[7],
               (int)rx_msg->payload[8], (int)rx_msg->payload[9]);
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
