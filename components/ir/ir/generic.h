#pragma once

#include <stdint.h>
#include <ir/rx.h>


typedef struct {
    int16_t header_mark;
    int16_t header_space;

    int16_t bit1_mark;
    int16_t bit1_space;

    int16_t bit0_mark;
    int16_t bit0_space;

    int16_t footer_mark;
    int16_t footer_space;

    uint8_t tolerance;
} ir_generic_config_t;


int ir_generic_send(ir_generic_config_t *config, uint8_t *data, uint16_t data_size);
ir_decoder_t *ir_generic_make_decoder(ir_generic_config_t *config);
