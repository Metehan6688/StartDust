#ifndef STARDUSTCORE_H
#define STARDUSTCORE_H

#include <stdint.h>





/* ABSOLUTE PROTOCOL IDENTITIFICATION VARIABLES */
constexpr uint8_t PACKET_START_BYTE = 0xAA;
constexpr uint8_t PAYLOADSIZE = 64;


/* Crypto variables */
extern uint8_t STARDUST_KEY[16];



/* Initilazing Star Dust System */
void beginStarDust(uint8_t myID, uint8_t masterID);





/* PACKET TYPE CONFIGS FOR PROTOCOL */
enum PacketType : uint8_t {
  REQUEST = 1,
  ACCEPT = 2,
  REFUSE = 3,

  TELEMETRY = 4,
  COMMAND = 5,
  ERROR = 101,

  EMERGENCY = 102,

  SYSTEMCOMMAND = 201,
  SYSTEMINFO = 202,
  SYSTEMHEARTBEAT = 203,

  BETRAYAL = 66
};








/* ======================================================= */
/* =============== DATA PACKETS STRUCTURES ================*/
/* ======================================================= */
struct __attribute__((packed)) RequestPayload {
  uint8_t version;
  uint8_t requestType;
  bool isCritical;
};



struct __attribute__((packed)) AcceptPayload {
  uint8_t version;
  uint8_t acceptType;
  bool accepted;
};  



struct __attribute__((packed)) RefusePayload {
  uint8_t version;
  uint8_t refuseType;
  bool refused;
};



struct __attribute__((packed)) TelemetryPayload {
    uint8_t version;

    double latitude;
    double longitude;
    double altitude;

    float yaw;
    float pitch;

    uint32_t timestamp;

    uint8_t status;
};



struct  __attribute__((packed)) CommandPayload {
    uint8_t version;

    float targetYaw;
    float targetPitch;
    
    uint8_t actionCode;
};



struct __attribute__((packed)) ErrorPayload {
  uint8_t version;
  uint16_t errorCode;
  uint16_t errorLocation;
  uint8_t errorSeverity;
};



struct __attribute__((packed)) EmergencyPayload {
  uint8_t version;
  uint16_t emergencyCode;
  uint16_t emergencyLocation;
  uint8_t emergencySeverity;
};



struct __attribute__((packed)) SystemCommandPayload {
  uint8_t version;
  uint16_t commandCode;
  uint16_t commandParameter;
  uint8_t commandSenderID;
  uint8_t commandAuthorityLevel;
};



struct __attribute__((packed)) SystemInfoPayload {
  uint8_t version;
  uint8_t infoType;

  bool systemOperational;
  float systemLoad;
  float systemTemperature;
  float systemVoltage;

  uint32_t uptime;

  float systemErrorChance;
  float expectedErrorChance;
  float systemReliability;
};



struct __attribute__((packed)) SystemHeartbeatPayload{
  uint8_t version;
  bool beatStatus;
  float systemErrorChance;
  float expectedErrorChance;
  uint8_t missedBeats;
};



struct __attribute__((packed)) BetrayalPayload{
  uint8_t version;
  bool isBetrayal;
  uint16_t betrayalCode;
  uint16_t betrayalLocation;
  uint8_t betrayalSeverity;

  bool knowsSecret;
  bool planExecuted;
  bool hasEscapePlan;
  bool preparedForBetrayal;
  float betrayalSuccessChance;
  float betrayalDetectionChance;

  bool hasAllies;
  uint8_t numberOfAllies;
  float allyLoyalty;

  bool knighFall;
};











/* ======================================================= */
/* =============== COMMON PACKET STRUCTURES ===============*/
/* ======================================================= */
struct __attribute__((packed)) PacketHeader {
  uint8_t start;
  uint8_t source;
  uint8_t target;
  PacketType type;
  uint8_t size;
};


struct __attribute__((packed)) PacketData {
  PacketHeader header;
  uint8_t payload[PAYLOADSIZE];
  uint16_t crc;
};





/* ======================================================= */
/* ============ FUNCTIONS FOR PACKET HANDLING =============*/
/* ======================================================= */
uint16_t calculateCRC16CCITT(const uint8_t *data, uint16_t length);
bool validatePacket(PacketData& packet);
void preparePacket(PacketData& packet, uint8_t source, uint8_t target, PacketType type, const uint8_t* payloadData, uint8_t payloadSize);
// DONT USE THEM OUTSIDE OF THE CORE, THEY ARE INTERNAL TO THE PROTOCOL IMPLEMENTATION
// They are exposed here for testing purposes but should not be used directly in user code




/* ======================================================= */
/* ============ FUNCTIONS FOR PACKET CREATION =============*/
/* ======================================================= */
PacketData sendRequest(uint8_t version, uint8_t requestType, bool isCritical);
PacketData sendAccept(uint8_t version, uint8_t acceptType, bool accepted);
PacketData sendRefuse(uint8_t version, uint8_t refuseType, bool refused);
PacketData sendTelemetry(uint8_t version, double latitude, double longitude, double altitude, float yaw, float pitch, uint32_t timestamp, uint8_t status);
PacketData sendCommand(uint8_t version, float targetYaw, float targetPitch, uint8_t actionCode);
PacketData sendError(uint8_t version, uint16_t errorCode, uint16_t errorLocation, uint8_t errorSeverity);
PacketData sendEmergency(uint8_t version, uint16_t emergencyCode, uint16_t emergencyLocation, uint8_t emergencySeverity);
PacketData sendSystemCommand(uint8_t version, uint16_t commandCode, uint16_t commandParameter, uint8_t commandSenderID, uint8_t commandAuthorityLevel);
PacketData sendSystemInfo(uint8_t version, uint8_t infoType, bool systemOperational, float systemLoad, float systemTemperature, float systemVoltage, uint32_t uptime, float systemErrorChance, float expectedErrorChance, float systemReliability);
PacketData sendSystemHeartbeat(uint8_t version, bool beatStatus, float systemErrorChance, float expectedErrorChance, uint8_t missedBeats);
PacketData sendBetrayal(uint8_t version, bool isBetrayal, uint16_t betrayalCode, uint16_t betrayalLocation, uint8_t betrayalSeverity, bool knowsSecret, bool planExecuted, bool hasEscapePlan, bool preparedForBetrayal, float betrayalSuccessChance, float betrayalDetectionChance, bool hasAllies, uint8_t numberOfAllies, float allyLoyalty, bool knighFall);
// USE THEM TO CREATE PACKETS TO SEND, THEY HANDLE THE CREATION OF THE PAYLOAD AND THE PROPER PACKET STRUCTURE FOR YOU, JUST PROVIDE THE NECESSARY DATA AND IT WILL RETURN A READY TO SEND PACKET DATA STRUCTURE


/* ======================================================= */
/* ============ FUNCTIONS FOR PACKET EXTRACTION ===========*/
/* ======================================================= */
RequestPayload receiveRequest(const PacketData& packet);
AcceptPayload receiveAccept(const PacketData& packet);
RefusePayload receiveRefuse(const PacketData& packet);
TelemetryPayload receiveTelemetry(const PacketData& packet);
CommandPayload receiveCommand(const PacketData& packet);
ErrorPayload receiveError(const PacketData& packet);
EmergencyPayload receiveEmergency(const PacketData& packet);
SystemCommandPayload receiveSystemCommand(const PacketData& packet);
SystemInfoPayload receiveSystemInfo(const PacketData& packet);
SystemHeartbeatPayload receiveSystemHeartbeat(const PacketData& packet);
BetrayalPayload receiveBetrayal(const PacketData& packet);
// USE THEM TO EXTRACT THE PAYLOAD DATA FROM THE RECEIVED PACKET, THEY HANDLE THE DECRYPTION AND PROPER CASTING OF THE PAYLOAD FOR YOU, JUST PROVIDE THE RECEIVED PACKET AND IT WILL RETURN THE EXTRACTED PAYLOAD STRUCTURE WITH THE DATA FILLED IN









// Crypto functions for encrypting and decrypting payloads, using AES-128 encryption with the predefined key
// These functions will be used internally to secure the payload data before sending and after receiving, ensuring that the communication is protected against eavesdropping and tampering. The actual implementation of these functions will depend on the AES library you choose to use, and they will utilize the STARDUST_KEY for encryption and decryption operations.
// The setStarCryptoKEY function allows you to update the encryption key at runtime if needed, providing flexibility in managing encryption keys for different sessions or devices.
void setStarCryptoKEY(const uint8_t* newKey);

void encryptPayload(uint8_t* payload, uint8_t size);
void decryptPayload(uint8_t* payload, uint8_t size);
// These functions will be used internally to secure the payload data before sending and after receiving, ensuring that the communication is protected against eavesdropping and tampering. The actual implementation of these functions will depend on the AES library you choose to use, and they will utilize the STARDUST_KEY for encryption and decryption operations.







/* ==================================================================================================== */
/* ==================================================================================================== */
/* ========================================= NEW PARSER TOOL ========================================== */
/* ==================================================================================================== */
/* ==================================================================================================== */

enum class ParserState {
    WAIT_START,
    READ_HEADER,
    READ_PAYLOAD,
    READ_CRC
};


// Parser function prototype
bool parsePacket(uint8_t incomingByte, PacketData& outPacket);













#endif
