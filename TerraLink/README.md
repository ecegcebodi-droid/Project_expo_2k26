# TerraLink

## Dynamic Off-Grid Emergency Communication System

TerraLink is a LoRa-based, off-grid emergency communication system designed to provide reliable communication in areas where conventional cellular or internet connectivity is unavailable or unreliable.

The system uses a distributed network of end nodes, relay nodes, and receiver nodes to transmit emergency information across remote and disaster-prone environments. Its multi-hop architecture enables packets to be forwarded through relay nodes while supporting dynamic routing and failure-aware communication.

**Project:** TerraLink
**Team:** QuadCore
**Institution:** Government College of Engineering, Bodinayakanur
**Domain:** Embedded Systems | Wireless Communication | IoT | Emergency Networks

---

## Overview

During disasters and in remote environments, communication infrastructure such as cellular networks and internet connectivity may become unavailable.

TerraLink addresses this problem by creating a local, infrastructure-independent communication network using LoRa.

### Network Architecture

```text
                         ┌──────────────────┐
                         │    Sender Node   │
                         │   ESP32 + LoRa   │
                         └────────┬─────────┘
                                  │
                                  ▼
                         ┌──────────────────┐
                         │    Relay Node    │
                         │   LoRa Router    │
                         └────────┬─────────┘
                                  │
                         ┌────────┴────────┐
                         │                 │
                         ▼                 ▼
                  ┌──────────────┐  ┌──────────────┐
                  │  Relay Node  │  │  Relay Node  │
                  └──────┬───────┘  └──────┬───────┘
                         │                 │
                         └────────┬────────┘
                                  ▼
                         ┌──────────────────┐
                         │ Receiver Gateway │
                         │     ESP8266      │
                         └────────┬─────────┘
                                  │
                                  ▼
                         ┌──────────────────┐
                         │ Rescue Command   │
                         │      Center      │
                         └──────────────────┘
```

The core emergency communication path does not depend on cellular towers or continuous internet connectivity.

---

## Key Features

* LoRa-based off-grid communication
* Multi-hop packet forwarding
* Dynamic route selection
* Failure-aware communication
* Emergency SOS transmission
* Acknowledgement-based communication
* GPS-based location reporting
* Packet sequence management
* Duplicate packet suppression
* TTL-based packet control
* Local receiver gateway
* Operator command interface
* Offline communication capability
* Persistent emergency-state handling
* Modular embedded firmware

---

## System Components

### Sender / End Node

The sender node is carried by the user and provides the primary emergency interface.

The current implementation is based on an ESP32 with LoRa communication and supporting peripherals.

Responsibilities include:

* SOS generation
* User interaction
* GPS location acquisition
* LoRa packet transmission
* Message reception
* Acknowledgement handling
* Display and status indication
* Persistent emergency-state management
* Low-power operation

Current firmware:

```text
firmware/
└── sender/
    └── V3_sender_end_node/
```

### Relay Node

Relay nodes extend the communication range of the TerraLink network by forwarding packets between nodes.

Responsibilities include:

* Packet forwarding
* Hop tracking
* TTL management
* Duplicate suppression
* Route awareness
* Dynamic next-hop selection
* Failure handling

Current firmware:

```text
firmware/
└── relay/
    └── V2_relay_node/
```

### Receiver Gateway

The receiver subsystem provides the interface between the LoRa network and the local rescue infrastructure.

The current implementation uses ESP8266-based nodes:

```text
V3_Receiver_node/
├── ESP8266_Rx1.cpp
└── ESP8266_Rx2.cpp
```

The receiver architecture separates gateway communication from the operator interface.

**Receiver 1 — Gateway**

* LoRa packet reception
* Gateway processing
* Local status indication
* Communication with Receiver 2

**Receiver 2 — Operator Interface**

Provides the local operator interface for interacting with the TerraLink network and forms the basis of the Rescue Command Center.

---

## Communication Protocol

TerraLink uses structured packets for communication between nodes.

```text
┌─────┬─────┬─────┬─────┬─────┬──────┬─────────┐
│ SRC │ DST │ SEQ │ TTL │ HOP │ TYPE │ PAYLOAD │
└─────┴─────┴─────┴─────┴─────┴──────┴─────────┘
```

| Field     | Description                   |
| --------- | ----------------------------- |
| `SRC`     | Source node identifier        |
| `DST`     | Destination node identifier   |
| `SEQ`     | Packet sequence number        |
| `TTL`     | Packet forwarding lifetime    |
| `HOP`     | Number of forwarding hops     |
| `TYPE`    | Packet/message classification |
| `PAYLOAD` | Application data              |

The protocol supports application-level communication as well as network-level routing operations.

### Packet Types

Current protocol concepts include:

```text
SOS
ACK
MSG
MSGACK
LOC
LOCREQ
RREQ
RREP
RERR
```

---

## Dynamic Routing

A key objective of TerraLink is to move beyond fixed-path LoRa communication.

A relay node may become unavailable because of power failure, hardware failure, physical obstruction, node movement, radio-link degradation, or environmental conditions.

TerraLink therefore uses a dynamic routing approach in which relay nodes can make forwarding decisions based on the available network state.

The routing design incorporates lightweight on-demand routing concepts such as:

* Route discovery
* Route response
* Route error handling
* Acknowledgements
* Route information maintained by relay nodes
* Dynamic next-hop selection

### Example

```text
Normal Route

Sender
  │
  ▼
Relay A
  │
  ▼
Relay B
  │
  ▼
Receiver


Relay A Unavailable

Sender
  │
  ├──────────► Relay C
  │               │
  │               ▼
  │            Relay B
  │               │
  └───────────────┘
                  │
                  ▼
               Receiver
```

The objective is to allow communication paths to adapt to changes in network availability rather than relying permanently on a single predefined route.

---

## Emergency Communication

The primary SOS communication flow is:

```text
User activates SOS
        │
        ▼
Sender creates SOS packet
        │
        ▼
LoRa transmission
        │
        ▼
Relay network
        │
        ▼
Receiver gateway
        │
        ▼
Rescue Command Center
        │
        ▼
Operator response
```

The sender can include available GPS location information with the emergency packet.

Acknowledgement mechanisms provide feedback regarding communication and packet delivery.

---

## Bidirectional Messaging

TerraLink is designed to support communication in both directions.

### Field to Command Center

```text
Field Node
    │
    │ SOS / MSG / LOC
    ▼
Relay Network
    │
    ▼
Receiver
    │
    ▼
Command Center
```

### Command Center to Field

```text
Command Center
    │
    │ MSG / ACK / Response
    ▼
Receiver
    │
    ▼
Relay Network
    │
    ▼
Field Node
```

This allows rescue operators to communicate with field users instead of only receiving emergency alerts.

---

## Hardware

| Component           | Current Platform                    |
| ------------------- | ----------------------------------- |
| Sender              | ESP32                               |
| Relay               | Arduino / LoRa-based relay platform |
| Receiver            | ESP8266                             |
| LoRa Transceiver    | AI-Thinker RA-02 / SX1278           |
| GPS                 | NEO-series GPS module               |
| Display             | OLED                                |
| Operator Input      | 4×4 keypad                          |
| Wireless Technology | LoRa                                |

Hardware-specific documentation is maintained separately from the firmware source.

---

## Firmware

TerraLink maintains separate PlatformIO projects for the major system nodes.

```text
firmware/
│
├── sender/
│   └── V3_sender_end_node/
│
├── relay/
│   └── V2_relay_node/
│
└── receiver/
    └── V3_Receiver_node/
```

Each target is maintained as an independent PlatformIO project.

The firmware is progressively being modularized to separate communication, protocol processing, routing, hardware drivers, user interface, storage, and system control.

---

## Development Environment

* Visual Studio Code
* PlatformIO
* Arduino Framework
* ESP32 toolchain
* ESP8266 toolchain
* Git
* GitHub

---

## Build and Upload

Each firmware target can be built independently using PlatformIO.

### Sender

```bash
cd firmware/sender/V3_sender_end_node
pio run
```

### Relay

```bash
cd firmware/relay/V2_relay_node
pio run
```

### Receiver

```bash
cd firmware/receiver/V3_Receiver_node
pio run
```

### Upload

```bash
pio run --target upload
```

### Serial Monitor

```bash
pio device monitor
```

The appropriate PlatformIO environment and connected hardware must be verified before uploading firmware.

---

## Repository Structure

```text
TerraLink/
│
├── README.md
├── docs/
│   ├── architecture/
│   ├── hardware/
│   ├── protocol/
│   └── research/
│
├── firmware/
│   ├── sender/
│   ├── relay/
│   └── receiver/
│
├── hardware/
│   ├── sender/
│   ├── relay/
│   └── receiver/
│
├── experiments/
├── project-expo/
├── archive/
├── LICENSE
└── .gitignore
```

| Directory       | Purpose                                    |
| --------------- | ------------------------------------------ |
| `docs/`         | Technical documentation                    |
| `firmware/`     | Current embedded firmware                  |
| `hardware/`     | Hardware designs and documentation         |
| `experiments/`  | Experimental and prototype implementations |
| `project-expo/` | Project Expo documentation                 |
| `archive/`      | Historical or deprecated material          |

---

## Development Status

TerraLink is currently under active development.

Current development areas include:

* Sender firmware modularization
* Relay routing implementation
* Dynamic route management
* Receiver gateway integration
* Rescue Command Center
* Operator messaging
* Packet reliability
* Hardware integration
* Network testing
* Field validation

The repository may contain both stable implementations and experimental components.

---

## Project Evolution

```text
Initial Concept
      │
      ▼
Communication Prototype
      │
      ▼
LoRa Emergency Node
      │
      ▼
Multi-Node Communication
      │
      ▼
Relay-Based Network
      │
      ▼
Dynamic Routing
      │
      ▼
Rescue Command Center
```

Historical implementations are retained separately to preserve the development history of TerraLink.

---

## Applications

Potential application areas include:

* Disaster response
* Search and rescue
* Remote villages
* Forest and hill regions
* Flood-affected areas
* Landslide-prone regions
* Emergency field operations
* Communication infrastructure failure scenarios

---

## Limitations

TerraLink is currently an academic research and engineering prototype.

Actual communication performance depends on:

* Antenna characteristics
* Transmit power
* Frequency configuration
* Terrain
* Obstacles
* Radio interference
* Node placement
* Battery capacity
* Environmental conditions

Quantitative range and reliability claims should therefore be based on measured laboratory and field-test results.

---

## Team

### Team QuadCore

Government College of Engineering, Bodinayakanur

**Team Members**

* R. Boomika
* R. Deepika
* S. Gopika
* Aswathi Sunil Kumar



## Project

**TerraLink — Dynamic Off-Grid Emergency Communication System**

Developed as part of **Project Expo 2026 / Niral Thiruvizha 3.0**.

---

## License

This project is developed primarily for academic, research, and prototype purposes.s.
