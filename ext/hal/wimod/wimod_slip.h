
#ifndef __LORA_WIMOD_SLIP_H
#define __LORA_WIMOD_SLIP_H

#include <zephyr/types.h>

// SLIP Protocol Characters
#define SLIP_END					0xC0
#define	SLIP_ESC					0xDB
#define	SLIP_ESC_END				0xDC
#define	SLIP_ESC_ESC				0xDD

//------------------------------------------------------------------------------
//
//  Function Prototypes
//
//------------------------------------------------------------------------------

// SLIP message receiver callback
typedef u8_t* (*slip_cb_rx_message_t)(u8_t* message, int length);

// Init SLIP layer
void slip_init(slip_cb_rx_message_t cb_rx_message);

// Init first receiver buffer
bool slip_set_rx_buffer(u8_t* rx_buffer, int rx_buf_size);

// Encode outgoing Data
int slip_encode_data(u8_t* dst_buffer, int tx_buf_size, u8_t* src_data, int src_length);

// Decode incoming Data
void slip_decode_data(u8_t* src_data, int src_length);

#endif /* __LORA_WIMOD_SLIP_H */
