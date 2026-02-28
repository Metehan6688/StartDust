#include <Arduino.h>
#include "StarDust.h"

// ESP32 Donanımsal Serial2 Pinleri
#define RXD2 16
#define TXD2 17

StarDust master;

void setup() {
    Serial.begin(115200); // PC Debug
    
    // Serial2 yapılandırması (Baud, Protokol, RX, TX)
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
    
    // Master Başlat: ID 0x00, Hedef Arduino ID 0x01
    master.begin(Serial2, 0x00, 0x01);
    master.setTimeout(100);
    
    Serial.println("ESP32 Master Hazir. (Pin 16=RX, 17=TX)");
}

void loop() {
    static uint32_t lastSend = 0;
    
    // Her 2 saniyede bir Arduino'ya komut gönder
    if (millis() - lastSend > 2000) {
        lastSend = millis();
        
        Serial.println("Arduino'ya hareket komutu gonderiliyor...");
        master.sendCommand(1, 90.0f, 0.0f, 0x01); // 90 dereceye dön komutu
    }

    // Geri bildirim bekle (Arduino cevap dönerse)
    PacketData response;
    if (master.update(response)) {
        Serial.println("Arduino'dan paket alindi!");
    }
}