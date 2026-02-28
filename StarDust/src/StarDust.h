#ifndef STARDUST_H
#define STARDUST_H

#include <stdint.h>
#include <string.h>

#ifdef ARDUINO
    #include <Arduino.h>
#else
    #include <cstddef>
    // PC testleri veya özel ortamlar için temel Stream sınıfı
    class Stream {
    public:
        virtual size_t write(const uint8_t* buffer, size_t size) { return 0; }
        virtual int available() { return 0; }
        virtual int read() { return -1; }
    };
    extern uint32_t millis(); // RTOS veya PC için kendi millis fonksiyonunuzu sağlamalısınız
#endif

/* MUTLAK PROTOKOL TANIMLAYICILARI */
constexpr uint8_t PACKET_START_BYTE = 0xAA; //
constexpr uint8_t PAYLOADSIZE = 64;         //
constexpr uint8_t BROADCAST_ID = 0xFF;

/* PAKET TİPLERİ */
enum PacketType : uint8_t { //
    REQUEST = 1, ACCEPT = 2, REFUSE = 3,
    TELEMETRY = 4, COMMAND = 5, ERROR = 101, EMERGENCY = 102,
    SYSTEMCOMMAND = 201, SYSTEMINFO = 202, SYSTEMHEARTBEAT = 203,
    BETRAYAL = 66
};

/* ======================================================= */
/* =============== VERİ PAKETİ YAPILARI ===================*/
/* (Ana struct yapılarına ve isimlerine dokunulmadı) */
/* ======================================================= */
struct __attribute__((packed)) RequestPayload { uint8_t version; uint8_t requestType; bool isCritical; };
struct __attribute__((packed)) AcceptPayload { uint8_t version; uint8_t acceptType; bool accepted; };
struct __attribute__((packed)) RefusePayload { uint8_t version; uint8_t refuseType; bool refused; };
struct __attribute__((packed)) TelemetryPayload { uint8_t version; double latitude; double longitude; double altitude; float yaw; float pitch; uint32_t timestamp; uint8_t status; };
struct __attribute__((packed)) CommandPayload { uint8_t version; float targetYaw; float targetPitch; uint8_t actionCode; };
struct __attribute__((packed)) ErrorPayload { uint8_t version; uint16_t errorCode; uint16_t errorLocation; uint8_t errorSeverity; };
struct __attribute__((packed)) EmergencyPayload { uint8_t version; uint16_t emergencyCode; uint16_t emergencyLocation; uint8_t emergencySeverity; };
struct __attribute__((packed)) SystemCommandPayload { uint8_t version; uint16_t commandCode; uint16_t commandParameter; uint8_t commandSenderID; uint8_t commandAuthorityLevel; };
struct __attribute__((packed)) SystemInfoPayload { uint8_t version; uint8_t infoType; bool systemOperational; float systemLoad; float systemTemperature; float systemVoltage; uint32_t uptime; float systemErrorChance; float expectedErrorChance; float systemReliability; };
struct __attribute__((packed)) SystemHeartbeatPayload{ uint8_t version; bool beatStatus; float systemErrorChance; float expectedErrorChance; uint8_t missedBeats; };
struct __attribute__((packed)) BetrayalPayload{ uint8_t version; bool isBetrayal; uint16_t betrayalCode; uint16_t betrayalLocation; uint8_t betrayalSeverity; bool knowsSecret; bool planExecuted; bool hasEscapePlan; bool preparedForBetrayal; float betrayalSuccessChance; float betrayalDetectionChance; bool hasAllies; uint8_t numberOfAllies; float allyLoyalty; bool knighFall; };

/* =============== ORTAK PAKET YAPILARI ===================*/
struct __attribute__((packed)) PacketHeader {
    uint8_t start; uint8_t source; uint8_t target; PacketType type; uint8_t size; //
};

struct __attribute__((packed)) PacketData {
    PacketHeader header;
    uint8_t payload[PAYLOADSIZE];
    uint16_t crc; //
};

enum class ParserState { //
    WAIT_START, READ_HEADER, READ_PAYLOAD, READ_CRC
};

/* ======================================================= */
/* =================== STARDUST SINIFI ====================*/
/* ======================================================= */
class StarDust {
public:
    StarDust();

    // Sistemi Başlatma
    void begin(Stream& port, uint8_t myID, uint8_t defaultTargetID = 0x00);
    
    // Güvenlik ve Ayarlar
    void setCryptoKey(const uint8_t* newKey); //
    void setTimeout(uint32_t timeoutMs);      // Okuma için timeout süresi belirleme
    void setTargetID(uint8_t targetID);       // İletişim kurulacak hedefi/slave'i değiştir

    // Ana Döngü Güncellemesi (Non-blocking Okuma)
    bool update(PacketData& outPacket);       //

    // ==== GÖNDERİM FONKSİYONLARI ====
    PacketData sendRequest(uint8_t version, uint8_t requestType, bool isCritical); //
    PacketData sendAccept(uint8_t version, uint8_t acceptType, bool accepted); //
    PacketData sendRefuse(uint8_t version, uint8_t refuseType, bool refused); //
    PacketData sendTelemetry(uint8_t version, double latitude, double longitude, double altitude, float yaw, float pitch, uint32_t timestamp, uint8_t status); //
    PacketData sendCommand(uint8_t version, float targetYaw, float targetPitch, uint8_t actionCode); //
    PacketData sendError(uint8_t version, uint16_t errorCode, uint16_t errorLocation, uint8_t errorSeverity); //
    PacketData sendEmergency(uint8_t version, uint16_t emergencyCode, uint16_t emergencyLocation, uint8_t emergencySeverity); //
    PacketData sendSystemCommand(uint8_t version, uint16_t commandCode, uint16_t commandParameter, uint8_t commandSenderID, uint8_t commandAuthorityLevel); //
    PacketData sendSystemInfo(uint8_t version, uint8_t infoType, bool systemOperational, float systemLoad, float systemTemperature, float systemVoltage, uint32_t uptime, float systemErrorChance, float expectedErrorChance, float systemReliability); //
    PacketData sendSystemHeartbeat(uint8_t version, bool beatStatus, float systemErrorChance, float expectedErrorChance, uint8_t missedBeats); //
    PacketData sendBetrayal(uint8_t version, bool isBetrayal, uint16_t betrayalCode, uint16_t betrayalLocation, uint8_t betrayalSeverity, bool knowsSecret, bool planExecuted, bool hasEscapePlan, bool preparedForBetrayal, float betrayalSuccessChance, float betrayalDetectionChance, bool hasAllies, uint8_t numberOfAllies, float allyLoyalty, bool knighFall); //

    // ==== ALIM FONKSİYONLARI ====
    RequestPayload receiveRequest(const PacketData& packet); //
    AcceptPayload receiveAccept(const PacketData& packet); //
    RefusePayload receiveRefuse(const PacketData& packet); //
    TelemetryPayload receiveTelemetry(const PacketData& packet); //
    CommandPayload receiveCommand(const PacketData& packet); //
    ErrorPayload receiveError(const PacketData& packet); //
    EmergencyPayload receiveEmergency(const PacketData& packet); //
    SystemCommandPayload receiveSystemCommand(const PacketData& packet); //
    SystemInfoPayload receiveSystemInfo(const PacketData& packet); //
    SystemHeartbeatPayload receiveSystemHeartbeat(const PacketData& packet); //
    BetrayalPayload receiveBetrayal(const PacketData& packet); //

private:
    Stream* _port;
    uint8_t _myID;
    uint8_t _targetID;
    uint8_t _cryptoKey[16];

    // Parser State (RTOS Uyumlu İzole Değişkenler)
    ParserState _state;
    uint16_t _bytesRead;
    uint8_t _rxBuffer[sizeof(PacketData)]; 
    uint32_t _lastRxTime;
    uint32_t _timeoutMs;

    // Yardımcı İç Fonksiyonlar
    uint16_t calculateCRC16CCITT(const uint8_t *data, uint16_t length); //
    void encryptPayload(uint8_t* payload, uint8_t size); //
    void decryptPayload(uint8_t* payload, uint8_t size); //
    bool validatePacket(PacketData* packet);
    void preparePacket(PacketData& packet, PacketType type, const uint8_t* payloadData, uint8_t payloadSize);
    bool parseByte(uint8_t incomingByte, PacketData& outPacket);
};

#endif
