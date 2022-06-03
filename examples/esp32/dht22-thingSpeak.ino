#include <DHT.h>
#include <ThingSpeak.h>
#include <WiFi.h>

const int PIN_DHT = 32;

DHT dht(PIN_DHT, DHT22);

const char* ssid = "Movistar14";
const char* password = "mate2306";

unsigned long CHANNEL_ID = 1757253;
const char* WRITE_API_KEY = "GS9K4RKUJ6IQZT7F";

WiFiClient client;

void setup() {
    Serial.begin(115200);
    Serial.println("Test de sensores:");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi conectado!");

    ThingSpeak.begin(client);

    dht.begin();
}

void loop() {
    delay(2000);

    leerDht22();
    ThingSpeak.writeFields(CHANNEL_ID, WRITE_API_KEY);
    Serial.println("Datos enviados a ThingSpeak!");

    delay(14000);
}

void leerDht22() {
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    while(isnan(temp) || isnan(hum)) {
        Serial.println("Lectura fallida en el sensor DHT22, repitiendo lectura");
        delay(2000);
        temp = dht.readTemperature();
        hum = dht.readHumidity();
    }

    Serial.println("Temperatura: " + String(temp, 1) + "°C");
    Serial.println("Humedad: " + String(hum,1) + "%.");

    Serial.println("------------------");

    ThingSpeak.setField(1, temp);
    ThingSpeak.setField(2, hum);
}