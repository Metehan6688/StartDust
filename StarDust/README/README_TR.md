# StarDust Protocol v2.0.0

Lightweight, Non-Blocking, Encrypted UART Communication Protocol

Author: Metehan Semerci\
License: MIT

------------------------------------------------------------------------

# Genel Bakış

**StarDust**, gömülü sistemler ve RTOS (Real-Time Operating System)
ortamları için tasarlanmış:

-   Non-blocking (kilitlenmeyen)
-   CRC16-CCITT doğrulamalı
-   XOR + bit-rotation tabanlı şifrelemeli
-   Master / Multi-Slave mimarili
-   Broadcast destekli

bir seri haberleşme protokolüdür.

Tek bir UART hattı üzerinde birden fazla cihazın çakışmadan ve güvenli
şekilde konuşmasını sağlar.

------------------------------------------------------------------------

# Protokol Yapısı

Her veri paketi şu yapıya sahiptir:

\[START\]\[HEADER\]\[PAYLOAD (64 byte)\]\[CRC16\]

## Sabitler

  Sabit                 Açıklama
  --------------------- --------------------------------
  `PACKET_START_BYTE`   Paket başlangıç byte'ı (0xAA)
  `PAYLOADSIZE`         Sabit payload boyutu (64 byte)
  `BROADCAST_ID`        0xFF -- tüm cihazlara gönderim

------------------------------------------------------------------------

# Temel Kullanım

## 1. Nesne Oluşturma

``` cpp
#include "StarDust.h"
StarDust sd;
```

## 2. Başlatma

``` cpp
sd.begin(Serial1, 0x01, 0x02);
```

-   `myID` → Bu cihazın ID'si\
-   `defaultTargetID` → Varsayılan hedef cihaz

## 3. Şifreleme Anahtarı

``` cpp
uint8_t newKey[16] = { /* 16 byte */ };
sd.setCryptoKey(newKey);
```

## 4. Timeout Ayarı

``` cpp
sd.setTimeout(50);
```

## 5. Hedef Değiştirme

``` cpp
sd.setTargetID(0x05);
sd.setTargetID(BROADCAST_ID);
```

------------------------------------------------------------------------

# Non-Blocking Okuma

``` cpp
PacketData packet;

if (sd.update(packet)) {
    // Geçerli paket alındı
}
```

-   UART kilitlenmez\
-   CRC doğrulaması yapılır\
-   Şifre çözme otomatik gerçekleşir

RTOS ortamında task loop içinde çağrılması önerilir.

------------------------------------------------------------------------

# Gönderim Fonksiyonları

Tüm gönderim fonksiyonları:

-   Paket hazırlar
-   CRC hesaplar
-   Payload şifreler
-   UART'a yazar
-   PacketData döndürür

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

Geniş kapsamlı özel veri paketi.

------------------------------------------------------------------------

# Paket Alma

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

Her receive fonksiyonu payload'ı ilgili struct'a map eder.

------------------------------------------------------------------------

# Güvenlik

-   CRC16-CCITT doğrulama
-   XOR + bit-rotation şifreleme
-   Hedef ID kontrolü
-   Broadcast filtreleme

Not: Şifreleme hafif seviyededir, askeri seviye değildir.

------------------------------------------------------------------------

# RTOS Uyumluluğu

-   ISR gerekmez
-   Non-blocking yapı
-   Timeout ile state reset
-   Task-safe kullanım

------------------------------------------------------------------------

# Master / Slave Örneği

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

# Lisans

MIT License\
Copyright (c) 2026 Metehan Semerci
