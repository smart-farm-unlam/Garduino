// Incluir librerías:
//   - DHT sensor library by adafruit
//   - Adafruit unified sensor

#include <DHT_U.h>
#include <DHT.h>

int PIN_SENSOR = 2; // pin digital usado para el sensor

DHT dht(PIN_SENSOR, DHT22);

void setup() {
    Serial.begin(9600);
    dht.begin();
}

void loop() {
    float tempC = dht.readTemperature();
    float tempF = dht.readTemperature(true);
    float hum = dht.readHumidity();

    if (isnan(tempC) || isnan(tempF) || isnan(hum)) {
        Serial.println("Revisar conexión");
    }
    else {
        Serial.println("Tem: " + String(tempC, 1) + "C " + String(tempF, 1) + "F");
        Serial.println("Hum: " + String(hum, 1) + "%");
    }

    delay(1000);
}
