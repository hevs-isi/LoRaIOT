/** @file
 *  @brief LoRa HCI driver API.
 */

/*
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LORA_WIMOD_HCI_DRIVER_H
#define __LORA_WIMOD_HCI_DRIVER_H

#include <kernel.h>
#include <zephyr/types.h>

/**
 * @brief HCI drivers
 * @defgroup wimod_hci_driver HCI drivers
 * @ingroup lora
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#define WIMOD_HCI_MSG_HEADER_SIZE       2
#define WIMOD_HCI_MSG_PAYLOAD_SIZE      300
#define WIMOD_HCI_MSG_FCS_SIZE          2

#define LOBYTE(x)                       (x)
#define HIBYTE(x)                       ((u16_t)(x) >> 8)

typedef struct
{
    // Payload Length Information,
    // this field not transmitted over UART interface !!!
    u16_t  length;

    // Service Access Point Identifier
    u8_t   sap_id;

    // Message Identifier
    u8_t   msg_id;

    // Payload Field
    u8_t   payload[WIMOD_HCI_MSG_PAYLOAD_SIZE];

    // Frame Check Sequence Field
    u8_t   crc16[WIMOD_HCI_MSG_FCS_SIZE];

} wimod_hci_message_t;

// Message receiver callback
typedef wimod_hci_message_t* (*wimod_hci_cb_rx_message)(wimod_hci_message_t* rx_message);

// Init HCI Layer
bool wimod_hci_init(wimod_hci_cb_rx_message   cb_rx_message,
					wimod_hci_message_t*      rx_message);

// Send HCI Message
int wimod_hci_send_message(wimod_hci_message_t* tx_message);

// Receiver Process
void wimod_hci_process();

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __LORA_WIMOD_HCI_DRIVER_H */
