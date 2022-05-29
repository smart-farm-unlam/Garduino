#include <SoftwareSerial.h>

SoftwareSerial D1(10,11);   //RX=10 TX=11

void setup() {

}

void loop() {
    if (D1.available()) {
        Serial.write(D1.read());
    }

    if(Serial.available()) {
        D1.write(Serial.read());
    }
}