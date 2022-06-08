#include <DHT.h>
#include <ThingSpeak.h>
#include <WiFi.h>

const int PIN_DHT = 33;

DHT dht(PIN_DHT, DHT22);

const char* ssid = "Movistar14";
const char* password = "mate2306";

unsigned long CHANNEL_ID = 1757253;
const char* WRITE_API_KEY = "GS9K4RKUJ6IQZT7F";

//Values
float temp = 0;
float hum = 0;

WiFiClient client;

void setup() {
    Serial.begin(115200);

    Serial.println("Trying to connect to wifi: ");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected!");

    ThingSpeak.begin(client);

    dht.begin();
}

void loop() {
    readTemperatureAndHumidity();

    sendDataToServer();
    delay(14000);
}

void readTemperatureAndHumidity() {
    temp = dht.readTemperature();
    hum = dht.readHumidity();

    while(isnan(temp) || isnan(hum)) {
        Serial.println("Failed read on DHT22 sensor, repeating read");
        delay(2000);
        temp = dht.readTemperature();
        hum = dht.readHumidity();
    }

    Serial.println("Temp: " + String(temp, 1) + "°C");
    Serial.println("Hum: " + String(hum,1) + "%.");

    Serial.println("------------------");
}

void sendDataToServer() {
    //TODO POST TO OUR SERVER
    ThingSpeak.setField(1, temp);
    ThingSpeak.setField(2, hum);
    ThingSpeak.writeFields(CHANNEL_ID, WRITE_API_KEY);
    Serial.println("Data sent to ThingSpeak!");
    
}