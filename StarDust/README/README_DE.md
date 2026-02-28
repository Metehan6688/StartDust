# StarDust Protocol v2.0.0

Leichtgewichtiges, nicht-blockierendes, verschlüsseltes
UART-Kommunikationsprotokoll

Autor: Metehan Semerci\
Lizenz: MIT

------------------------------------------------------------------------

# Übersicht

**StarDust** wurde für Embedded-Systeme und RTOS- (Real-Time Operating
System) Umgebungen entwickelt und bietet:

-   Nicht-blockierenden Betrieb
-   CRC16-CCITT-Verifikation
-   XOR- + Bit-Rotations-basierte Verschlüsselung
-   Master / Multi-Slave-Architektur
-   Broadcast-Unterstützung

Es ermöglicht mehreren Geräten eine sichere Kommunikation über eine
einzige UART-Leitung ohne Kollisionen.

------------------------------------------------------------------------

# Protokollstruktur

Jedes Datenpaket hat folgende Struktur:

\[START\]\[HEADER\]\[PAYLOAD (64 Byte)\]\[CRC16\]

## Konstanten

  Konstante             Beschreibung
  --------------------- -------------------------------
  `PACKET_START_BYTE`   Startbyte des Pakets (0xAA)
  `PAYLOADSIZE`         Feste Payload-Größe (64 Byte)
  `BROADCAST_ID`        0xFF -- an alle Geräte senden

------------------------------------------------------------------------

# Grundlegende Verwendung

## 1. Objekterstellung

``` cpp
#include "StarDust.h"
StarDust sd;
```

## 2. Initialisierung

``` cpp
sd.begin(Serial1, 0x01, 0x02);
```

-   `myID` → ID dieses Geräts\
-   `defaultTargetID` → Standard-Zielgerät

## 3. Verschlüsselungsschlüssel

``` cpp
uint8_t newKey[16] = { /* 16 Byte */ };
sd.setCryptoKey(newKey);
```

## 4. Timeout-Einstellung

``` cpp
sd.setTimeout(50);
```

## 5. Ziel ändern

``` cpp
sd.setTargetID(0x05);
sd.setTargetID(BROADCAST_ID);
```

------------------------------------------------------------------------

# Nicht-blockierendes Lesen

``` cpp
PacketData packet;

if (sd.update(packet)) {
    // Gültiges Paket empfangen
}
```

-   UART blockiert nicht\
-   CRC-Verifikation wird durchgeführt\
-   Entschlüsselung erfolgt automatisch

Es wird empfohlen, dies innerhalb der Task-Schleife in RTOS-Umgebungen
aufzurufen.

------------------------------------------------------------------------

# Übertragungsfunktionen

Alle Übertragungsfunktionen:

-   Paket vorbereiten
-   CRC berechnen
-   Payload verschlüsseln
-   In UART schreiben
-   PacketData zurückgeben

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

Erweitertes benutzerdefiniertes Datenpaket.

------------------------------------------------------------------------

# Pakete empfangen

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

Jede Empfangsfunktion ordnet die Payload der entsprechenden Struktur zu.

------------------------------------------------------------------------

# Sicherheit

-   CRC16-CCITT-Verifikation
-   XOR- + Bit-Rotations-Verschlüsselung
-   Ziel-ID-Validierung
-   Broadcast-Filterung

Hinweis: Die Verschlüsselung ist leichtgewichtig und nicht
militärtauglich.

------------------------------------------------------------------------

# RTOS-Kompatibilität

-   Kein ISR erforderlich
-   Nicht-blockierende Architektur
-   Zustands-Reset über Timeout
-   Task-sichere Nutzung

------------------------------------------------------------------------

# Master / Slave Beispiel

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

# Lizenz

MIT-Lizenz\
Copyright (c) 2026 Metehan Semerci
