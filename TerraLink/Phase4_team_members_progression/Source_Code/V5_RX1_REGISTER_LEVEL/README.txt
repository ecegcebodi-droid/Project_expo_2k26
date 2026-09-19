TERRALINK RX1 - REGISTER-LEVEL ESP8266 GATEWAY
===============================================

Target:
  ESP8266 NodeMCU v2 / nodemcuv2
  AI-Thinker RA-02 / SX1278 433 MHz

Important:
  The application is compiled as C++ because PlatformIO Arduino for ESP8266
  and the existing TerraLink stack use Arduino C++ objects. Hardware access
  for GPIO, SPI and LCD I2C is implemented at register/bit-bang level.
  The SX1278 is driven directly through its registers; the LoRa library is
  NOT required.

HARDWARE
--------
LoRa RA-02:
  NSS  -> D1 / GPIO5
  RST  -> D0 / GPIO16
  SCK  -> D5 / GPIO14
  MISO -> D6 / GPIO12
  MOSI -> D7 / GPIO13
  DIO0 -> NOT CONNECTED (RX polling via RegIrqFlags)

LCD 16x2 I2C PCF8574:
  SDA -> D2 / GPIO4
  SCL -> D4 / GPIO2
  ADDR -> 0x27

Buzzer:
  + -> D8 / GPIO15

UART RX1 -> RX2:
  RX1 D3 / GPIO0 -> RX2 D8
  GND -> GND
  Baud -> 9600

Do NOT connect RX1 D3 to the buzzer. D3 is reserved for UART TX.

TERRALINK RADIO PARAMETERS
--------------------------
Frequency : 433 MHz
Sync word : 0xF3
Spreading : SF7
Bandwidth : 125 kHz
Coding    : 4/5
CRC       : enabled
TX power  : PA boost, 17 dBm nominal configuration
Node ID   : 4
Broadcast : 255
Default TTL: 5

PACKET EXAMPLES
---------------
SOS:
SRC:1,DST:255,SEQ:12,TTL:5,HOP:0,LAT:11.234567,LON:76.123456,TYPE:SOS

ACK:
SRC:4,DST:1,SEQ:12,TTL:5,HOP:0,TYPE:ACK

MSG:
SRC:4,DST:1,SEQ:12,TTL:5,HOP:0,MSGID:1,TYPE:MSG,MSG:STAY CALM

MSGACK:
SRC:1,DST:4,SEQ:12,TTL:5,HOP:0,MSGID:1,TYPE:MSGACK

LOC:
SRC:1,DST:4,SEQ:12,TTL:5,HOP:0,LAT:11.234567,LON:76.123456,TYPE:LOC

ARCHITECTURE
------------
main.cpp
  -> tl_gateway
     -> tl_lora       direct SX1278 register driver
     -> tl_packet     packet parser / packet builder
     -> tl_state      duplicate filter + node statistics
     -> tl_uart       RX1 -> RX2 gateway telemetry
     -> tl_i2c_lcd    software I2C LCD driver
     -> tl_buzzer     register-level GPIO buzzer
     -> tl_spi        bit-banged SPI
     -> tl_registers  ESP8266 GPIO registers

BUILD
-----
Open this folder as a PlatformIO project.
Use:
  pio run -e rx1

Upload:
  pio run -e rx1 -t upload

Monitor:
  pio device monitor -b 115200

EXPECTED SERIAL OUTPUT
----------------------
TERRALINK RX1 - REGISTER LEVEL GATEWAY
[UART->RX2] RX1_STATUS,...
[UART->RX2] EVENT,SOS,...

DESIGN NOTE
-----------
RX1 is the final rescue station/gateway. Relay nodes perform dynamic
next-hop selection, TTL decrement, HOP increment, duplicate suppression
and forwarding. RX1 receives the final packet, acknowledges where required,
and forwards incident/communication telemetry to RX2.

The current physical design uses polling instead of DIO0 interrupt because
DIO0 is intentionally not wired. This is slower than an interrupt-driven
receiver but is deterministic and matches the established RX1 hardware.
