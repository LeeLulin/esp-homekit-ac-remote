#pragma once

#include <ir/ir.h>
#include <ir/raw.h>
#include <ir/rx.h>
#include <ir/tx.h>

#define IR_SEND(command)                                                       \
  ir_raw_send(command, sizeof(command) / sizeof(command[0]));

void ir_init();
void ir_ac_power();
void ir_ac_temp_up();
void ir_ac_temp_down();
void ir_ac_wind_speed();
void ir_ac_swing_enable();
void ir_ac_swing_disable();

void ir_fan_power();
void ir_fan_rotation_speed();

void ir_dump_task(void *arg);

void ir_ac_power_on();
void ir_ac_power_off();

void ir_ac_mode_cool();
void ir_ac_mode_heat();

void ir_ac_temp_22();
void ir_ac_temp_23();
void ir_ac_temp_24();
void ir_ac_temp_25();
void ir_ac_temp_26();
void ir_ac_temp_27();
void ir_ac_temp_28();
void ir_ac_temp_29();
void ir_ac_temp_30();

void ir_ac_fan_speed_auto();
void ir_ac_fan_speed_0();
void ir_ac_fan_speed_1();
void ir_ac_fan_speed_2();
void ir_ac_fan_speed_3();
void ir_ac_fan_speed_4();
void ir_ac_fan_speed_5();