TerraLink RX2 — modular register-oriented build
===============================================

IMPORTANT:
This is an ESP8266 Arduino/PlatformIO project. A genuinely pure `.c` build
cannot use ESP8266WebServer, WiFi, LittleFS File, SoftwareSerial, or Arduino
String because those are C++ APIs. Therefore the application is split into
`.cpp/.h` modules, while keypad GPIO operations are isolated in
`tl_registers.h` as low-level helpers.

This preserves the requested architecture:
  main controller
  HAL / keypad / UART
  application / incidents / commands
  storage / LittleFS
  UI
  network / dashboard

Build:
  pio run
Upload:
  pio run -t upload
Monitor:
  pio device monitor -b 115200

Hardware:
UART RX D8(GPIO15), TX D0(GPIO16), 9600
Keypad C: D1,D2,D5,D6
Keypad R: D7,GPIO3,D4,D3

AP:
SSID TerraLink-RX2
Password terralink123

If the original RX2 source has additional dashboard HTML or protocol fields
beyond the supplied version, merge those handlers into tl_dashboard.cpp or
tl_incident.cpp without moving the hardware layer.
