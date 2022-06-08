//External Libraries
#include <DHT.h>
#include <ThingSpeak.h>
#include <WiFi.h>
//-----------------------------------------------

//Constants
//PINS
const int PIN_DHT = 33;
const int PIN_SOIL_MOISTURE_1 = 32;
const int PIN_SOIL_MOISTURE_2 = 35;
const int PIN_SOIL_MOISTURE_3 = 34;


DHT dht(PIN_DHT, DHT22);
const int CANT_SAMPLES = 5;

//WIFI Settings
const char* ssid = "Movistar14";
const char* password = "mate2306";
WiFiClient client;

//ThingSpeak keys
unsigned long CHANNEL_ID = 1757253;
const char* WRITE_API_KEY = "GS9K4RKUJ6IQZT7F";

//Software timer
const int MEASUREMENT_TIME = 15000;     //15 seconds
unsigned long current_time, previous_time;

//-----------------------------------------------

//Values
//DHT22
float temp = 0;
float hum = 0;

//Capacitive soil moisture Sensor
int soilMoistureValue_1 = 0;
int soilMoisturePercent_1 = 0;
int soilMoistureValue_2 = 0;
int soilMoisturePercent_2 = 0;
int soilMoistureValue_3 = 0;
int soilMoisturePercent_3 = 0;
const int airValue = 3650;
const int waterValue = 1250;

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
        readSoilMoistureSensors();
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

void readSoilMoistureSensors() {
    soilMoistureValue_1 = analogRead(PIN_SOIL_MOISTURE_1);
    soilMoisturePercent_1 = map(soilMoistureValue_1, waterValue, airValue, 100, 0);
    Serial.println("Humidity value 1: " + String(soilMoistureValue_1));
    Serial.println("Soil Humidity percentage 1: " + String(soilMoisturePercent_1) + "%");

    soilMoistureValue_2 = analogRead(PIN_SOIL_MOISTURE_2);
    soilMoisturePercent_2 = map(soilMoistureValue_2, waterValue, airValue, 100, 0);
    Serial.println("Humidity value 2: " + String(soilMoistureValue_2));
    Serial.println("Soil Humidity percentage 2: " + String(soilMoisturePercent_2) + "%");

    soilMoistureValue_3 = analogRead(PIN_SOIL_MOISTURE_3);
    soilMoisturePercent_3 = map(soilMoistureValue_3, waterValue, airValue, 100, 0);
    Serial.println("Humidity value 3: " + String(soilMoistureValue_3));
    Serial.println("Soil Humidity percentage 3: " + String(soilMoisturePercent_3) + "%");
}

void sendDataToServer() {
    //TODO POST TO OUR SERVER
    ThingSpeak.setField(1, temp);
    ThingSpeak.setField(2, hum);
    ThingSpeak.setField(3, soilMoisturePercent_1);
    ThingSpeak.setField(4, soilMoisturePercent_2);
    ThingSpeak.setField(5, soilMoisturePercent_3);

    ThingSpeak.writeFields(CHANNEL_ID, WRITE_API_KEY);
    Serial.println(">> Data sent to ThingSpeak!");
}