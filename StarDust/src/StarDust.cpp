#include "StarDust.h"

// Başlangıç için varsayılan bir şifreleme anahtarı
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
    _timeoutMs = 100; // Varsayılan timeout 100 milisaniye
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
        memcpy(_cryptoKey, newKey, 16); // Dinamik anahtar değişimi
    }
}

void StarDust::setTimeout(uint32_t timeoutMs) {
    _timeoutMs = timeoutMs;
}

void StarDust::setTargetID(uint8_t targetID) {
    _targetID = targetID;
}

// ======================== GÜVENLİK VE CRC ========================

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

void StarDust::preparePacket(PacketData& packet, PacketType type, const uint8_t* payloadData, uint8_t payloadSize) {
    packet.header.start = PACKET_START_BYTE; //
    packet.header.source = _myID;
    packet.header.target = _targetID;
    packet.header.type = type;
    packet.header.size = payloadSize;

    memset(packet.payload, 0, PAYLOADSIZE);
    memcpy(packet.payload, payloadData, payloadSize);

    // CRC hesaplama Düz metin (plaintext) üzerinden yapılır
    packet.crc = calculateCRC16CCITT((uint8_t*)&packet, sizeof(PacketHeader) + PAYLOADSIZE);

    // Ardından payload şifrelenir
    encryptPayload(packet.payload, PAYLOADSIZE);
}

bool StarDust::validatePacket(PacketData* packet) {
    if (packet->header.start != PACKET_START_BYTE) return false;

    // Şifreyi çöz
    decryptPayload(packet->payload, PAYLOADSIZE);

    // Çözülmüş halinin CRC'sini hesapla
    uint16_t calcCRC = calculateCRC16CCITT((uint8_t*)packet, sizeof(PacketHeader) + PAYLOADSIZE);

    // CRC eşleşiyorsa ve paket bize veya genel yayına (broadcast) gelmişse geçerlidir
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
                    // Güvenli kopyalama: Sadece geçerli paket outPacket'e yazılır!
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

    // Zaman Aşımı (Timeout) Kontrolü - Sistemi kilitlemez!
    if (_state != ParserState::WAIT_START) {
        if (millis() - _lastRxTime > _timeoutMs) {
            _state = ParserState::WAIT_START; // Zaman dolduysa state'i sıfırla
            _bytesRead = 0;
        }
    }

    bool packetReceived = false;
    while (_port->available() > 0) { // Sadece veri varsa oku (Non-blocking)
        uint8_t b = _port->read(); //
        _lastRxTime = millis();    // Zamanlayıcıyı güncelle
        
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

PacketData StarDust::sendCommand(uint8_t version, float targetYaw, float targetPitch, uint8_t actionCode) {
    CommandPayload payload = {version, targetYaw, targetPitch, actionCode};
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

PacketData StarDust::sendRequest(uint8_t version, uint8_t requestType, bool isCritical) {
    RequestPayload payload = {version, requestType, isCritical}; //
    PacketData packet;
    preparePacket(packet, PacketType::REQUEST, (uint8_t*)&payload, sizeof(payload));
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData)); //
    return packet;
}

// Diğer tüm sendXXXX fonksiyonlarını benzer şekilde yapılandırabiliriz. 
// Örnek olarak Telemetry'yi de ekliyorum (Diğerlerini de aynen bu düzende sınıf metodu olarak kopyalayabilirsin):
PacketData StarDust::sendTelemetry(uint8_t version, double latitude, double longitude, double altitude, float yaw, float pitch, uint32_t timestamp, uint8_t status) {
    TelemetryPayload payload = {version, latitude, longitude, altitude, yaw, pitch, timestamp, status}; //
    PacketData packet;
    preparePacket(packet, PacketType::TELEMETRY, (uint8_t*)&payload, sizeof(payload));
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData)); //
    return packet;
}

// ... (Buraya diğer sendXXXX fonksiyonlarının gövdeleri yukarıdaki şablona göre gelecek) ...

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
