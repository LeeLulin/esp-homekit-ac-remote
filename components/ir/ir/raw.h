#pragma once

#include <ir/rx.h>

int ir_raw_send(int16_t *widths, uint16_t count);
ir_decoder_t *ir_raw_make_decoder();
