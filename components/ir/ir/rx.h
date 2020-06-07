#pragma once

#include <stdint.h>

typedef struct ir_decoder ir_decoder_t;

typedef int(*ir_decoder_decode_t)(ir_decoder_t *decoder, int16_t *pulses, uint16_t pulse_count,
                                  void *decoded_data, uint16_t decoded_size);

typedef void(*ir_decoder_free_t)(ir_decoder_t *decoder);

struct ir_decoder {
    ir_decoder_decode_t decode;
    ir_decoder_free_t free;
};


void ir_rx_init(uint8_t gpio, uint16_t buffer_size);

// Some sensors have lag and thus marks tend to be slightly longer
// and spaces - slightly shorter. Excess amount (in microseconds) is subtracted from
// mark pulse lengths and added to space lengths.
void ir_rx_set_excess(int16_t excess);

int ir_recv(ir_decoder_t *decoder, uint32_t timeout, void *receive_buffer, uint16_t receive_buffer_size);
