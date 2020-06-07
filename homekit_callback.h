#pragma once

#include <homekit/homekit.h>

void ac_identify(homekit_value_t _value);
homekit_value_t ac_active_get();
void ac_active_set(homekit_value_t value);
homekit_value_t ac_target_temperature_get();
void ac_target_temperature_set(homekit_value_t value);
homekit_value_t ac_swing_get();
void ac_swing_set(homekit_value_t value);
homekit_value_t ac_speed_get();
void ac_speed_set(homekit_value_t value);

homekit_value_t fan_active_get();
void fan_active_set(homekit_value_t value);
homekit_value_t fan_speed_get();
void fan_speed_set(homekit_value_t value);

void temperature_sensor_task(void *_args);