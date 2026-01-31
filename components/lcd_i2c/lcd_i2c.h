// lcd_i2c.h
#pragma once

#include <stdint.h>
#include "driver/i2c_master.h"

void lcd_clear(void);
esp_err_t lcd_send_row(uint8_t row, const char *str);
void lcd_i2c_init(void);

