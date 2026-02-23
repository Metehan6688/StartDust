#include <SoftwareSerial.h>
#include <StarDustSerial.h>

SoftwareSerial espSerial(2, 3);
static PacketData incomingPacket;  // Veriyi tutacak ana paket

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);

  // Kendi ID: 0x01, Master: 0x00
  starDustSerialBegin(0x01, 0x00, espSerial);

  Serial.println("--- STAR DUST SISTEM HAZIR ---");
}

void loop() {


  // STARupdate artık arka planda parsePacket'i kullanarak bayt bayt toplar
  if (STARupdate(incomingPacket)) {

    // Paket geldi! Tipine göre ayıralım
    switch (incomingPacket.header.type) {

      case PacketType::SYSTEMHEARTBEAT:
        {
          // Senin yazdığın o temiz alıcı fonksiyonu
          SystemHeartbeatPayload hb = STARreceiveSystemHeartbeat(incomingPacket);

          Serial.println("\n------------------------------------");
          Serial.println("[!] SIFRELI PAKET COZULDU");
          Serial.print("Sistem Durumu: ");
          Serial.println(hb.beatStatus ? "AKTIF" : "PASIF");
          Serial.print("Hata Payi: ");
          Serial.println(hb.systemErrorChance, 4);
          Serial.print("Kacirilan Atis: ");
          Serial.println(hb.missedBeats);
          Serial.println("------------------------------------");
          break;
        }

        // Diğer paketler (Telemetry vb.) buraya eklenebilir
    }
  }
}