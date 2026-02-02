# esp32_encoder_component

esp32 component to interact with rotary encoder with push button

# Overview

This repository contains an ESP-IDF component that allows you to interface with a rotary encoder with push button.
The component provides functions to initialize the encoder, read rotation, and handle button presses.

# Features

- Initialize the rotary encoder
- Read rotation direction and steps [clockwise/counter-clockwise]
- Handle push button presses with debounce [single, long press]
- Compatible with ESP-IDF framework

# Requirements

- ESP-IDF v4.0 or later
- Rotary encoder with push button module
- ESP32 development board

# API

```c
#include "encoder.h"

#define ENCODER_CLK_GPIO     4         /*!< GPIO number used for encoder clock */
#define ENCODER_DT_GPIO      5        /*!< GPIO number used for encoder data  */
#define ENCODER_SW_GPIO      6       /*!< GPIO number used for encoder switch */

encoder_config_t encoder_config = {
        .clk_pin = ENCODER_CLK_GPIO,
        .dt_pin = ENCODER_DT_GPIO,
        .sw_pin = ENCODER_SW_GPIO,
        .min_value = 1,                 // Minimum value for the encoder
        .max_value = 120,               // Maximum value for the encoder
        .step = 1,                      // Step value for each detent
        .initial_value = DEFAULT_VALUE  // Initial value of the encoder
    };

Encoder* encoder = init_encoder(encoder_config);

// Check rotation (this will update the encoder state internally according to configuration)
RotationType rotation = encoder_check_rotation(encoder); 

switch (rotation) {
    case CLOCKWISE:
        // Handle clockwise rotation
        break;
    case COUNTERCLOCKWISE:
        // Handle counter-clockwise rotation
        break;
    case NONE:
        // No rotation detected
        break;
    default:
        break;
}

// Check button click type
ClickType click = encoder_check_click(encoder);

switch (click) {
    case SHORT_CLICK:
        // Handle single click
        break;
    case LONG_CLICK:
        // Handle long press
        break;
    case NO_CLICK:
        // No click detected
        break;
    default:
        break;
}
```
