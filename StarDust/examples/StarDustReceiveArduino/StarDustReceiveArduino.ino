#include <Arduino.h>
#include <SoftwareSerial.h>
#include "StarDust.h"

// Arduino Yazılımsal Seri Port Pinleri
SoftwareSerial softSerial(2, 3); // RX=2, TX=3

StarDust slave;

void setup() {
    Serial.begin(9600);   // Bilgisayar Ekranı (Debug)
    softSerial.begin(9600); // ESP32 ile Haberleşme
    
    // Slave Başlat: Kendi ID 0x01, Hedef ESP ID 0x00
    // softSerial nesnesini Stream referansı olarak veriyoruz
    slave.begin(softSerial, 0x01, 0x00);
    
    Serial.println("Arduino Slave Hazir. (Pin 2=RX, 3=TX)");
}

void loop() {
    PacketData rxPacket;

    // Kilitlenmeyen (non-blocking) okuma
    if (slave.update(rxPacket)) {
        
        if (rxPacket.header.type == COMMAND) {
            CommandPayload cmd = slave.receiveCommand(rxPacket);
            
            Serial.println("--- Komut Geldi ---");
            Serial.print("Hedef Aci: "); Serial.println(cmd.targetYaw);
            Serial.print("Eylem Kodu: "); Serial.println(cmd.actionCode);
            Serial.println("------------------");
            
            // Onay için Master'a bir Heartbeat (veya Accept) gönderilebilir
            slave.sendAccept(1, 0x05, true);
        }
    }
}