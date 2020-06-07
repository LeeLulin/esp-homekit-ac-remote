#include <FreeRTOS.h>
#include <esp8266.h>
#include <esplibs/libmain.h>
#include <espressif/esp_system.h>
#include <event_groups.h>
#include <stdio.h>
#include <stdlib.h>
#include <task.h>

#include <dht/dht.h>

#include "homekit_callback.h"
#include "homekit_config.h"
#include "ir.h"

void led_write(bool on) { gpio_write(LED_GPIO, on ? 0 : 1); }

void ac_identify_task(void *_args) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      led_write(true);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      led_write(false);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    vTaskDelay(250 / portTICK_PERIOD_MS);
  }

  led_write(false);
  vTaskDelete(NULL);
}

void ac_identify(homekit_value_t _value) {
  printf("AC identify\n");
  xTaskCreate(ac_identify_task, "Thermostat identify", 128, NULL, 2, NULL);
};

homekit_value_t ac_active_get() { return HOMEKIT_UINT8(AC.active); };
bool isPower = false;
void ac_active_set(homekit_value_t value) {
  if (AC.active == value.bool_value)
    return;

  ac_active.value = value;
  AC.active = value.bool_value;
  AC.swing = false; // Swing mode is always reset to false
  if(AC.active){
    ir_ac_power_on();
  }else{
    ir_ac_power_off();
  }
};

homekit_value_t ac_target_temperature_get() {
  return HOMEKIT_FLOAT(AC.targetTemperature);
};

void ac_target_temperature_set(homekit_value_t value) {
  if (!AC.active) {
    printf("Can't change temperature while AC is off\n");
    homekit_characteristic_notify(&target_temperature,target_temperature.value);
    return;
  }

  printf("set target temp = %g\n", value.float_value);
  switch((int)value.float_value){
    case 22:
      ir_ac_temp_22();
      break;
    case 23:
      ir_ac_temp_23();
      break;
    case 24:
      ir_ac_temp_24();
      break;
    case 25:
      ir_ac_temp_25();
      break;
    case 26:
      ir_ac_temp_26();
      break;
    case 27:
      ir_ac_temp_27();
      break;
    case 28:
      ir_ac_temp_28();
      break;
    case 29:
      ir_ac_temp_29();
      break;
    case 30:
      ir_ac_temp_30();
      break;
    default:
      ir_ac_temp_26();
      break;
  }

  // int diff = value.float_value - AC.targetTemperature;
  // int times = abs(diff);
  // if ((xTaskGetTickCount() - AC.lastTargetTempChange) * portTICK_PERIOD_MS >=5000) {
  //   times++; // If last temperature change > 5 seconds from now, the first temp
  //            // up/down key will not change the temperature, only display
  //            // temperature on AC so we need to add one more time.
  // }
  // for (int i = 0; i < times; i++) {
  //   if (diff > 0) {
  //     ir_ac_temp_up();
  //   } else {
  //     ir_ac_temp_down();
  //   }
  //   vTaskDelay(100 / portTICK_PERIOD_MS);
  //   AC.lastTargetTempChange = xTaskGetTickCount();
  //   vTaskDelay(150 / portTICK_PERIOD_MS);
  // }
  AC.targetTemperature = value.float_value;
  target_temperature.value = value;
};

homekit_value_t ac_swing_get() { return HOMEKIT_UINT8(AC.swing); }
void ac_swing_set(homekit_value_t value) {
  AC.swing = value.bool_value;
  ac_swing_mode.value = value;
  if (value.bool_value) {
    ir_ac_swing_enable();
  } else {
    ir_ac_swing_disable();
  }
}

uint8_t speed_adjust_table[4][4] = {
  {0, 0, 0, 0}, 
  {0, 0, 1, 2}, 
  {0, 3, 0, 1}, 
  {0, 2, 3, 0}
};

homekit_value_t ac_speed_get() { return HOMEKIT_FLOAT(AC.rotationSpeed); }

void ac_speed_set(homekit_value_t value) {
  // if (value.float_value > 0 && // prevent losing state from power off
  //     value.float_value <= 4 && AC.active) {

  //   int times = speed_adjust_table[(int)AC.rotationSpeed][(int)value.float_value];

  //   for (int i = 0; i < times; i++) {
  //     ir_ac_wind_speed();
  //     vTaskDelay(250 / portTICK_PERIOD_MS);
  //   }

  //   ac_rotation_speed.value = value;
  //   AC.rotationSpeed = value.float_value;
  // } else {
  //   ac_rotation_speed.value = HOMEKIT_FLOAT(AC.rotationSpeed);
  //   homekit_characteristic_notify(&ac_rotation_speed, ac_rotation_speed.value);
  // }
}

homekit_value_t fan_active_get() { return HOMEKIT_UINT8(FAN.active); }

void fan_active_set(homekit_value_t value) {
  if (FAN.active == value.bool_value)
    return;

  fan_active.value = value;
  FAN.active = value.bool_value;

  ir_fan_power();
}

homekit_value_t fan_speed_get() { return HOMEKIT_FLOAT(FAN.rotationSpeed); }

void fan_speed_set(homekit_value_t value) {
  printf("FAN roation speed set: %f\n", value.float_value);
  if (value.float_value > 0 && // prevent losing state from power off
      value.float_value <= 4 && FAN.active) {

    int times =
      speed_adjust_table[(int)FAN.rotationSpeed][(int)value.float_value];

    for (int i = 0; i < times; i++) {
      ir_fan_rotation_speed();
      vTaskDelay(1100 / portTICK_PERIOD_MS);
    }

    fan_rotation_speed.value = value;
    FAN.rotationSpeed = value.float_value;
  } else {
    fan_rotation_speed.value = HOMEKIT_FLOAT(FAN.rotationSpeed);
    homekit_characteristic_notify(&fan_rotation_speed,
                                  fan_rotation_speed.value);
  }
}

void temperature_sensor_task(void *_args) {
  gpio_set_pullup(TEMPERATURE_SENSOR_GPIO, false, false);

  float humidity_value, temperature_value;
  while (1) {
    bool success = dht_read_float_data(
      DHT_TYPE_DHT11, 
      TEMPERATURE_SENSOR_GPIO,
      &humidity_value, &temperature_value);
    if (success) {
      printf("[DHT11] temperature %g℃, humidity %g%%\n", temperature_value,
             humidity_value);
      current_temperature.value = HOMEKIT_FLOAT(temperature_value);
      current_humidity.value = HOMEKIT_FLOAT(humidity_value);

      homekit_characteristic_notify(&current_temperature,
                                    current_temperature.value);
      homekit_characteristic_notify(&current_humidity, current_humidity.value);

      /* INACTIVE = 0; IDLE = 1; HEATING = 2; COOLING = 3; */
      homekit_value_t new_state_value = HOMEKIT_UINT8(0);
      if (AC.active) {
        if (AC.targetTemperature > current_temperature.value.float_value) {
          new_state_value = HOMEKIT_UINT8(1);
        } else {
          new_state_value = HOMEKIT_UINT8(3);
        }
      }
      if (!homekit_value_equal(&new_state_value,
                               &current_heater_cooler_state.value)) {
        current_heater_cooler_state.value = new_state_value;

        homekit_characteristic_notify(&current_heater_cooler_state,
                                      current_heater_cooler_state.value);
      }

    } else {
      printf("Couldn't read data from sensor\n");
    }

    vTaskDelay(TEMPERATURE_POLL_PERIOD / portTICK_PERIOD_MS);
  }
}
