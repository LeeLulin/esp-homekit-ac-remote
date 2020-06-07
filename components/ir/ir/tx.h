#pragma once

#include <stdint.h>


typedef struct ir_encoder ir_encoder_t;

typedef int16_t(*ir_get_next_pulse_t)(ir_encoder_t *);
typedef void(*ir_free_t)(ir_encoder_t *);

struct ir_encoder {
    ir_get_next_pulse_t get_next_pulse;
    ir_free_t free;
};


void ir_tx_init();
int ir_tx_send(ir_encoder_t *encoder);
