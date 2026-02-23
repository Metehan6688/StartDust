#ifndef STARDUSTSERIAL_H
#define STARDUSTSERIAL_H

#include "StarDustCore.h"
#include <stdint.h>










/* ================================================================================================= */
#ifdef ARDUINO
    #include <Arduino.h>
#else
    #include <cstddef>
    // Make a dummy Stream class for testing on PC, since Serial is not available
    class Stream {
    public:
        virtual size_t write(const uint8_t* buffer, size_t size) { return 0; }
        // You can add more dummy methods if needed for testing, like read() or available()
        virtual int available() { return 0; }
        virtual int read() { return -1; }
    };
    
    // For testing purposes, we can create a global instance of the dummy Stream class
    // In real usage, users will pass their actual Stream (like Serial) to the starDustSerialBegin function

    /* Stream Serial; */
    
    // Uncomment this line if you want to use Serial in testing on PC
#endif
/* ================================================================================================== */







// Serial Communication Functions for Star Dust Protocol
void starDustSerialBegin(uint8_t myID, uint8_t masterID, Stream& port);

// Set a new crypto key for encrypting and decrypting payloads, allowing dynamic key management for different sessions or devices
void STARsetCryptoKey(const uint8_t* newKey);



// This function should be called in the main loop to process incoming data and update the protocol state, it reads from the serial port, assembles packets, validates them, and if a valid packet is received that is addressed to us (or is a broadcast), it copies the packet data into the provided output parameter and returns true. If no valid packet is received, it returns false. This allows the user to easily integrate the packet receiving functionality into their main loop and handle incoming packets as they arrive.
// Update function to be called in the main loop to process incoming data and update the protocol state
bool STARupdate(PacketData& outPacket);
// Like a chief for receiving packets :D
// Use before STARreceiveXXXXXXXX funcs
// Okay



/* ======================================================= */
/* ========== FUNCTIONS FOR PACKET SEND VIA UART ==========*/
/* ======================================================= */
PacketData STARsendRequest(uint8_t version, uint8_t requestType, bool isCritical);
PacketData STARsendAccept(uint8_t version, uint8_t acceptType, bool accepted);
PacketData STARsendRefuse(uint8_t version, uint8_t refuseType, bool refused);
PacketData STARsendTelemetry(uint8_t version, double latitude, double longitude, double altitude, float yaw, float pitch, uint32_t timestamp, uint8_t status);
PacketData STARsendCommand(uint8_t version, float targetYaw, float targetPitch, uint8_t actionCode);
PacketData STARsendError(uint8_t version, uint16_t errorCode, uint16_t errorLocation, uint8_t errorSeverity);
PacketData STARsendEmergency(uint8_t version, uint16_t emergencyCode, uint16_t emergencyLocation, uint8_t emergencySeverity);
PacketData STARsendSystemCommand(uint8_t version, uint16_t commandCode, uint16_t commandParameter, uint8_t commandSenderID, uint8_t commandAuthorityLevel);
PacketData STARsendSystemInfo(uint8_t version, uint8_t infoType, bool systemOperational, float systemLoad, float systemTemperature, float systemVoltage, uint32_t uptime, float systemErrorChance, float expectedErrorChance, float systemReliability);
PacketData STARsendSystemHeartbeat(uint8_t version, bool beatStatus, float systemErrorChance, float expectedErrorChance, uint8_t missedBeats);
PacketData STARsendBetrayal(uint8_t version, bool isBetrayal, uint16_t betrayalCode, uint16_t betrayalLocation, uint8_t betrayalSeverity, bool knowsSecret, bool planExecuted, bool hasEscapePlan, bool preparedForBetrayal, float betrayalSuccessChance, float betrayalDetectionChance, bool hasAllies, uint8_t numberOfAllies, float allyLoyalty, bool knighFall);
// USE THEM TO CREATE PACKETS TO SEND, THEY HANDLE THE CREATION OF THE PAYLOAD AND THE PROPER PACKET STRUCTURE FOR YOU, JUST PROVIDE THE NECESSARY DATA AND IT WILL RETURN A READY TO SEND PACKET DATA STRUCTURE




/* ======================================================= */
/* ========== FUNCTIONS FOR PACKET RECEIVE VIA UART =======*/
/* ======================================================= */
RequestPayload STARreceiveRequest(const PacketData& packet);
AcceptPayload STARreceiveAccept(const PacketData& packet);
RefusePayload STARreceiveRefuse(const PacketData& packet);
TelemetryPayload STARreceiveTelemetry(const PacketData& packet);
CommandPayload STARreceiveCommand(const PacketData& packet);
ErrorPayload STARreceiveError(const PacketData& packet);
EmergencyPayload STARreceiveEmergency(const PacketData& packet);
SystemCommandPayload STARreceiveSystemCommand(const PacketData& packet);
SystemInfoPayload STARreceiveSystemInfo(const PacketData& packet);
SystemHeartbeatPayload STARreceiveSystemHeartbeat(const PacketData& packet);
BetrayalPayload STARreceiveBetrayal(const PacketData& packet);
// USE THEM TO EXTRACT THE PAYLOAD DATA FROM THE RECEIVED PACKET, THEY HANDLE THE DECRYPTION AND PROPER CASTING OF THE PAYLOAD FOR YOU, JUST PROVIDE THE RECEIVED PACKET AND IT WILL RETURN THE EXTRACTED PAYLOAD STRUCTURE WITH THE DATA FILLED IN







#endif
