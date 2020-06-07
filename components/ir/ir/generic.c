#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <ir/tx.h>
#include <ir/rx.h>
#include <ir/generic.h>
#include <ir/debug.h>


typedef enum {
    ir_state_idle,

    ir_state_header_mark,
    ir_state_header_space,

    ir_state_bit_mark,
    ir_state_bit_space,

    ir_state_footer_mark,
    ir_state_footer_space,
} ir_generic_fsm_state_t;


typedef struct {
    ir_encoder_t encoder;

    ir_generic_config_t *config;

    ir_generic_fsm_state_t fsm_state;

    size_t bit_count;
    size_t bits_left;

    size_t byte_pos;
    uint8_t bit_pos;

    uint8_t data[];
} ir_generic_encoder_t;


static int16_t ir_generic_get_next_pulse(ir_generic_encoder_t *encoder) {
    switch(encoder->fsm_state) {
        case ir_state_idle:
            return 0;

        case ir_state_header_mark:
            encoder->fsm_state = ir_state_header_space;
            return encoder->config->header_mark;

        case ir_state_header_space:
            encoder->fsm_state = ir_state_bit_mark;
            return encoder->config->header_space;

        case ir_state_bit_mark: {
            encoder->fsm_state = ir_state_bit_space;

            uint8_t bit = (encoder->data[encoder->byte_pos] >> encoder->bit_pos) & 0x1;

            return (bit ? encoder->config->bit1_mark : encoder->config->bit0_mark);
        }

        case ir_state_bit_space: {
            encoder->bits_left--;
            if (!encoder->bits_left) {
                if (encoder->config->footer_mark) {
                    encoder->fsm_state = ir_state_footer_mark;
                } else if (encoder->config->footer_space) {
                    encoder->fsm_state = ir_state_footer_space;
                } else {
                    encoder->fsm_state = ir_state_idle;
                }
            } else {
                encoder->fsm_state = ir_state_bit_mark;
            }

            uint8_t bit = (encoder->data[encoder->byte_pos] >> encoder->bit_pos) & 0x1;

            encoder->bit_pos++;
            if (encoder->bit_pos >= 8) {
                encoder->bit_pos = 0;
                encoder->byte_pos++;
            }

            return (bit ? encoder->config->bit1_space : encoder->config->bit0_space);
        }

        case ir_state_footer_mark:
            encoder->fsm_state = ir_state_footer_space;
            return encoder->config->footer_mark;

        case ir_state_footer_space:
            encoder->fsm_state = ir_state_idle;
            return encoder->config->footer_space;
    }

    return 0;
}


static void ir_generic_free(ir_generic_encoder_t *encoder) {
    if (encoder)
        free(encoder);
}


static ir_encoder_t *ir_generic_make_encoder(ir_generic_config_t *config, uint8_t *data, uint16_t data_size) {
    ir_generic_encoder_t *encoder =
        malloc(sizeof(ir_generic_encoder_t) + sizeof(uint8_t) * data_size);
    if (!encoder)
        return NULL;

    encoder->encoder.get_next_pulse = (ir_get_next_pulse_t)ir_generic_get_next_pulse;
    encoder->encoder.free = (ir_free_t)ir_generic_free;

    encoder->config = config;
    memcpy(encoder->data, data, sizeof(uint8_t) * data_size);

    encoder->bit_count = encoder->bits_left = data_size * 8;
    encoder->byte_pos = encoder->bit_pos = 0;

    if (config->header_mark) {
        encoder->fsm_state = ir_state_header_mark;
    } else if (config->header_space) {
        encoder->fsm_state = ir_state_header_space;
    } else {
        encoder->fsm_state = ir_state_bit_mark;
    }

    return (ir_encoder_t*)encoder;
}


int ir_generic_send(ir_generic_config_t *config, uint8_t *data, uint16_t data_size) {
    ir_encoder_t *encoder = ir_generic_make_encoder(config, data, data_size);
    if (!encoder)
        return -1;

    int result = ir_tx_send(encoder);
    if (result) {
        encoder->free(encoder);
    }
    return result;
}


typedef struct {
  ir_decoder_t decoder;

  ir_generic_config_t *config;
} ir_generic_decoder_t;


static int match(int16_t actual, int16_t expected, uint8_t tolerance) {
    if (actual < 0) {
        if (expected > 0) {
            return 0;
        }
        actual = -actual + 50;
        expected = -expected;
    } else {
        if (expected < 0) {
            return 0;
        }

        actual -= 50;
    }

    uint16_t delta = ((uint32_t)expected) * tolerance / 100;
    if ((actual < expected - delta) || (expected + delta < actual)) {
        return 0;
    }

    return 1;
}


static int ir_generic_decode(ir_generic_decoder_t *decoder,
                             int16_t *pulses, uint16_t count,
                             void *data, uint16_t data_size)
{
    if (!data_size) {
        ir_debug("generic: invalid buffer size\n");
        return -1;
    }

    ir_generic_config_t *c = decoder->config;

    if (!match(pulses[0], c->header_mark, c->tolerance) ||
           !match(pulses[1], c->header_space, c->tolerance))
    {
        ir_debug("generic: header does not match\n");
        return 0;
    }

    uint8_t *bits = data;
    uint8_t *bits_end = data + data_size;

    *bits = 0;

    uint8_t bit_count = 0;
    for (int i=2; i + 1 < count; i+=2, bit_count++) {
        if (bit_count >= 8) {
            bits++;
            *bits = 0;

            bit_count = 0;
        }

        if (match(pulses[i], c->bit1_mark, c->tolerance) &&
                match(pulses[i+1], c->bit1_space, c->tolerance)) {

            if (bits == bits_end) {
                ir_debug("generic: data overflow\n");
                return -1;
            }

            *bits |= 1 << bit_count;
        } else if (match(pulses[i], c->bit0_mark, c->tolerance) &&
                match(pulses[i+1], c->bit0_space, c->tolerance)) {

            if (bits == bits_end) {
                ir_debug("generic: data overflow\n");
                return -1;
            }

            // *bits |= 0 << bit_count;
        } else {
            ir_debug("generic: pulses at %d does not match: %d %d\n",
                     i, pulses[i], pulses[i+1]);
            return (bits - (uint8_t*)data + (bit_count ? 1 : 0));
        }
    }

    int decoded_size = bits - (uint8_t*)data + (bit_count ? 1 : 0);
    ir_debug("generic: decoded %d bytes\n", decoded_size);
    return decoded_size;
}


ir_decoder_t *ir_generic_make_decoder(ir_generic_config_t *config) {
    ir_generic_decoder_t *decoder = malloc(sizeof(ir_generic_decoder_t));
    if (!decoder)
        return NULL;

    decoder->decoder.decode = (ir_decoder_decode_t)ir_generic_decode;
    decoder->decoder.free = (ir_decoder_free_t) free;
    decoder->config = config;

    return (ir_decoder_t*) decoder;
}
