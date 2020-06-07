#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <ir/tx.h>
#include <ir/rx.h>


typedef struct {
  ir_encoder_t encoder;

  size_t count;
  size_t pos;
  int16_t data[];
} ir_raw_encoder_t;


static int16_t ir_raw_get_next_pulse(ir_raw_encoder_t *encoder) {
    if (encoder->pos >= encoder->count)
        return 0;

    return encoder->data[encoder->pos++];
}


static void ir_raw_free(ir_raw_encoder_t *encoder) {
    if (encoder)
        free(encoder);
}


int ir_raw_send(int16_t *widths, uint16_t count) {
    ir_raw_encoder_t *encoder = malloc(sizeof(ir_raw_encoder_t) + sizeof(int16_t) * count);
    encoder->encoder.get_next_pulse = (ir_get_next_pulse_t)ir_raw_get_next_pulse;
    encoder->encoder.free = (ir_free_t)ir_raw_free;

    encoder->count = count;
    encoder->pos = 0;

    memcpy(encoder->data, widths, sizeof(int16_t) * count);

    int result = ir_tx_send((ir_encoder_t*) encoder);
    if (result) {
        ir_raw_free(encoder);
    }
    return result;
}


static int ir_raw_decode(ir_decoder_t *decoder, int16_t *pulses, uint16_t count,
                         void *decoded_data, uint16_t decoded_size)
{
    if (decoded_size < count * sizeof(int16_t))
        return -1;

    memcpy(decoded_data, pulses, count * sizeof(int16_t));

    return count;
}


ir_decoder_t *ir_raw_make_decoder() {
    ir_decoder_t *decoder = malloc(sizeof(ir_decoder_t));
    decoder->decode = (ir_decoder_decode_t) ir_raw_decode;
    decoder->free = (ir_decoder_free_t) free;
    return decoder;
}
