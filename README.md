# ESP32 Rotary Countdown Timer

This project is an ESP32-S3 based countdown timer controlled by a clickable rotary encoder.

## Features

- **Rotary Encoder Control**: Easily set the countdown time using a rotary encoder.
- **Clickable Button**: Start, pause, and reset the timer with a simple button press.
- **LCD display**: Visual feedback of the countdown time on a small LCD screen.

## Components Used

- Freenove ESP32-S3-WROOM-1 Board
  - doc `https://freenove.com/fnk0099`
  - amazon link: `https://www.amazon.ca/-/fr/dp/B0B76Z83Y4`
- Rotary Encoder with Click Button (5 pins)
  - amazon link: `https://www.amazon.ca/-/fr/dp/B09R41C1HR`
- Freenove I2C LCD 1602 module
  - doc `https://freenove.com/fnk0079`
  - amazon link: `https://www.amazon.ca/-/fr/dp/B0DHJZ1V81`

## Wiring

- Connect the rotary encoder to the ESP32 as follows:
  - CLK to GPIO 4
  - DT to GPIO 5
  - SW to GPIO 6
  - VCC to 3.3V
  - GND to GND

- Connect the I2C LCD to the ESP32 as follows:
  - SDA to GPIO 8
  - SCL to GPIO 9
  - VCC to 5V
  - GND to GND

## Requirements

- ESP-IDF framework

```bash
# Install ESP-IDF following the official guide: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/

# On Arch Linux, you can install it via AUR:
yay -S esp-idf

# Then, set up the environment by adding the following line to your shell configuration file (e.g., ~/.bashrc or ~/.zshrc):
export alias esp-init='source /opt/esp-idf/export.sh'

# After adding the line, run:
source ~/.bashrc  # or source ~/.zshrc
```

## Installation

Make sure the ESP32 is connected to your computer via USB.

```bash
# Clone the repository
git clone

cd esp32-rotary-countdown-timer

# Set up the ESP-IDF environment:
esp-init
# or 
source /opt/esp-idf/export.sh

# Build the project (the first build may take some time to download dependencies and toolchains)
# the next builds will be faster
idf.py build

# Flash the firmware to the ESP32
idf.py flash
# or specify the port
idf.py -p /dev/ttyACM0 flash

# Monitor the serial output
idf.py monitor
```

## Usage

The Timer will initialize and display "Set Time: 15min" on the LCD. (SETTING MODE)

- Rotate the encoder to set the desired countdown time. (from 1 minute to 120 minutes)
- Short click the button to start/pause the countdown.
  - SETTING MODE -> RUNNING MODE
  - RUNNING MODE -> PAUSED MODE
  - PAUSED MODE -> RUNNING MODE
- When the timer reaches 00:00, it will automatically switch back to SETTING MODE.
- Long click the button again to reset the timer. (> 1 second)
  - PAUSED MODE -> SETTING MODE
  - RUNNING MODE -> SETTING MODE
