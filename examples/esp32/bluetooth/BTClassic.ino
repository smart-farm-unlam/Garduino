#include <BluetoothSerial.h>

BluetoothSerial SerialBT;

const String deviceName = "facu_esp32";

void setup() {
    Serial.begin(115200);

    SerialBT.begin(deviceName); //el nombre por defecto es ESP32
    Serial.println(deviceName + " listo para emparejar...");
}

void loop() {
    serialEvent();
}

void serialEvent() {
    if (Serial.available())
        SerialBT.write(Serial.read());
    if(SerialBT.available())
        Serial.write(SerialBT.read());
    
    delay(20);
}