# StarDust Protocol v2.0.0

Lightweight, Non-Blocking, Encrypted UART Communication Protocol

Author: Metehan Semerci\
License: MIT

------------------------------------------------------------------------

# Overview

**StarDust** is designed for embedded systems and RTOS (Real-Time
Operating System) environments and features:

-   Non-blocking operation
-   CRC16-CCITT verification
-   XOR + bit-rotation based encryption
-   Master / Multi-Slave architecture
-   Broadcast support

It enables multiple devices to communicate securely over a single UART
line without collisions.

------------------------------------------------------------------------

# Protocol Structure

Each data packet has the following structure:

\[START\]\[HEADER\]\[PAYLOAD (64 byte)\]\[CRC16\]

## Constants

  Constant              Description
  --------------------- ------------------------------
  `PACKET_START_BYTE`   Packet start byte (0xAA)
  `PAYLOADSIZE`         Fixed payload size (64 byte)
  `BROADCAST_ID`        0xFF -- send to all devices

------------------------------------------------------------------------

# Basic Usage

## 1. Object Creation

``` cpp
#include "StarDust.h"
StarDust sd;
```

## 2. Initialization

``` cpp
sd.begin(Serial1, 0x01, 0x02);
```

-   `myID` → This device's ID\
-   `defaultTargetID` → Default target device

## 3. Encryption Key

``` cpp
uint8_t newKey[16] = { /* 16 byte */ };
sd.setCryptoKey(newKey);
```

## 4. Timeout Setting

``` cpp
sd.setTimeout(50);
```

## 5. Changing Target

``` cpp
sd.setTargetID(0x05);
sd.setTargetID(BROADCAST_ID);
```

------------------------------------------------------------------------

# Non-Blocking Read

``` cpp
PacketData packet;

if (sd.update(packet)) {
    // Valid packet received
}
```

-   UART does not block\
-   CRC verification is performed\
-   Decryption happens automatically

It is recommended to call this inside the task loop in RTOS
environments.

------------------------------------------------------------------------

# Transmission Functions

All transmission functions:

-   Prepare the packet
-   Calculate CRC
-   Encrypt the payload
-   Write to UART
-   Return PacketData

## sendRequest

``` cpp
sd.sendRequest(version, requestType, isCritical);
```

## sendAccept

``` cpp
sd.sendAccept(version, acceptType, accepted);
```

## sendRefuse

``` cpp
sd.sendRefuse(version, refuseType, refused);
```

## sendTelemetry

``` cpp
sd.sendTelemetry(version, lat, lon, alt, yaw, pitch, timestamp, status);
```

## sendCommand

``` cpp
sd.sendCommand(version, targetYaw, targetPitch, actionCode);
```

## sendError

``` cpp
sd.sendError(version, errorCode, errorLocation, errorSeverity);
```

## sendEmergency

``` cpp
sd.sendEmergency(version, emergencyCode, emergencyLocation, emergencySeverity);
```

## sendSystemCommand

``` cpp
sd.sendSystemCommand(version, commandCode, commandParameter, senderID, authorityLevel);
```

## sendSystemInfo

``` cpp
sd.sendSystemInfo(version, infoType, systemOperational, systemLoad,
                  systemTemperature, systemVoltage, uptime,
                  systemErrorChance, expectedErrorChance, systemReliability);
```

## sendSystemHeartbeat

``` cpp
sd.sendSystemHeartbeat(version, beatStatus,
                       systemErrorChance, expectedErrorChance,
                       missedBeats);
```

## sendBetrayal

Extended custom data packet.

------------------------------------------------------------------------

# Receiving Packets

``` cpp
if (sd.update(packet)) {
    switch(packet.header.type) {

        case REQUEST:
            auto req = sd.receiveRequest(packet);
            break;

        case TELEMETRY:
            auto tel = sd.receiveTelemetry(packet);
            break;

        case COMMAND:
            auto cmd = sd.receiveCommand(packet);
            break;
    }
}
```

Each receive function maps the payload to the corresponding struct.

------------------------------------------------------------------------

# Security

-   CRC16-CCITT verification
-   XOR + bit-rotation encryption
-   Target ID validation
-   Broadcast filtering

Note: Encryption is lightweight and not military-grade.

------------------------------------------------------------------------

# RTOS Compatibility

-   No ISR required
-   Non-blocking architecture
-   State reset via timeout
-   Task-safe usage

------------------------------------------------------------------------

# Master / Slave Example

Master:

``` cpp
sd.setTargetID(2);
sd.sendCommand(...);
```

Slave:

``` cpp
if(sd.update(packet)) {
    if(packet.header.type == COMMAND) {
        auto cmd = sd.receiveCommand(packet);
    }
}
```

------------------------------------------------------------------------

# License

MIT License\
Copyright (c) 2026 Metehan Semerci
