#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"

#include "encoder.h"
#include "lcd_i2c.h"
#include "timer.h"
#include "esp_timer.h"

#define ENCODER_CLK 4
#define ENCODER_DT  5
#define ENCODER_SW  6

#define DEFAULT_TIME_VALUE 15

static bool is_busy = false;

void draw(char* row1, char* row2) {
    if (is_busy) {
        // Prevent re-entrant calls
        return;
    }
    is_busy = true;
    static char last_row1[17] = {0};
    static char last_row2[17] = {0};

    if (strcmp(row1, last_row1) != 0) {
        if (lcd_send_row(0, row1) == ESP_OK) {
            strncpy(last_row1, row1, 16);
        } else {
            printf("Failed to send row 1: [%s]\n", row1);
        }
        vTaskDelay(pdMS_TO_TICKS(2)); // Short rest between rows
    }

    if (strcmp(row2, last_row2) != 0) {
        if (lcd_send_row(1, row2) == ESP_OK) {
            strncpy(last_row2, row2, 16);
        } else {
            printf("Failed to send row 2: [%s]\n", row2);
        }
    }
    is_busy = false;
}

char* format_title(enum Mode mode) {
    switch (mode) {
        case SETTING: return "Set Timer";
        case RUNNING: return "Timer Running";
        case PAUSED:  return "Timer Paused";
        default:      return "Unknown Mode";
    }
}

void us_to_min_sec(int64_t us, int* minutes, int* seconds)
{
    int64_t total_seconds = us / 1000000;
    *minutes = total_seconds / 60;
    *seconds = total_seconds % 60;
}

char* format_value(enum Mode mode, int64_t duration_us)
{
    static char value[16];

    int min, sec;
    us_to_min_sec(duration_us, &min, &sec);

    switch (mode) {
        case SETTING:
            snprintf(value, sizeof(value), "Time: %02d min", min);
            break;
        case RUNNING:
            snprintf(value, sizeof(value), "-> %02d:%02d <-", min, sec);
            break;
        case PAUSED:
            snprintf(value, sizeof(value), "|| %02d:%02d ||", min, sec);
            break;
        default:
            snprintf(value, sizeof(value), "Mode Unknown");
            break;
    }

    return value;
}

void app_main(void)
{      
    lcd_i2c_init();

    encoder_config_t encoder_config = {
        .clk_pin = ENCODER_CLK,
        .dt_pin = ENCODER_DT,
        .sw_pin = ENCODER_SW,
        .min_value = 1,
        .max_value = 120,
        .step = 1,
        .initial_value = DEFAULT_TIME_VALUE
    };

    Encoder* encoder = init_encoder(encoder_config);
    Timer* timer = init_timer(DEFAULT_TIME_VALUE);

    enum Mode mode = timer_get_mode(timer);
    draw(format_title(mode), format_value(mode, timer_remaining(timer)));

    while (1) {
        bool needs_update = false;

        RotationType rotation = encoder_check_rotation(encoder);
        if (rotation != NONE && timer_get_mode(timer) == SETTING) {
            needs_update = true;
        }

        ClickType click = encoder_check_click(encoder);

        if (click == SHORT_CLICK) {
            switch (timer_get_mode(timer)) {
              case SETTING:
                  timer_start(timer , min_to_us(encoder_get_value(encoder)));
                  break;
              case RUNNING:
                  timer_pause(timer);
                  break;
              case PAUSED:
                  timer_resume(timer);
                  break;
            }
            needs_update = true;
        } else if (click == LONG_CLICK) {
            timer_stop(timer);
            needs_update = true;
        }

        if (timer_expired(timer)) {
            timer_stop(timer);
            needs_update = true;
        } 

        if (timer_get_mode(timer) == RUNNING) {
            static int64_t last_remaining_us = -1;
            int64_t current_remaining_us = timer_remaining(timer);
            if (current_remaining_us / 1000000 != last_remaining_us / 1000000) {
                needs_update = true;
                last_remaining_us = current_remaining_us;
            }
        }

        if (needs_update) {
          int64_t display_us = (timer_get_mode(timer) == SETTING) ? 
                              min_to_us(encoder_get_value(encoder)) : 
                              timer_remaining(timer);

          char* t = format_title(timer_get_mode(timer));
          char* v = format_value(timer_get_mode(timer), display_us);

          printf("DRAW: [%s] [%s]\n", t, v);

          draw(t, v);
          needs_update = false;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

