#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct Encoder Encoder;

/* ===================== ENCODER CONFIG =====================
 * Configuration struct for initializing the rotary encoder
 * clk_pin: GPIO pin number for CLK
 * dt_pin: GPIO pin number for DT
 * sw_pin: GPIO pin number for SW (button)
 * min_value: Minimum value for the encoder
 * max_value: Maximum value for the encoder
 * step: Step size for each detent
 * initial_value: Starting value for the encoder
 * */
typedef struct {
    int clk_pin;
    int dt_pin;
    int sw_pin;
    int min_value;
    int max_value;
    int step;
    int initial_value;
} encoder_config_t;

/* ===================== ClickType =====================
 * Enum for different types of button clicks
 * NO_CLICK: No click detected
 * SHORT_CLICK: Short button press detected
 * LONG_CLICK: Long button press detected (> 1 second)
 * */
typedef enum {
    NO_CLICK,
    SHORT_CLICK,
    LONG_CLICK
} ClickType;

/* ===================== RotationType =====================
 * Enum for rotary encoder rotation direction
 * NONE: No rotation detected
 * CLOCKWISE: Clockwise rotation detected
 * COUNTERCLOCKWISE: Counterclockwise rotation detected
 * */
typedef enum {
    NONE,
    CLOCKWISE,
    COUNTERCLOCKWISE
} RotationType;

Encoder* init_encoder(encoder_config_t config);
ClickType encoder_check_click(Encoder* e);
RotationType encoder_check_rotation(Encoder* e);
int encoder_get_value(Encoder* e);
