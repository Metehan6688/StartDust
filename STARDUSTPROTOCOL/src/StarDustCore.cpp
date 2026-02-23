#include "StarDustCore.h"
#include <stdint.h>
#include <string.h>



static uint8_t _virtualmyID = 0x01;
static uint8_t _virtualmasterID = 0x00;



// Referance default key for encryption, can be changed at runtime using the setStarCryptoKEY function, and is used internally for encrypting and decrypting payloads to secure the communication against eavesdropping and tampering. The actual implementation of the encryption and decryption functions will utilize this key for their operations, ensuring that the payload data is protected during transmission.
uint8_t STARDUST_KEY[16] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
};



/* CRC16 CCITT Implementation */
uint16_t calculateCRC16CCITT(const uint8_t* data, uint16_t length) {
  uint16_t crc = 0xFFFF; // Initial value
  for (uint16_t i = 0; i < length; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8; // XOR byte into the upper byte of crc
    for (int j = 0; j < 8; ++j) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021; // CCIITT polynomial value
      } else {
        crc <<= 1; // Shift left bit without XOR
      }
    }
  }
  return crc;  // Final XOR value is 0x0000, so we return crc directly
}










/* Encrypt and Decrypting functions */
void setStarCryptoKEY(const uint8_t* newKey) {
    if (newKey != nullptr) {
        // We copy the new key into the STARDUST_KEY array, ensuring that we do not exceed the defined key size of 16 bytes for AES-128 encryption. This allows us to update the encryption key at runtime if needed, providing flexibility in managing encryption keys for different sessions or devices. The new key will be used for all subsequent encryption and decryption operations until it is changed again.
        memcpy(STARDUST_KEY, newKey, 16); 
    }
}





void encryptPayload(uint8_t* payload, uint8_t size) {
    for (uint8_t i = 0; i < size; i++) {
        payload[i] ^= STARDUST_KEY[i % 16];

        payload[i] = ((payload[i] << 3) | (payload[i] >> 5)) & 0xFF; 
    }
}

void decryptPayload(uint8_t* payload, uint8_t size) {
    for (uint8_t i = 0; i < size; i++) {
        payload[i] = ((payload[i] >> 3) | (payload[i] << 5)) & 0xFF;
        payload[i] ^= STARDUST_KEY[i % 16];

    }
}











/* VALIDATE THE PACKET BY CHECKING START BYTE AND CRC */
bool validatePacket(PacketData& packet) {

    if (packet.header.start != PACKET_START_BYTE) {
        return false;
    }

    // Önce decrypt et
    decryptPayload(packet.payload, PAYLOADSIZE);

    // Sonra CRC hesapla (plaintext üstünden)
    uint16_t calculatedCRCvalue =
        calculateCRC16CCITT((uint8_t*)&packet, sizeof(PacketHeader) + PAYLOADSIZE);

    if (calculatedCRCvalue == packet.crc) {
        return true;
    } else {
        return false;
    }
}







/* PREPARE A PACKET FOR TRANSMISSION  */
void preparePacket(PacketData& packet, uint8_t source, uint8_t target,
                   PacketType type, const uint8_t* payloadData, uint8_t payloadSize)
{

    packet.header.start = PACKET_START_BYTE;
    packet.header.source = source;
    packet.header.target = target;
    packet.header.type = type;
    packet.header.size = payloadSize;

    memset(packet.payload, 0, PAYLOADSIZE);
    memcpy(packet.payload, payloadData, payloadSize);

    // CRC plaintext üstünden
    packet.crc =
        calculateCRC16CCITT((uint8_t*)&packet, sizeof(PacketHeader) + PAYLOADSIZE);

    // sonra encrypt
    encryptPayload(packet.payload, PAYLOADSIZE);
}





void beginStarDust(uint8_t myID, uint8_t masterID) {
    _virtualmyID = myID;
    _virtualmasterID = masterID;
}




/* ========================= */
/* Helper Function Creations */
/* ========================= */

PacketData sendRequest(uint8_t version, uint8_t requestType, bool isCritical) {
    RequestPayload payload;
    payload.version = version;
    payload.requestType = requestType;
    payload.isCritical = isCritical;

    PacketData packet;
    preparePacket(packet, _virtualmyID, _virtualmasterID, PacketType::REQUEST, (uint8_t*)&payload, sizeof(payload));
    return packet;
}

PacketData sendAccept(uint8_t version, uint8_t acceptType, bool accepted) {
    AcceptPayload payload;
    payload.version = version;
    payload.acceptType = acceptType;
    payload.accepted = accepted;

    PacketData packet;
    preparePacket(packet, _virtualmyID, _virtualmasterID, PacketType::ACCEPT, (uint8_t*)&payload, sizeof(payload));
    return packet;
}

PacketData sendRefuse(uint8_t version, uint8_t refuseType, bool refused) {
    RefusePayload payload;
    payload.version = version;
    payload.refuseType = refuseType;
    payload.refused = refused;

    PacketData packet;
    preparePacket(packet, _virtualmyID, _virtualmasterID, PacketType::REFUSE, (uint8_t*)&payload, sizeof(payload));
    return packet;
}

PacketData sendTelemetry(uint8_t version, double latitude, double longitude, double altitude, float yaw, float pitch, uint32_t timestamp, uint8_t status) {
    TelemetryPayload payload;
    payload.version = version;
    payload.latitude = latitude;
    payload.longitude = longitude;
    payload.altitude = altitude;
    payload.yaw = yaw;
    payload.pitch = pitch;
    payload.timestamp = timestamp;
    payload.status = status;

    PacketData packet;
    preparePacket(packet, _virtualmyID, _virtualmasterID, PacketType::TELEMETRY, (uint8_t*)&payload, sizeof(payload));
    return packet;
}

PacketData sendCommand(uint8_t version, float targetYaw, float targetPitch, uint8_t actionCode) {
    CommandPayload payload;
    payload.version = version;
    payload.targetYaw = targetYaw;
    payload.targetPitch = targetPitch;
    payload.actionCode = actionCode;

    PacketData packet;
    preparePacket(packet, _virtualmyID, _virtualmasterID, PacketType::COMMAND, (uint8_t*)&payload, sizeof(payload));
    return packet;
}

PacketData sendError(uint8_t version, uint16_t errorCode, uint16_t errorLocation, uint8_t errorSeverity) {
    ErrorPayload payload;
    payload.version = version;
    payload.errorCode = errorCode;
    payload.errorLocation = errorLocation;
    payload.errorSeverity = errorSeverity;

    PacketData packet;
    preparePacket(packet, _virtualmyID, _virtualmasterID, PacketType::ERROR, (uint8_t*)&payload, sizeof(payload));
    return packet;
}

PacketData sendEmergency(uint8_t version, uint16_t emergencyCode, uint16_t emergencyLocation, uint8_t emergencySeverity) {
    EmergencyPayload payload;
    payload.version = version;
    payload.emergencyCode = emergencyCode;
    payload.emergencyLocation = emergencyLocation;
    payload.emergencySeverity = emergencySeverity;

    PacketData packet;
    preparePacket(packet, _virtualmyID, _virtualmasterID, PacketType::EMERGENCY, (uint8_t*)&payload, sizeof(payload));
    return packet;
}

PacketData sendSystemCommand(uint8_t version, uint16_t commandCode, uint16_t commandParameter, uint8_t commandSenderID, uint8_t commandAuthorityLevel) {
    SystemCommandPayload payload;
    payload.version = version;
    payload.commandCode = commandCode;
    payload.commandParameter = commandParameter;
    payload.commandSenderID = commandSenderID;
    payload.commandAuthorityLevel = commandAuthorityLevel;

    PacketData packet;
    preparePacket(packet, _virtualmyID, _virtualmasterID, PacketType::SYSTEMCOMMAND, (uint8_t*)&payload, sizeof(payload));
    return packet;
}

PacketData sendSystemInfo(uint8_t version, uint8_t infoType, bool systemOperational, float systemLoad, float systemTemperature, float systemVoltage, uint32_t uptime, float systemErrorChance, float expectedErrorChance, float systemReliability) {
    SystemInfoPayload payload;
    payload.version = version;
    payload.infoType = infoType;
    payload.systemOperational = systemOperational;
    payload.systemLoad = systemLoad;
    payload.systemTemperature = systemTemperature;
    payload.systemVoltage = systemVoltage;
    payload.uptime = uptime;
    payload.systemErrorChance = systemErrorChance;
    payload.expectedErrorChance = expectedErrorChance;
    payload.systemReliability = systemReliability;

    PacketData packet;
    preparePacket(packet, _virtualmyID, _virtualmasterID, PacketType::SYSTEMINFO, (uint8_t*)&payload, sizeof(payload));
    return packet;
}

PacketData sendSystemHeartbeat(uint8_t version, bool beatStatus, float systemErrorChance, float expectedErrorChance, uint8_t missedBeats) {
    SystemHeartbeatPayload payload;
    payload.version = version;
    payload.beatStatus = beatStatus;
    payload.systemErrorChance = systemErrorChance;
    payload.expectedErrorChance = expectedErrorChance;
    payload.missedBeats = missedBeats;

    PacketData packet;
    preparePacket(packet, _virtualmyID, _virtualmasterID, PacketType::SYSTEMHEARTBEAT, (uint8_t*)&payload, sizeof(payload));
    return packet;
}

PacketData sendBetrayal(uint8_t version, bool isBetrayal, uint16_t betrayalCode, uint16_t betrayalLocation, uint8_t betrayalSeverity, bool knowsSecret, bool planExecuted, bool hasEscapePlan, bool preparedForBetrayal, float betrayalSuccessChance, float betrayalDetectionChance, bool hasAllies, uint8_t numberOfAllies, float allyLoyalty, bool knighFall) {
    BetrayalPayload payload;
    payload.version = version;
    payload.isBetrayal = isBetrayal;
    payload.betrayalCode = betrayalCode;
    payload.betrayalLocation = betrayalLocation;
    payload.betrayalSeverity = betrayalSeverity;
    payload.knowsSecret = knowsSecret;
    payload.planExecuted = planExecuted;
    payload.hasEscapePlan = hasEscapePlan;
    payload.preparedForBetrayal = preparedForBetrayal;
    payload.betrayalSuccessChance = betrayalSuccessChance;
    payload.betrayalDetectionChance = betrayalDetectionChance;
    payload.hasAllies = hasAllies;
    payload.numberOfAllies = numberOfAllies;
    payload.allyLoyalty = allyLoyalty;
    payload.knighFall = knighFall;

    PacketData packet;
    preparePacket(packet, _virtualmyID, _virtualmasterID, PacketType::BETRAYAL, (uint8_t*)&payload, sizeof(payload));
    return packet;
}





















/* ======================================================= */
/* ============ FUNCTIONS FOR PACKET EXTRACTION ===========*/
/* ======================================================= */

RequestPayload receiveRequest(const PacketData& packet) {
    RequestPayload payload;
    memcpy(&payload, packet.payload, sizeof(RequestPayload));
    return payload;
}

AcceptPayload receiveAccept(const PacketData& packet) {
    AcceptPayload payload;
    memcpy(&payload, packet.payload, sizeof(AcceptPayload));
    return payload;
}

RefusePayload receiveRefuse(const PacketData& packet) {
    RefusePayload payload;
    memcpy(&payload, packet.payload, sizeof(RefusePayload));
    return payload;
}

TelemetryPayload receiveTelemetry(const PacketData& packet) {
    TelemetryPayload payload;
    // packet.payload dizisindeki verileri, TelemetryPayload yapısına kopyalıyoruz
    memcpy(&payload, packet.payload, sizeof(TelemetryPayload));
    return payload;
}

CommandPayload receiveCommand(const PacketData& packet) {
    CommandPayload payload;
    memcpy(&payload, packet.payload, sizeof(CommandPayload));
    return payload;
}

ErrorPayload receiveError(const PacketData& packet) {
    ErrorPayload payload;
    memcpy(&payload, packet.payload, sizeof(ErrorPayload));
    return payload;
}

EmergencyPayload receiveEmergency(const PacketData& packet) {
    EmergencyPayload payload;
    memcpy(&payload, packet.payload, sizeof(EmergencyPayload));
    return payload;
}

SystemCommandPayload receiveSystemCommand(const PacketData& packet) {
    SystemCommandPayload payload;
    memcpy(&payload, packet.payload, sizeof(SystemCommandPayload));
    return payload;
}

SystemInfoPayload receiveSystemInfo(const PacketData& packet) {
    SystemInfoPayload payload;
    memcpy(&payload, packet.payload, sizeof(SystemInfoPayload));
    return payload;
}

SystemHeartbeatPayload receiveSystemHeartbeat(const PacketData& packet) {
    SystemHeartbeatPayload payload;
    memcpy(&payload, packet.payload, sizeof(SystemHeartbeatPayload));
    return payload;
}

BetrayalPayload receiveBetrayal(const PacketData& packet) {
    BetrayalPayload payload;
    memcpy(&payload, packet.payload, sizeof(BetrayalPayload));
    return payload;
}
















/* ==================================================================================================== */
/* ==================================================================================================== */
/* ========================================= NEW PARSER TOOL ========================================== */
/* ==================================================================================================== */
/* ==================================================================================================== */

static ParserState _state = ParserState::WAIT_START;
static uint16_t _bytesRead = 0;
static uint8_t* _buffer = nullptr;



bool parsePacket(uint8_t incomingByte, PacketData& outPacket) {
  switch (_state) {
    case ParserState::WAIT_START:
      if(incomingByte == PACKET_START_BYTE) { // Start byte control 
        _buffer = (uint8_t*)&outPacket; // Connect the buffer directly to the output packet structure, so we can write directly into it as we read bytes
        memset(_buffer, 0, sizeof(PacketData)); // Reset the buffer to zero before starting to fill it with new packet data
        _buffer[0] = incomingByte; // Write the start byte into the buffer as the first byte of the packet
        _bytesRead = 1; 
        _state = ParserState::READ_HEADER; 
      }
      break;
    
    case ParserState::READ_HEADER:
      _buffer[_bytesRead++] = incomingByte; 
      if(_bytesRead == sizeof(PacketHeader)) {
        _state = ParserState::READ_PAYLOAD; 
      }
      break;
      
    case ParserState::READ_PAYLOAD:
      _buffer[_bytesRead++] = incomingByte; 
      if(_bytesRead == sizeof(PacketHeader) + PAYLOADSIZE) {
        _state = ParserState::READ_CRC; 
      }
      break;
      
    case ParserState::READ_CRC:
      _buffer[_bytesRead++] = incomingByte;
      if(_bytesRead == sizeof(PacketData)) {
        _state = ParserState::WAIT_START; // Reset state for next packet
        

        // Validate the packet and check if it's intended for us (or is a broadcast)
        return validatePacket(outPacket); 
      }
      break;
  }

  return false; 
}