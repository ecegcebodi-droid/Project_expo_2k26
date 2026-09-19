# TerraLink Sender — Register-Level Embedded C

This project restructures the TerraLink ESP32 sender as a C-only embedded firmware
architecture, with the low-level hardware layer written around ESP32 peripheral
registers and C APIs.

## Architecture

- `src/hal/` — GPIO, SPI, UART, I2C, timer, interrupt and power abstraction
- `src/drivers/` — SX1278, GPS and SH1106 device drivers
- `src/protocol/` — packet format/parser/routing
- `src/application/` — SOS, message, sequence and persistent storage
- `src/ui/` — input, buzzer and display application logic
- `src/system/` — system controller, state machine and global state
- `src/main.c` — firmware entry point

## Important

This is intentionally **C-only**. It does not use Arduino `.cpp` libraries such as
LoRa, U8g2, TinyGPSPlus, HardwareSerial, Preferences, `String`, `pinMode()`,
`digitalWrite()`, `SPI.begin()` or `Wire.begin()`.

The SX1278 driver communicates through the ESP32 SPI peripheral and SX1278
registers. GPIO uses ESP32 GPIO registers. UART and I2C use the ESP-IDF C APIs
and low-level peripheral access where appropriate. NVS and deep sleep use the
ESP-IDF C APIs rather than Arduino wrappers.

## Hardware

ESP32-WROOM / ESP32 Dev Module

### SX1278 / RA-02
SCK  = GPIO18
MISO = GPIO19
MOSI = GPIO23
NSS  = GPIO5
RST  = GPIO27
DIO0 = GPIO26

### GPS NEO-M8N
RX = GPIO16
TX = GPIO17
UART2, 9600 8N1

### SH1106 OLED
SDA = GPIO21
SCL = GPIO22

### User I/O
Touch/SOS = GPIO32
Local alarm = GPIO33
LED button = GPIO13
Buzzer = GPIO25
Torch = GPIO4

### TerraLink radio configuration
Frequency = 433 MHz
Sync word = 0xF3
Spreading factor = SF7
Bandwidth = 125 kHz
Coding rate = 4/5
CRC = enabled
TX power = 17 dBm

## Build

Open this folder in PlatformIO and build the `esp32dev` environment.

This source is a register-level/C architecture baseline. Verify the ESP32 target
variant and peripheral register definitions against the exact ESP-IDF version
used by the project before hardware deployment.
