//External Libraries
#include <DHT.h>
#include <ThingSpeak.h>
#include <WiFi.h>
//-----------------------------------------------

//Constants
//DHT22
const int PIN_DHT = 33;
DHT dht(PIN_DHT, DHT22);
const int CANT_SAMPLES = 5;

//WIFI Settings
const char* ssid = "Movistar14";
const char* password = "mate2306";
WiFiClient client;

//ThingSpeak keys
unsigned long CHANNEL_ID = 1757253;
const char* WRITE_API_KEY = "GS9K4RKUJ6IQZT7F";

//Values
float temp = 0;
float hum = 0;

//Software timer
const int MEASUREMENT_TIME = 15000;     //15 seconds
unsigned long current_time, previous_time;

//-----------------------------------------------

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

    //software timer init
    current_time = millis();
    previous_time = millis();
}

void loop() {
    current_time = millis();

    if ((current_time - previous_time) > MEASUREMENT_TIME) {
        Serial.println("-----------------------------------");
        Serial.println("Start collecting data from sensors");
        readTemperatureAndHumidity();
        sendDataToServer();
        previous_time = millis();
    }
    
}

//Error +-2.5%
void readTemperatureAndHumidity() {
    float temp_sum = 0;
    float hum_sum = 0;

    for(int i = 0; i < CANT_SAMPLES; i++) {
        delay(500); //0.5Hz between DHT22 samples
        float temp_aux = dht.readTemperature();
        float hum_aux = dht.readHumidity();

        while(isnan(temp_aux) || isnan(hum_aux)) {
            Serial.println("Failed read on DHT22 sensor, repeating read");
            delay(2000);
            temp_aux = dht.readTemperature();
            hum_aux = dht.readHumidity();
        }

        temp_sum += temp_aux;
        hum_sum += hum_aux;
    }

    temp = temp_sum / CANT_SAMPLES;
    hum = hum_sum / CANT_SAMPLES;

    Serial.println("Temp: " + String(temp, 1) + "°C");
    Serial.println("Hum: " + String(hum, 1) + "%");
}

void sendDataToServer() {
    //TODO POST TO OUR SERVER
    ThingSpeak.setField(1, temp);
    ThingSpeak.setField(2, hum);
    ThingSpeak.writeFields(CHANNEL_ID, WRITE_API_KEY);
    Serial.println("Data sent to ThingSpeak!");
    
}