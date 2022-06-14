//External Libraries
#include <DHT.h>
#include <ThingSpeak.h>
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
//-----------------------------------------------

//Constants
//PINS
const int PIN_DHT = 33;
const int PIN_SOIL_MOISTURE_1 = 35;
const int PIN_SOIL_MOISTURE_2 = 34;
const int PIN_SOIL_MOISTURE_3 = 39;
const int PIN_DS18B20 = 32;
const int TEMPERATURE_PRECISION = 9;

//Sensor configuration
DHT dht(PIN_DHT, DHT22);
OneWire oneWire(PIN_DS18B20);
DallasTemperature sensors(&oneWire);
DeviceAddress first_thermometer, second_thermometer, third_thermometer;

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
const int ERROR_VALUE = -99;
//DHT22
float temp = ERROR_VALUE;
float hum = ERROR_VALUE;

//Capacitive soil moisture Sensor
int soil_moisture_percent_array[] = {ERROR_VALUE, ERROR_VALUE, ERROR_VALUE};
const int AIR_VALUE = 3700;
const int WATER_VALUE = 1200;

//ds18b20 Moisture Sensor
int soil_temperature_array[] = {ERROR_VALUE, ERROR_VALUE, ERROR_VALUE};
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

    //Sensors initialize
    initializeSensors();
    
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
        readSoilTemperatureSensors();
        sendDataToServer();
        previous_time = millis();
    }
    
}

//Error +-2.5%
void readTemperatureAndHumidity() { 
    temp = dht.readTemperature();
    hum = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {
        temp = ERROR_VALUE;
        hum = ERROR_VALUE;
    }

    Serial.println("Temp: " + String(temp, 1) + "°C");
    Serial.println("Hum: " + String(hum, 1) + "%");
}

void readSoilMoistureSensors() {
    setSoilMoisture(PIN_SOIL_MOISTURE_1, 0);
    setSoilMoisture(PIN_SOIL_MOISTURE_2, 1);
    setSoilMoisture(PIN_SOIL_MOISTURE_3, 2);
}

void setSoilMoisture(int pin, int index) {
    int soilMoistureValue = analogRead(pin);
    int soilMoistureValuePercent = map(soilMoistureValue, WATER_VALUE, AIR_VALUE, 100, 0);
    if (soilMoistureValuePercent < 0 || soilMoistureValuePercent > 100) {
        soilMoistureValuePercent = ERROR_VALUE;
    }
    Serial.println("Humidity value " + String(index + 1) + ": " + String(soilMoistureValue));
    Serial.println("Soil Humidity percentage "+ String(index + 1) + ": " + String(soilMoistureValuePercent) + "%");
    soil_moisture_percent_array[index] = soilMoistureValuePercent;
}

void readSoilTemperatureSensors() {
    sensors.requestTemperatures();
    setSoilTemperature(first_thermometer, 0);
    setSoilTemperature(second_thermometer, 1);
    setSoilTemperature(third_thermometer, 2);
}

void setSoilTemperature(DeviceAddress deviceAddress, int index)
{
  float tempC = sensors.getTempC(deviceAddress);
  if(tempC == DEVICE_DISCONNECTED_C) 
  {
    Serial.println("Error ["+ String(index + 1) + "]: Could not read temperature data");
    tempC = ERROR_VALUE;
  }
  Serial.println("Temp " + String(index + 1) + ": " + String(tempC, 1) + "°C");
  soil_temperature_array[index] = tempC;
}

void initializeSensors() {
    //DHT22 init
    dht.begin();    

    //ds18b20 init
    sensors.begin();
    Serial.println("Locating ds18b20 devices...");
    Serial.println("Found " + String(sensors.getDeviceCount()) + " devices");
    if (!sensors.getAddress(first_thermometer, 0)) Serial.println("Unable to find address for Device 0");
    if (!sensors.getAddress(second_thermometer, 1)) Serial.println("Unable to find address for Device 1");
    if (!sensors.getAddress(third_thermometer, 2)) Serial.println("Unable to find address for Device 2");

    // show the addresses we found on the bus
    Serial.print("Device 0 Address: ");
    printAddress(first_thermometer);
    Serial.println();

    Serial.print("Device 1 Address: ");
    printAddress(second_thermometer);
    Serial.println();

    Serial.print("Device 2 Address: ");
    printAddress(third_thermometer);
    Serial.println();

    // set the resolution to 9 bit per device
    sensors.setResolution(first_thermometer, TEMPERATURE_PRECISION);
    sensors.setResolution(second_thermometer, TEMPERATURE_PRECISION);
    sensors.setResolution(third_thermometer, TEMPERATURE_PRECISION);
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void sendDataToServer() {
    //TODO POST TO OUR SERVER
    ThingSpeak.setField(1, temp);
    ThingSpeak.setField(2, hum);
    ThingSpeak.setField(3, soil_moisture_percent_array[0]);
    ThingSpeak.setField(4, soil_moisture_percent_array[1]);
    ThingSpeak.setField(5, soil_moisture_percent_array[2]);
    ThingSpeak.setField(6, soil_temperature_array[0]);
    ThingSpeak.setField(7, soil_temperature_array[1]);
    ThingSpeak.setField(8, soil_temperature_array[2]);

    ThingSpeak.writeFields(CHANNEL_ID, WRITE_API_KEY);
    Serial.println(">> Data sent to ThingSpeak!");
}