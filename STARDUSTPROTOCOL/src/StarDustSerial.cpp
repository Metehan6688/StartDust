#include "StarDustSerial.h"
#include <stdint.h>
#include <string.h>



uint8_t _myID = 0x01;
uint8_t _masterID = 0x00;
static Stream* _port = nullptr;





// Serial Communication Functions for Star Dust Protocol
void starDustSerialBegin(uint8_t myID, uint8_t masterID, Stream& port) {
    _myID = myID;
    _masterID = masterID;
    _port = &port;
}


// Set a new crypto key for encrypting and decrypting payloads, allowing dynamic key management for different sessions or devices
void STARsetCryptoKey(const uint8_t* newKey) {
    setStarCryptoKEY(newKey); // Call the core function to set the new crypto key for encrypting and decrypting payloads, allowing dynamic key management for different sessions or devices
}


// Update function to be called in the main loop to process incoming data and update the protocol state



/*
bool STARupdate(PacketData& outPacket) {
    if (_port == nullptr || !_port->available()) return false;

    static uint8_t buffer[sizeof(PacketData)];
    static uint8_t index = 0;

    while (_port->available()) {
        uint8_t b = _port->read();

        // Start byte control
        if (index == 0 && b != PACKET_START_BYTE) continue;

        buffer[index++] = b;

        // Packet complete control
        if (index >= sizeof(PacketData)) {
            index = 0; // Reset index for next packet
            
            // Create a pointer to the buffer as a PacketData structure for easier access
            PacketData* potentialPacket = (PacketData*)buffer;

            // Validate the packet and check if it's intended for us (or is a broadcast)
            if (validatePacket(*potentialPacket)) {
                // Check if the packet is addressed to us or is a broadcast (0xFF)
                if (potentialPacket->header.target == _myID || potentialPacket->header.target == 0xFF) {
                    memcpy(&outPacket, potentialPacket, sizeof(PacketData));
                    return true; // Valid packet received and copied to output parameter
                }
            }
        }
    }
    return false;
}
*/







bool STARupdate(PacketData& outPacket) {
    // Port ayarlanmamışsa veya okunacak veri yoksa çık
    if (_port == nullptr || !_port->available()) return false;

    // Porttaki tüm baytları tek tek oku ve parsePacket'e gönder
    while (_port->available() > 0) {
        uint8_t b = _port->read();
        
        // StarDustCore içindeki o meşhur durum makinesini çağırıyoruz
        if (parsePacket(b, outPacket)) {
            // parsePacket TRUE döndüğünde:
            // 1. Paket tamdır.
            // 2. Şifre çözülmüştür (decryptPayload).
            // 3. CRC kontrolü geçilmiştir.
            return true; 
        }
    }
    
    return false; // Henüz tam bir paket oluşmadı
}










/* ======================================================= */
/* ========== FUNCTIONS FOR PACKET SEND VIA UART ==========*/
/* ======================================================= */

PacketData STARsendRequest(uint8_t version, uint8_t requestType, bool isCritical) {
    RequestPayload payload = {version, requestType, isCritical};
    PacketData packet;
    preparePacket(packet, _myID, _masterID, PacketType::REQUEST, (uint8_t*)&payload, sizeof(payload));
    
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet; // Boş dönme, paketi dön!
}

PacketData STARsendAccept(uint8_t version, uint8_t acceptType, bool accepted) {
    AcceptPayload payload = {version, acceptType, accepted};
    PacketData packet;
    preparePacket(packet, _myID, _masterID, PacketType::ACCEPT, (uint8_t*)&payload, sizeof(payload));
    
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}

PacketData STARsendRefuse(uint8_t version, uint8_t refuseType, bool refused) {
    RefusePayload payload = {version, refuseType, refused};
    PacketData packet;
    preparePacket(packet, _myID, _masterID, PacketType::REFUSE, (uint8_t*)&payload, sizeof(payload));
    
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}

PacketData STARsendTelemetry(uint8_t version, double latitude, double longitude, double altitude, float yaw, float pitch, uint32_t timestamp, uint8_t status) {
    TelemetryPayload payload = {version, latitude, longitude, altitude, yaw, pitch, timestamp, status};
    PacketData packet;
    preparePacket(packet, _myID, _masterID, PacketType::TELEMETRY, (uint8_t*)&payload, sizeof(payload));
    
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}

PacketData STARsendCommand(uint8_t version, float targetYaw, float targetPitch, uint8_t actionCode) {
    CommandPayload payload = {version, targetYaw, targetPitch, actionCode};
    PacketData packet;
    preparePacket(packet, _myID, _masterID, PacketType::COMMAND, (uint8_t*)&payload, sizeof(payload));
    
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}

PacketData STARsendError(uint8_t version, uint16_t errorCode, uint16_t errorLocation, uint8_t errorSeverity) {
    ErrorPayload payload = {version, errorCode, errorLocation, errorSeverity};
    PacketData packet;
    preparePacket(packet, _myID, _masterID, PacketType::ERROR, (uint8_t*)&payload, sizeof(payload));
    
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}

PacketData STARsendEmergency(uint8_t version, uint16_t emergencyCode, uint16_t emergencyLocation, uint8_t emergencySeverity) {
    EmergencyPayload payload = {version, emergencyCode, emergencyLocation, emergencySeverity};
    PacketData packet;
    preparePacket(packet, _myID, _masterID, PacketType::EMERGENCY, (uint8_t*)&payload, sizeof(payload));
    
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}

PacketData STARsendSystemCommand(uint8_t version, uint16_t commandCode, uint16_t commandParameter, uint8_t commandSenderID, uint8_t commandAuthorityLevel) {
    SystemCommandPayload payload = {version, commandCode, commandParameter, commandSenderID, commandAuthorityLevel};
    PacketData packet;
    preparePacket(packet, _myID, _masterID, PacketType::SYSTEMCOMMAND, (uint8_t*)&payload, sizeof(payload));
    
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}

PacketData STARsendSystemInfo(uint8_t version, uint8_t infoType, bool systemOperational, float systemLoad, float systemTemperature, float systemVoltage, uint32_t uptime, float systemErrorChance, float expectedErrorChance, float systemReliability) {
    SystemInfoPayload payload = {version, infoType, systemOperational, systemLoad, systemTemperature, systemVoltage, uptime, systemErrorChance, expectedErrorChance, systemReliability};
    PacketData packet;
    preparePacket(packet, _myID, _masterID, PacketType::SYSTEMINFO, (uint8_t*)&payload, sizeof(payload));
    
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}

PacketData STARsendSystemHeartbeat(uint8_t version, bool beatStatus, float systemErrorChance, float expectedErrorChance, uint8_t missedBeats) {
    SystemHeartbeatPayload payload = {version, beatStatus, systemErrorChance, expectedErrorChance, missedBeats};
    PacketData packet;
    preparePacket(packet, _myID, _masterID, PacketType::SYSTEMHEARTBEAT, (uint8_t*)&payload, sizeof(payload));
    
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}

PacketData STARsendBetrayal(uint8_t version, bool isBetrayal, uint16_t betrayalCode, uint16_t betrayalLocation, uint8_t betrayalSeverity, bool knowsSecret, bool planExecuted, bool hasEscapePlan, bool preparedForBetrayal, float betrayalSuccessChance, float betrayalDetectionChance, bool hasAllies, uint8_t numberOfAllies, float allyLoyalty, bool knighFall) {
    BetrayalPayload payload = {version, isBetrayal, betrayalCode, betrayalLocation, betrayalSeverity, knowsSecret, planExecuted, hasEscapePlan, preparedForBetrayal, betrayalSuccessChance, betrayalDetectionChance, hasAllies, numberOfAllies, allyLoyalty, knighFall};
    PacketData packet;
    preparePacket(packet, _myID, _masterID, PacketType::BETRAYAL, (uint8_t*)&payload, sizeof(payload));
    
    if(_port != nullptr) _port->write((uint8_t*)&packet, sizeof(PacketData));
    return packet;
}









/* ======================================================= */
/* ========== FUNCTIONS FOR PACKET RECEIVE VIA UART =======*/
/* ======================================================= */

RequestPayload STARreceiveRequest(const PacketData& packet) { 
    return receiveRequest(packet); 
}


AcceptPayload STARreceiveAccept(const PacketData& packet) { 
    return receiveAccept(packet); 
}


RefusePayload STARreceiveRefuse(const PacketData& packet) { 
    return receiveRefuse(packet); 
}


TelemetryPayload STARreceiveTelemetry(const PacketData& packet) { 
    return receiveTelemetry(packet);
}


CommandPayload STARreceiveCommand(const PacketData& packet) { 
    return receiveCommand(packet); 
}


ErrorPayload STARreceiveError(const PacketData& packet) { 
    return receiveError(packet); 
}


EmergencyPayload STARreceiveEmergency(const PacketData& packet) {
    return receiveEmergency(packet); 
}


SystemCommandPayload STARreceiveSystemCommand(const PacketData& packet) { 
    return receiveSystemCommand(packet); 
}


SystemInfoPayload STARreceiveSystemInfo(const PacketData& packet) {
    return receiveSystemInfo(packet); 
}


SystemHeartbeatPayload STARreceiveSystemHeartbeat(const PacketData& packet) { 
    return receiveSystemHeartbeat(packet); 
}


BetrayalPayload STARreceiveBetrayal(const PacketData& packet) { 
    return receiveBetrayal(packet); 
}


