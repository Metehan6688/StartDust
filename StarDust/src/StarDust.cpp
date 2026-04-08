/*

MIT License

Copyright (c) 2026 Metehan Semerci

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

StarDust Communication Protocol Implementation
Author: Metehan Semerci
Contact: furkanmetehansemerci@gmail.com


That is a library for the StarDust communication protocol, designed for secure and 
efficient data exchange in embedded systems. It provides a structured way to send and
receive various types of packets, including requests, telemetry, commands, errors, and more.

The library includes features such as CRC validation, payload encryption/decryption, 
and a non-blocking parser for real-time applications.

*/






#include "StarDust.h"

// Başlangıç için varsayılan bir şifreleme anahtarı
// Default encryption key for initial use
const uint8_t DEFAULT_KEY[16] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
};

StarDust::StarDust() {
    _port = nullptr;
    _myID = 0x01;
    _targetID = 0x00;
    _state = ParserState::WAIT_START;
    _bytesRead = 0;
    _lastRxTime = 0;
    _timeoutMs = 100; // Varsayılan timeout 100 milisaniye // Default timeout 100 milliseconds
    memcpy(_cryptoKey, DEFAULT_KEY, 16);
    memset(_rxBuffer, 0, sizeof(_rxBuffer));
}

void StarDust::begin(Stream& port, uint8_t myID, uint8_t defaultTargetID) {
    _port = &port;
    _myID = myID;
    _targetID = defaultTargetID;
}

void StarDust::setCryptoKey(const uint8_t* newKey) {
    if (newKey != nullptr) {
        memcpy(_cryptoKey, newKey, 16); // Dinamik anahtar değişimi // Dynamic key change
    }
}

void StarDust::setTimeout(uint32_t timeoutMs) {
    _timeoutMs = timeoutMs;
}

void StarDust::setTargetID(uint8_t targetID) {
    _targetID = targetID;
}

// ======================== GÜVENLİK VE CRC ========================
// ======================== SECURITY AND CRC =======================

uint16_t StarDust::calculateCRC16CCITT(const uint8_t* data, uint16_t length) {
    uint16_t crc = 0xFFFF; //
    for (uint16_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021; //
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void StarDust::encryptPayload(uint8_t* payload, uint8_t size) {
    for (uint8_t i = 0; i < size; i++) {
        payload[i] ^= _cryptoKey[i % 16]; //
        payload[i] = ((payload[i] << 3) | (payload[i] >> 5)) & 0xFF; //
    }
}

void StarDust::decryptPayload(uint8_t* payload, uint8_t size) {
    for (uint8_t i = 0; i < size; i++) {
        payload[i] = ((payload[i] >> 3) | (payload[i] << 5)) & 0xFF; //
        payload[i] ^= _cryptoKey[i % 16]; //
    }
}


// ======================== PAKET İŞLEME VE PARSER ========================
// ===================== PACKET PROCESSING AND PARSER =====================
void StarDust::preparePacket(PacketData& packet, PacketType type, const uint8_t* payloadData, uint8_t payloadSize) {
    packet.header.start = PACKET_START_BYTE; //
    packet.header.source = _myID;
    packet.header.target = _targetID;
    packet.header.type = type;
    packet.header.size = payloadSize;

    memset(packet.payload, 0, PAYLOADSIZE);
    memcpy(packet.payload, payloadData, payloadSize);

    // CRC hesaplama Düz metin (plaintext) üzerinden yapılır
    // CRC is calculated on the plaintext (before encryption)
    packet.crc = calculateCRC16CCITT((uint8_t*)&packet, sizeof(PacketHeader) + PAYLOADSIZE);

    // Ardından payload şifrelenir
    // Then the payload is encrypted
    encryptPayload(packet.payload, PAYLOADSIZE);
}

bool StarDust::validatePacket(PacketData* packet) {
    if (packet->header.start != PACKET_START_BYTE) return false;

    // Şifreyi çöz
    // Decrypt the payload
    decryptPayload(packet->payload, PAYLOADSIZE);

    // Çözülmüş halinin CRC'sini hesapla
    // Calculate CRC of the decrypted payload
    uint16_t calcCRC = calculateCRC16CCITT((uint8_t*)packet, sizeof(PacketHeader) + PAYLOADSIZE);

    // CRC eşleşiyorsa ve paket bize veya genel yayına (broadcast) gelmişse geçerlidir
    // If CRC matches and the packet is addressed to us or is a broadcast, it's valid
    if (calcCRC == packet->crc) {
        if (packet->header.target == _myID || packet->header.target == BROADCAST_ID) {
            return true;
        }
    }
    return false;
}

bool StarDust::parseByte(uint8_t incomingByte, PacketData& outPacket) {
    switch (_state) {
        case ParserState::WAIT_START:
            if(incomingByte == PACKET_START_BYTE) {
                _rxBuffer[0] = incomingByte;
                _bytesRead = 1;
                _state = ParserState::READ_HEADER;
            }
            break;
            
        case ParserState::READ_HEADER:
            _rxBuffer[_bytesRead++] = incomingByte;
            if(_bytesRead == sizeof(PacketHeader)) {
                _state = ParserState::READ_PAYLOAD;
            }
            break;
            
        case ParserState::READ_PAYLOAD:
            _rxBuffer[_bytesRead++] = incomingByte;
            if(_bytesRead == sizeof(PacketHeader) + PAYLOADSIZE) {
                _state = ParserState::READ_CRC;
            }
            break;
            
        case ParserState::READ_CRC:
            _rxBuffer[_bytesRead++] = incomingByte;
            if(_bytesRead == sizeof(PacketData)) {
                _state = ParserState::WAIT_START;
                
                PacketData* potentialPacket = (PacketData*)_rxBuffer;
                
                if (validatePacket(potentialPacket)) {
                    // Güvenli kopyalama: Sadece geçerli paket outPacket'e yazılır
                    // Safe copying: Only valid packets are copied to outPacket
                    memcpy(&outPacket, potentialPacket, sizeof(PacketData));
                    return true;
                }
            }
            break;
    }
    return false;
}

bool StarDust::update(PacketData& outPacket) {
    if (_port == nullptr) return false; //

    // Zaman Aşımı Kontrolü
    // Timeout Control
    if (_state != ParserState::WAIT_START) {
        if (millis() - _lastRxTime > _timeoutMs) {
            _state = ParserState::WAIT_START; // Zaman dolduysa state'i sıfırla   // Reset state if timeout occurs
            _bytesRead = 0;
        }
    }

    bool packetReceived = false;
    while (_port->available() > 0) {
        uint8_t b = _port->read();
        _lastRxTime = millis();
        // Parse incoming byte and check if a complete packet has been received
        
        if (parseByte(b, outPacket)) {
            packetReceived = true;
        }
    }
    return packetReceived;
}




PacketData StarDust::sendAccept(uint8_t version, uint8_t acceptType, bool accepted) {
    AcceptPayload payload = {version, acceptType, accepted};
    PacketData packet;
    preparePacket(packet, PacketType::ACCEPT, (uint8_t*)&payload, sizeof(payload));
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}

PacketData StarDust::sendRefuse(uint8_t version, uint8_t refuseType, bool refused) {
    RefusePayload payload = {version, refuseType, refused};
    PacketData packet;
    preparePacket(packet, PacketType::REFUSE, (uint8_t*)&payload, sizeof(payload));
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}

PacketData StarDust::sendCommand(uint8_t version,double targetLat,double targetLon,float targetAlt,uint8_t actionCode) {
    CommandPayload payload = {version, targetLat, targetLon, targetAlt, actionCode};
    PacketData packet;
    preparePacket(packet, PacketType::COMMAND, (uint8_t*)&payload, sizeof(payload));
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}

PacketData StarDust::sendError(uint8_t version, uint16_t errorCode, uint16_t errorLocation, uint8_t errorSeverity) {
    ErrorPayload payload = {version, errorCode, errorLocation, errorSeverity};
    PacketData packet;
    preparePacket(packet, PacketType::ERROR, (uint8_t*)&payload, sizeof(payload));
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}

PacketData StarDust::sendEmergency(uint8_t version, uint16_t emergencyCode, uint16_t emergencyLocation, uint8_t emergencySeverity) {
    EmergencyPayload payload = {version, emergencyCode, emergencyLocation, emergencySeverity};
    PacketData packet;
    preparePacket(packet, PacketType::EMERGENCY, (uint8_t*)&payload, sizeof(payload));
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}

PacketData StarDust::sendSystemCommand(uint8_t version, uint16_t commandCode, uint16_t commandParameter, uint8_t commandSenderID, uint8_t commandAuthorityLevel) {
    SystemCommandPayload payload = {version, commandCode, commandParameter, commandSenderID, commandAuthorityLevel};
    PacketData packet;
    preparePacket(packet, PacketType::SYSTEMCOMMAND, (uint8_t*)&payload, sizeof(payload));
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}

PacketData StarDust::sendSystemInfo(uint8_t version, uint8_t infoType, bool systemOperational, float systemLoad, float systemTemperature, float systemVoltage, uint32_t uptime, float systemErrorChance, float expectedErrorChance, float systemReliability) {
    SystemInfoPayload payload = {version, infoType, systemOperational, systemLoad, systemTemperature, systemVoltage, uptime, systemErrorChance, expectedErrorChance, systemReliability};
    PacketData packet;
    preparePacket(packet, PacketType::SYSTEMINFO, (uint8_t*)&payload, sizeof(payload));
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}

PacketData StarDust::sendSystemHeartbeat(uint8_t version, bool beatStatus, float systemErrorChance, float expectedErrorChance, uint8_t missedBeats) {
    SystemHeartbeatPayload payload = {version, beatStatus, systemErrorChance, expectedErrorChance, missedBeats};
    PacketData packet;
    preparePacket(packet, PacketType::SYSTEMHEARTBEAT, (uint8_t*)&payload, sizeof(payload));
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}

PacketData StarDust::sendBetrayal(uint8_t version, bool isBetrayal, uint16_t betrayalCode, uint16_t betrayalLocation, uint8_t betrayalSeverity, bool knowsSecret, bool planExecuted, bool hasEscapePlan, bool preparedForBetrayal, float betrayalSuccessChance, float betrayalDetectionChance, bool hasAllies, uint8_t numberOfAllies, float allyLoyalty, bool knighFall) {
    BetrayalPayload payload = {version, isBetrayal, betrayalCode, betrayalLocation, betrayalSeverity, knowsSecret, planExecuted, hasEscapePlan, preparedForBetrayal, betrayalSuccessChance, betrayalDetectionChance, hasAllies, numberOfAllies, allyLoyalty, knighFall};
    PacketData packet;
    preparePacket(packet, PacketType::BETRAYAL, (uint8_t*)&payload, sizeof(payload));
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}




// ======================== GÖNDERİM VE ALIM SARMALAYICILARI ========================
// ======================== TRANSMISSION AND RECEPTION WRAPPERS ======================

PacketData StarDust::sendRequest(uint8_t version, uint8_t requestType, bool isCritical) {
    RequestPayload payload = {version, requestType, isCritical}; //
    PacketData packet;
    preparePacket(packet, PacketType::REQUEST, (uint8_t*)&payload, sizeof(payload));
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData)); //
    return packet;
}

PacketData StarDust::sendTelemetry(uint8_t version, double latitude, double longitude, double altitude, float yaw, float pitch, uint32_t timestamp, uint8_t status) {
    TelemetryPayload payload = {version, latitude, longitude, altitude, yaw, pitch, timestamp, status}; //
    PacketData packet;
    preparePacket(packet, PacketType::TELEMETRY, (uint8_t*)&payload, sizeof(payload));
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData)); //
    return packet;
}

RequestPayload StarDust::receiveRequest(const PacketData& packet) { 
    RequestPayload payload;
    memcpy(&payload, packet.payload, sizeof(RequestPayload)); //
    return payload; 
}

TelemetryPayload StarDust::receiveTelemetry(const PacketData& packet) { 
    TelemetryPayload payload;
    memcpy(&payload, packet.payload, sizeof(TelemetryPayload)); //
    return payload;
}

AcceptPayload StarDust::receiveAccept(const PacketData& packet) {
    AcceptPayload payload;
    memcpy(&payload, packet.payload, sizeof(AcceptPayload));
    return payload;
}

RefusePayload StarDust::receiveRefuse(const PacketData& packet) {
    RefusePayload payload;
    memcpy(&payload, packet.payload, sizeof(RefusePayload));
    return payload;
}

CommandPayload StarDust::receiveCommand(const PacketData& packet) {
    CommandPayload payload;
    memcpy(&payload, packet.payload, sizeof(CommandPayload));
    return payload;
}

ErrorPayload StarDust::receiveError(const PacketData& packet) {
    ErrorPayload payload;
    memcpy(&payload, packet.payload, sizeof(ErrorPayload));
    return payload;
}

EmergencyPayload StarDust::receiveEmergency(const PacketData& packet) {
    EmergencyPayload payload;
    memcpy(&payload, packet.payload, sizeof(EmergencyPayload));
    return payload;
}

SystemCommandPayload StarDust::receiveSystemCommand(const PacketData& packet) {
    SystemCommandPayload payload;
    memcpy(&payload, packet.payload, sizeof(SystemCommandPayload));
    return payload;
}

SystemInfoPayload StarDust::receiveSystemInfo(const PacketData& packet) {
    SystemInfoPayload payload;
    memcpy(&payload, packet.payload, sizeof(SystemInfoPayload));
    return payload;
}

SystemHeartbeatPayload StarDust::receiveSystemHeartbeat(const PacketData& packet) {
    SystemHeartbeatPayload payload;
    memcpy(&payload, packet.payload, sizeof(SystemHeartbeatPayload));
    return payload;
}

BetrayalPayload StarDust::receiveBetrayal(const PacketData& packet) {
    BetrayalPayload payload;
    memcpy(&payload, packet.payload, sizeof(BetrayalPayload));
    return payload;
}
