# StarDust Protocol v2.0.0

Protocolo de Comunicación UART Ligero, No Bloqueante y Cifrado

Autor: Metehan Semerci\
Licencia: MIT

------------------------------------------------------------------------

# Descripción General

**StarDust** está diseñado para sistemas embebidos y entornos RTOS
(Sistema Operativo en Tiempo Real) y ofrece:

-   Funcionamiento no bloqueante
-   Verificación CRC16-CCITT
-   Cifrado basado en XOR + rotación de bits
-   Arquitectura Master / Multi-Slave
-   Soporte de broadcast

Permite que múltiples dispositivos se comuniquen de forma segura a
través de una sola línea UART sin colisiones.

------------------------------------------------------------------------

# Estructura del Protocolo

Cada paquete de datos tiene la siguiente estructura:

\[START\]\[HEADER\]\[PAYLOAD (64 bytes)\]\[CRC16\]

## Constantes

  Constante             Descripción
  --------------------- -----------------------------------------
  `PACKET_START_BYTE`   Byte de inicio del paquete (0xAA)
  `PAYLOADSIZE`         Tamaño fijo del payload (64 bytes)
  `BROADCAST_ID`        0xFF -- enviar a todos los dispositivos

------------------------------------------------------------------------

# Uso Básico

## 1. Creación del Objeto

``` cpp
#include "StarDust.h"
StarDust sd;
```

## 2. Inicialización

``` cpp
sd.begin(Serial1, 0x01, 0x02);
```

-   `myID` → ID de este dispositivo\
-   `defaultTargetID` → Dispositivo objetivo por defecto

## 3. Clave de Cifrado

``` cpp
uint8_t newKey[16] = { /* 16 bytes */ };
sd.setCryptoKey(newKey);
```

## 4. Configuración de Timeout

``` cpp
sd.setTimeout(50);
```

## 5. Cambio de Destino

``` cpp
sd.setTargetID(0x05);
sd.setTargetID(BROADCAST_ID);
```

------------------------------------------------------------------------

# Lectura No Bloqueante

``` cpp
PacketData packet;

if (sd.update(packet)) {
    // Paquete válido recibido
}
```

-   UART no bloquea\
-   Se realiza verificación CRC\
-   El descifrado ocurre automáticamente

Se recomienda llamarlo dentro del bucle de tareas en entornos RTOS.

------------------------------------------------------------------------

# Funciones de Transmisión

Todas las funciones de transmisión:

-   Preparan el paquete
-   Calculan el CRC
-   Cifran el payload
-   Escriben en UART
-   Devuelven PacketData

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

Paquete de datos personalizado extendido.

------------------------------------------------------------------------

# Recepción de Paquetes

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

Cada función de recepción asigna el payload a la estructura
correspondiente.

------------------------------------------------------------------------

# Seguridad

-   Verificación CRC16-CCITT
-   Cifrado XOR + rotación de bits
-   Validación de ID de destino
-   Filtrado de broadcast

Nota: El cifrado es ligero y no es de nivel militar.

------------------------------------------------------------------------

# Compatibilidad con RTOS

-   No requiere ISR
-   Arquitectura no bloqueante
-   Reinicio de estado mediante timeout
-   Uso seguro en tareas

------------------------------------------------------------------------

# Ejemplo Master / Slave

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

# Licencia

Licencia MIT\
Copyright (c) 2026 Metehan Semerci
