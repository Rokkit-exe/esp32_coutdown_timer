#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/i2c_master.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

#include "lcd_i2c.h"

static const char *TAG = "LCD_I2C";

/* ===================== I2C CONFIG ===================== */
#define I2C_MASTER_SDA_IO   8
#define I2C_MASTER_SCL_IO   9
#define I2C_MASTER_FREQ_HZ  400000

#define LCD_ADDR            0x27

/* ===================== LCD BITS ===================== */
#define LCD_RS        0x01
#define LCD_RW        0x02
#define LCD_ENABLE    0x04
#define LCD_BACKLIGHT 0x08

/* ===================== LCD COMMANDS ===================== */
#define CMD_CLEAR             0x01
#define CMD_HOME              0x02
#define CMD_ENTRY             0x06
#define CMD_DISPLAY_ON        0x0C
#define CMD_FUNCTION_SET_4BIT 0x28
#define CMD_SET_DDRAM_ADDR    0x80
#define CMD_INIT_8BIT         0x30
#define CMD_INIT_4BIT         0x20

/* ===================== GLOBALS ===================== */
static i2c_master_bus_handle_t bus_handle = NULL;
static i2c_master_dev_handle_t lcd_dev = NULL;
static SemaphoreHandle_t i2c_mutex;

/* ===================== LOW LEVEL I2C =====================
 * Sends raw data over I2C to the LCD device
 * data: pointer to data buffer
 * len: length of data buffer
 * */
static esp_err_t lcd_i2c_tx(uint8_t *data, size_t len)
{
    const char *task = pcTaskGetName(NULL);

    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "[%s] I2C mutex timeout", task);
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = i2c_master_transmit(lcd_dev, data, len, -1);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[%s] i2c_master_transmit failed: %s",
                 task, esp_err_to_name(err));
    }

    xSemaphoreGive(i2c_mutex);
    return err;
}


/* ===================== WRITE NIBBLE =====================
 * Sends a single nibble (4 bits) to the LCD with proper EN toggling
 * nibble: upper 4 bits contain data to send
 * mode: LCD_RS for data, 0 for command
 * */
static esp_err_t lcd_write_nibble(uint8_t nibble, uint8_t mode)
{
    uint8_t data = (nibble & 0xF0) | mode | LCD_BACKLIGHT;

    uint8_t pkt[2] = {
        data | LCD_ENABLE,   // EN = 1
        data & ~LCD_ENABLE   // EN = 0
    };

    esp_err_t err = lcd_i2c_tx(pkt, sizeof(pkt));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Nibble TX failed: %s", esp_err_to_name(err));
        return err;
    }

    esp_rom_delay_us(50);  // LCD settle time
    return ESP_OK;
}


/* Send a full byte (2 nibbles) to the LCD */
static void lcd_send_byte(uint8_t val, uint8_t mode)
{
    esp_err_t err;
    err = lcd_write_nibble(val & 0xF0, mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send high nibble");
        return;
    }
    err = lcd_write_nibble((val << 4) & 0xF0, mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send low nibble");
        return;
    }
}

/* Helper to pack 2 nibbles (one byte) into our buffer
 * Each byte of data sent to LCD requires 2 I2C packets (EN=1 then EN=0) */
static void pack_byte_to_buffer(uint8_t *buf, int *idx, uint8_t val, uint8_t mode) {
    // High Nibble
    uint8_t high = (val & 0xF0) | mode | LCD_BACKLIGHT;
    buf[(*idx)++] = high | LCD_ENABLE;
    buf[(*idx)++] = high & ~LCD_ENABLE;
    
    // Low Nibble
    uint8_t low = ((val << 4) & 0xF0) | mode | LCD_BACKLIGHT;
    buf[(*idx)++] = low | LCD_ENABLE;
    buf[(*idx)++] = low & ~LCD_ENABLE;
}

/* ===================== CLEAR DISPLAY =====================
 * Clears the LCD display and returns cursor to home position
 * ** takes ~2ms to complete ** */
void lcd_clear(void)
{
    configASSERT(!xPortInIsrContext());
    lcd_send_byte(CMD_CLEAR, 0);
    esp_rom_delay_us(2000);
}


/* ===================== SEND ROW =====================
 * Sends a full row (16 characters) to the LCD in one I2C transaction
 * row: 0 or 1 for the two rows of the display
 * str: null-terminated string to display (will be padded/truncated to 16 chars)
 * */
esp_err_t lcd_send_row(uint8_t row, const char *str) {
    // 4 bytes for address + 64 bytes for 16 chars = 68 bytes total
    uint8_t buf[68];
    int idx = 0;
    uint8_t addr = (row == 0) ? (CMD_SET_DDRAM_ADDR | 0x00) : (0xC0); // 0xC0 is Row 1 start

    // 1. Pack the Cursor Address command
    pack_byte_to_buffer(buf, &idx, addr, 0);

    // 2. Pack the String (exactly 16 characters)
    for (int i = 0; i < 16; i++) {
        uint8_t c = (i < strlen(str)) ? (uint8_t)str[i] : (uint8_t)' ';
        pack_byte_to_buffer(buf, &idx, c, LCD_RS);
    }

    // 3. Set up the multi-buffer info
    i2c_master_transmit_multi_buffer_info_t info = {
        .write_buffer = buf,
        .buffer_size = 68
    };

    // 4. Transmit everything in ONE go
    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        // We use a 100ms timeout for the hardware to finish
        esp_err_t err = i2c_master_multi_buffer_transmit(lcd_dev, &info, 1, 100);
        xSemaphoreGive(i2c_mutex);
        return err;
    }
    
    return ESP_ERR_TIMEOUT;
}

/* ===================== INIT =====================
 * Initializes the I2C bus and LCD display with proper sequence
 * called ONCE before any other lcd_i2c functions are used
 * */
void lcd_i2c_init(void)
{
    configASSERT(!xPortInIsrContext());

    ESP_LOGI(TAG, "Initializing I2C bus");

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = -1,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LCD_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &lcd_dev));

    i2c_mutex = xSemaphoreCreateMutex();
    assert(i2c_mutex);

    vTaskDelay(pdMS_TO_TICKS(100));

    lcd_write_nibble(CMD_INIT_8BIT, 0);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_write_nibble(CMD_INIT_8BIT, 0);
    esp_rom_delay_us(150);

    lcd_write_nibble(CMD_INIT_8BIT, 0);
    esp_rom_delay_us(150);

    lcd_write_nibble(CMD_INIT_4BIT, 0);
    esp_rom_delay_us(150);

    lcd_send_byte(CMD_FUNCTION_SET_4BIT, 0);
    lcd_send_byte(CMD_DISPLAY_ON, 0);
    lcd_send_byte(CMD_ENTRY, 0);
    lcd_clear();
    ESP_LOGI(TAG, "LCD initialized");
}

