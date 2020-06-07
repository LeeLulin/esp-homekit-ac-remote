#pragma once

#ifdef IR_DEBUG
#include <stdio.h>
#define ir_debug(message, ...) printf("IR: " message, ## __VA_ARGS__)
#define ir_debug1(message, ...) printf(message, ## __VA_ARGS__)
#else
#define ir_debug(message, ...)
#define ir_debug1(message, ...)
#endif
