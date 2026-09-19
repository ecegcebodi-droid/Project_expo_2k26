# TerraLink Relay1 - Refined Register-Level C Firmware

Target: Arduino Uno + SX1278/RA-02 LoRa (433 MHz)

## Structure
- `src/core/` - packet parsing, routing, relay logic
- `src/hal/` - SPI, LoRa register access, console adapter
- `include/` - public C headers and configuration
- `src/main.cpp` - Arduino entry-point wrapper only

## Important
The application and HAL are written in C. `main.cpp` is intentionally kept as a
very small C++ Arduino entry-point wrapper because the Arduino framework expects
`setup()` and `loop()` from the C++ build environment.

## Relay IDs
- Relay1: `TL_RELAY_NODE_ID = 2`
- Relay2: `TL_RELAY_NODE_ID = 3`

## Build
Open THIS extracted folder in VS Code/PlatformIO. Do not merge these files into
an older relay project. Delete `.pio` if it exists, then run Clean and Build.

## Warning cleanup
- Added the correct `millis()` declaration to the LoRa C module through `tl_time.h`.
- Reworked the forwarding-delay arithmetic to use explicit fixed-width types.
- Removed `-Wconversion` from PlatformIO flags because the bundled Arduino AVR
  framework itself emits many conversion warnings under that flag; this project
  uses `-Wall -Wextra` for normal source diagnostics.

The package contains no `.pio` build cache and no duplicate source files.
