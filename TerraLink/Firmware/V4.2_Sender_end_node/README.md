# TerraLink Sender — Modular ESP32 Dev Module

Based on the supplied refined sender firmware. The firmware is split by responsibility while retaining the ESP32 Arduino framework.

## Genuine C module
`packet_parser_c.c` is pure C.

## C++ modules
Arduino/ESP32 libraries require C++, so radio, GPS, OLED, Preferences, GPIO, FreeRTOS and String-dependent modules remain `.cpp`.

## Build
Open this folder as the PlatformIO project and use Build. Do not compile individual files with `g++`.
