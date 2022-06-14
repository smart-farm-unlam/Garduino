const int SensorPin = 35;
const int airValue = 3615;
const int waterValue = 1230;
int soilMoistureValue = 0;
int soilMoisturePercent = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("Prueba soil moisture sensor:");
}

void loop() {
    delay(2000);
    readSensor();
}

void readSensor() {
    soilMoistureValue = analogRead(SensorPin);
    Serial.println("Sensor value: " + String(soilMoistureValue));
    soilMoisturePercent = map(soilMoistureValue, waterValue, airValue, 100, 0);
    Serial.println("Humedad de la tierra: " + String(soilMoisturePercent) + "%");
}