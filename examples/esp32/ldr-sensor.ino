const int sensorPin = 33;
int lightValue = 0;
int lightValuePercent = 0;

void setup() {
    Serial.begin(115200);
}

void loop() {
    delay(2000);
    readSensor();
}

void readSensor() {
    lightValue = digitalRead(sensorPin);
    Serial.println("Sensor value: " + String(lightValue));
}