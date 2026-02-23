#include <StarDustSerial.h>



void setup() {
  Serial.begin(115200);
  // Serial2.begin(baud, config, RX_PIN, TX_PIN);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);



  starDustSerialBegin(0x01, 0x00, Serial2);



  Serial.println("Sistem basladi. Veri yolu: GPIO 17");
}



void loop() {
  STARsendSystemHeartbeat(1, true, 0.02, 0.05, 0);
  delay(1000);
}