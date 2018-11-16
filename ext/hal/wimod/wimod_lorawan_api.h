
#ifndef WIMOD_LORAWAN_API_H
#define WIMOD_LORAWAN_API_H

#include <kernel.h>
#include <zephyr/types.h>

//------------------------------------------------------------------------------
//
//  Endpoint (SAP) Identifier
//
//------------------------------------------------------------------------------

#define DEVMGMT_SAP_ID                      0x01
#define LORAWAN_SAP_ID                      0x10

//------------------------------------------------------------------------------
//
//  Device Management SAP Message Identifier
//
//------------------------------------------------------------------------------

// Status Identifier
#define	DEVMGMT_STATUS_OK                       0x00
#define	DEVMGMT_STATUS_ERROR                    0x01
#define	DEVMGMT_STATUS_CMD_NOT_SUPPORTED        0x02
#define	DEVMGMT_STATUS_WRONG_PARAMETER          0x03
#define DEVMGMT_STATUS_WRONG_DEVICE_MODE        0x04

// Message Identifier
#define DEVMGMT_MSG_PING_REQ                    0x01
#define DEVMGMT_MSG_PING_RSP                    0x02

#define DEVMGMT_MSG_GET_DEVICE_INFO_REQ         0x03
#define DEVMGMT_MSG_GET_DEVICE_INFO_RSP         0x04

#define DEVMGMT_MSG_GET_FW_VERSION_REQ          0x05
#define DEVMGMT_MSG_GET_FW_VERSION_RSP          0x06

#define DEVMGMT_MSG_RESET_REQ  					0x07
#define DEVMGMT_MSG_RESET_RSP  					0x08

#define DEVMGMT_MSG_SET_OPMODE_REQ  			0x09
#define DEVMGMT_MSG_SET_OPMODE_RSP  			0x0A
#define DEVMGMT_MSG_GET_OPMODE_REQ  			0x0B
#define DEVMGMT_MSG_GET_OPMODE_RSP  			0x0C

#define DEVMGMT_MSG_SET_RTC_REQ  				0x0D
#define DEVMGMT_MSG_SET_RTC_RSP  				0x0E
#define DEVMGMT_MSG_GET_RTC_REQ  				0x0F
#define DEVMGMT_MSG_GET_RTC_RSP  				0x10

#define DEVMGMT_MSG_GET_DEVICE_STATUS_REQ  		0x17
#define DEVMGMT_MSG_GET_DEVICE_STATUS_RSP  		0x18

#define DEVMGMT_MSG_SET_RTC_ALARM_REQ  			0x31
#define DEVMGMT_MSG_SET_RTC_ALARM_RSP  			0x32
#define DEVMGMT_MSG_CLEAR_RTC_ALARM_REQ  		0x33
#define DEVMGMT_MSG_CLEAR_RTC_ALARM_RSP  		0x34
#define DEVMGMT_MSG_GET_RTC_ALARM_REQ  			0x35
#define DEVMGMT_MSG_GET_RTC_ALARM_RSP  			0x36
#define DEVMGMT_MSG_RTC_ALARM_IND  				0x38

//------------------------------------------------------------------------------
//
//  LoRaWAN SAP Message Identifier
//
//------------------------------------------------------------------------------

// Status Identifier
#define LORAWAN_STATUS_OK                       0x00
#define	LORAWAN_STATUS_ERROR                    0x01
#define	LORAWAN_STATUS_CMD_NOT_SUPPORTED        0x02
#define	LORAWAN_STATUS_WRONG_PARAMETER          0x03
#define LORAWAN_STATUS_WRONG_DEVICE_MODE        0x04
#define LORAWAN_STATUS_NOT_ACTIVATED            0x05
#define LORAWAN_STATUS_BUSY                     0x06
#define LORAWAN_STATUS_QUEUE_FULL               0x07
#define LORAWAN_STATUS_LENGTH_ERROR             0x08
#define LORAWAN_STATUS_NO_FACTORY_SETTINGS      0x09
#define LORAWAN_STATUS_CHANNEL_BLOCKED_BY_DC    0x0A
#define LORAWAN_STATUS_CHANNEL_NOT_AVAILABLE    0x0B

// Message Identifier
#define LORAWAN_MSG_ACTIVATE_DEVICE_REQ  		0x01
#define LORAWAN_MSG_ACTIVATE_DEVICE_RSP  		0x02

#define LORAWAN_MSG_SET_JOIN_PARAM_REQ  		0x05
#define LORAWAN_MSG_SET_JOIN_PARAM_RSP  		0x06

#define LORAWAN_MSG_JOIN_NETWORK_REQ            0x09
#define LORAWAN_MSG_JOIN_NETWORK_RSP            0x0A
#define LORAWAN_MSG_JOIN_NETWORK_TX_IND         0x0B
#define LORAWAN_MSG_JOIN_NETWORK_IND            0x0C

#define LORAWAN_MSG_SEND_UDATA_REQ              0x0D
#define LORAWAN_MSG_SEND_UDATA_RSP              0x0E
#define LORAWAN_MSG_SEND_UDATA_TX_IND           0x0F
#define LORAWAN_MSG_RECV_UDATA_IND              0x10

#define LORAWAN_MSG_SEND_CDATA_REQ              0x11
#define LORAWAN_MSG_SEND_CDATA_RSP              0x12
#define LORAWAN_MSG_SEND_CDATA_TX_IND           0x13
#define LORAWAN_MSG_RECV_CDATA_IND              0x14

#define LORAWAN_MSG_RECV_ACK_IND                0x15
#define LORAWAN_MSG_RECV_NO_DATA_IND            0x16

#define LORAWAN_MSG_SET_RSTACK_CONFIG_REQ  0x19
#define LORAWAN_MSG_SET_RSTACK_CONFIG_RSP  0x1A
#define LORAWAN_MSG_GET_RSTACK_CONFIG_REQ  0x1B
#define LORAWAN_MSG_GET_RSTACK_CONFIG_RSP  0x1C

#define LORAWAN_MSG_REACTIVATE_DEVICE_REQ  0x1D
#define LORAWAN_MSG_REACTIVATE_DEVICE_RSP  0x1E

#define LORAWAN_MSG_DEACTIVATE_DEVICE_REQ  0x21
#define LORAWAN_MSG_DEACTIVATE_DEVICE_RSP  0x22

#define LORAWAN_MSG_FACTORY_RESET_REQ  0x23
#define LORAWAN_MSG_FACTORY_RESET_RSP  0x24

#define LORAWAN_MSG_SET_DEVICE_EUI_REQ  0x25
#define LORAWAN_MSG_SET_DEVICE_EUI_RSP  0x26
#define LORAWAN_MSG_GET_DEVICE_EUI_REQ  0x27
#define LORAWAN_MSG_GET_DEVICE_EUI_RSP  0x28

#define LORAWAN_MSG_GET_NWK_STATUS_REQ  0x29
#define LORAWAN_MSG_GET_NWK_STATUS_RSP  0x2A

#define LORAWAN_MSG_SEND_MAC_CMD_REQ  0x2B
#define LORAWAN_MSG_SEND_MAC_CMD_RSP  0x2C
#define LORAWAN_MSG_RECV_MAC_CMD_IND  0x2D

#define LORAWAN_MSG_SET_CUSTOM_CFG_REQ  0x31
#define LORAWAN_MSG_SET_CUSTOM_CFG_RSP  0x32
#define LORAWAN_MSG_GET_CUSTOM_CFG_REQ  0x33
#define LORAWAN_MSG_GET_CUSTOM_CFG_RSP  0x34

#define LORAWAN_MSG_GET_SUPPORTED_BANDS_REQ  0x35
#define LORAWAN_MSG_GET_SUPPORTED_BANDS_RSP  0x36

#define LORAWAN_MSG_SET_LINKADRREQ_CONFIG_REQ  0x3B
#define LORAWAN_MSG_SET_LINKADRREQ_CONFIG_RSP  0x3C
#define LORAWAN_MSG_GET_LINKADRREQ_CONFIG_REQ  0x3D
#define LORAWAN_MSG_GET_LINKADRREQ_CONFIG_RSP  0x3E

#define LORAWAN_OPERATION_MODE_DEFAULT      0
#define LORAWAN_OPERATION_MODE_CUSTOMER     3



// helper struct for ID -> string conversion
typedef struct
{
    int         id;
    const char* string;
} id_string_t;

typedef void (*join_network_cb)();

//------------------------------------------------------------------------------
//
//  Function Prototypes
//
//------------------------------------------------------------------------------

// init
bool wimod_lorawan_init();

int wimod_lorawan_factory_reset();

// ping device
int wimod_lorawan_send_ping();

// get firmware Version
int wimod_lorawan_get_firmware_version();

int wimod_lorawan_set_op_mode();
int wimod_lorawan_get_op_mode();

int wimod_lorawan_get_device_eui();
int wimod_lorawan_set_join_param_request();

// join network
int wimod_lorawan_join_network_request(join_network_cb cb);

// send unconfirmed radio data
int wimod_lorawan_send_u_radio_data(u8_t port, u8_t* data, int length);

// send confirmed radio data
int wimod_lorawan_send_c_radio_data(u8_t port, u8_t* data, int length);

// receiver process
void wimod_lorawan_process();

int wimod_lorawan_get_nwk_status();

int wimod_lorawan_set_rstack_config();
int wimod_lorawan_get_rstack_config();
int wimod_lorawan_get_rtc();
int wimod_lorawan_set_rtc();
int wimod_lorawan_get_rtc_alarm();

#endif // WIMOD_LORAWAN_API_H

