#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "encoder.h"
#include "driver/gpio.h"
#include <stdint.h>

#define LONG_PRESS_DURATION_MS 1000

/* ===================== ENCODER STRUCT =====================
* Holds the state and configuration of the rotary encoder with button (5 pins)
* */
struct Encoder {
  int value;
  int min;
  int max;
  int step;
  int last_clk;
  int last_state;
  uint32_t press_start_time;
  bool is_pressed;
  int clk_pin;
  int dt_pin;
  int sw_pin;
};

/* ===================== INIT ENCODER =====================
* Initializes GPIO pins and encoder state
* config: encoder configuration struct
* returns: pointer to Encoder struct
* */
Encoder* init_encoder(encoder_config_t config) {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << config.clk_pin) | (1ULL << config.dt_pin) | (1ULL << config.sw_pin),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&io_conf);

    Encoder* e = calloc(1, sizeof(Encoder));
    if (!e) return NULL;

    e->value = config.initial_value;
    e->min = config.min_value;
    e->max = config.max_value;
    e->step = config.step;
    e->clk_pin = config.clk_pin;
    e->dt_pin = config.dt_pin;
    e->sw_pin = config.sw_pin;
    e->last_clk = gpio_get_level(e->clk_pin);
    return e;
}

/* ===================== CHECK CLICK =====================
* Checks the button state and determines click type
* e: pointer to Encoder struct
* returns: ClickType enum indicating click type
* */
ClickType encoder_check_click(Encoder* e) {
  if (!e) return NO_CLICK;
  int current_sw = gpio_get_level(e->sw_pin);
  ClickType result = NO_CLICK;

  // Button is pressed (Assuming pull-up: 0 = pressed)
  if (current_sw == 0 && !e->is_pressed) {
      e->press_start_time = xTaskGetTickCount();
      e->is_pressed = true;
  } 
  // Button is released
  else if (current_sw == 1 && e->is_pressed) {
      uint32_t duration = (xTaskGetTickCount() - e->press_start_time) * portTICK_PERIOD_MS;
      e->is_pressed = false;

      if (duration >= LONG_PRESS_DURATION_MS) {
          result = LONG_CLICK;
      } else if (duration > 50) {
          result = SHORT_CLICK;
      }
  }

  return result;
}

/* ===================== CHECK ROTATION =====================
* Checks the rotary encoder state and determines rotation direction
* updates the encoder value accordingly
* e: pointer to Encoder struct
* returns: RotationType enum indicating rotation direction
* */
RotationType encoder_check_rotation(Encoder* e) {
    if (!e) return NONE;

    int current_clk = gpio_get_level(e->clk_pin);
    RotationType result = NONE;

    // Only look for the falling edge (1 to 0 transition)
    // This is often cleaner than the rising edge on cheap encoders
    if (e->last_clk == 1 && current_clk == 0) {
        int current_dt = gpio_get_level(e->dt_pin);
        
        // If DT is high while CLK just went low, it's one direction
        // If DT is low while CLK just went low, it's the other
        if (current_dt != current_clk) {
            e->value += e->step;
            if (e->value > e->max) e->value = e->max;
            result = CLOCKWISE;
        } else {
            e->value -= e->step;
            if (e->value < e->min) e->value = e->min;
            result = COUNTERCLOCKWISE;
        }
    }

    e->last_clk = current_clk;
    return result;
}

/* ===================== GET VALUE =====================
* Returns the current value of the encoder
* e: pointer to Encoder struct
* returns: current encoder value
* */
int encoder_get_value(Encoder* e) {
    if (!e) return 0;
    return e->value;
}

