//External Libraries
#include <DHT.h>
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
//-----------------------------------------------

//Constants
//PINS
const int PIN_DHT = 33;
const int PIN_SOIL_MOISTURE_1 = 35;
const int PIN_SOIL_MOISTURE_2 = 34;
const int PIN_SOIL_MOISTURE_3 = 39;
const int PIN_DS18B20 = 32;
const int TEMPERATURE_PRECISION = 9;
const int PIN_VALVE_1 = 15;
const int PIN_VALVE_2 = 4;
const int PIN_VALVE_3 = 5;
const int PIN_WATER_PUMP = 18;

//Sensor configuration
DHT dht(PIN_DHT, DHT22);
OneWire oneWire(PIN_DS18B20);
DallasTemperature sensors(&oneWire);
DeviceAddress first_thermometer, second_thermometer, third_thermometer;

//WIFI Settings
const char* ssid = "Movistar14";
const char* password = "mate2306";

//Software timer
const int MEASUREMENT_TIME = 15000;     //15 seconds
unsigned long current_time, previous_time;

//Server path
//String SERVER_URI = String("http://192.168.1.45:8080");
String SERVER_URI = String("https://smartfarmunlam.azurewebsites.net");

//FARM ID
String FARM_ID = String("62acc77270752f2b6bbb88ee");

//-----------------------------------------------
//Values
const double ERROR_VALUE = -99;
//DHT22
float temp = ERROR_VALUE;
float hum = ERROR_VALUE;

//Capacitive soil moisture limit
const int AIR_VALUE = 3700;
const int WATER_VALUE = 1200;
//-----------------------------------------------
//Classes
class Measure {
    public: 
        //time_t dateTime;
        float value;

    Measure() {
        value = ERROR_VALUE;
    }

    Measure(double value) {
        value = value;
    }
};

class Sensor {
    public:
        std::string code;
        Measure measure;
    
    Sensor(std::string code) {
        code = code;
        measure = Measure(ERROR_VALUE);
    }

    Sensor(std::string code, Measure measure) {
        code = code;
        measure = measure;
    }
};
//----------------------------------------------
//Events declaration
Sensor tempSensor = Sensor("AT1");
Sensor humiditySensor = Sensor("AH1");
Sensor soilMoistureSensor1 = Sensor("SH1");
Sensor soilMoistureSensor2 = Sensor("SH2");
Sensor soilMoistureSensor3 = Sensor("SH3");
Sensor soilTempSensor1 = Sensor("ST1") ;
Sensor soilTempSensor2 = Sensor("ST2");
Sensor soilTempSensor3 = Sensor("ST3");

Sensor soilMoistureArray[] = {soilMoistureSensor1, soilMoistureSensor2, soilMoistureSensor3}; 
Sensor soilTemperatureArray[] = {soilTempSensor1, soilTempSensor2, soilTempSensor3}; 

//Irrigation system
boolean irrigationSectorStatus[] = {false, false, false};
String waterPumpStatus = "OFF";

//----------------------------------------------

void setup() {
    Serial.begin(115200);

    initWiFi();

    //Conf Pines
    pinMode(PIN_VALVE_1, OUTPUT);
    pinMode(PIN_VALVE_2, OUTPUT);
    pinMode(PIN_VALVE_3, OUTPUT);
    digitalWrite(PIN_VALVE_1, HIGH);
    digitalWrite(PIN_VALVE_2, HIGH);
    digitalWrite(PIN_VALVE_3, HIGH);

    //Sensors initialize
    initializeSensors();

    //Retrieved data from server
    getSectorsInfo();
    
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
        //sendDataToServer();
        irrigationEventResolver();
        previous_time = millis();
    }
    
}

void initWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.print("Connect to the WiFi network with IP: ");
    Serial.println(WiFi.localIP());
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

    tempSensor.measure.value = temp;
    humiditySensor.measure.value = hum;
}

void readSoilMoistureSensors() {
    setSoilMoisture(PIN_SOIL_MOISTURE_1, 1);
    setSoilMoisture(PIN_SOIL_MOISTURE_2, 2);
    setSoilMoisture(PIN_SOIL_MOISTURE_3, 3);
}

void setSoilMoisture(int pin, int index) {
    int soilMoistureValue = analogRead(pin);
    int soilMoistureValuePercent = map(soilMoistureValue, WATER_VALUE, AIR_VALUE, 100, 0);
    if (soilMoistureValuePercent <= 0 || soilMoistureValuePercent > 100) {
        soilMoistureValuePercent = ERROR_VALUE;
    }
    Serial.println("Humidity value " + String(index) + ": " + String(soilMoistureValue));
    Serial.println("Soil Humidity percentage "+ String(index) + ": " + String(soilMoistureValuePercent) + "%");
    soilMoistureArray[index-1].measure.value = soilMoistureValuePercent;
}

void readSoilTemperatureSensors() {
    sensors.requestTemperatures();
    setSoilTemperature(first_thermometer, 1);
    setSoilTemperature(second_thermometer, 2);
    setSoilTemperature(third_thermometer, 3);
}

void setSoilTemperature(DeviceAddress deviceAddress, int index)
{
  float tempC = sensors.getTempC(deviceAddress);
  if(tempC == DEVICE_DISCONNECTED_C) 
  {
    Serial.println("Error ["+ String(index) + "]: Could not read temperature data");
    tempC = ERROR_VALUE;
  }
  Serial.println("Temp " + String(index) + ": " + String(tempC, 1) + "°C");
  soilTemperatureArray[index-1].measure.value = tempC;
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
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient client;

        String endpoint = SERVER_URI + "/sensors/" + FARM_ID + "/events";

        client.begin(endpoint);
        client.addHeader("Content-Type", "application/json");

        char jsonInput[1024];
        StaticJsonDocument<1024> doc;
        appendJsonObject(doc, tempSensor);
        appendJsonObject(doc, humiditySensor);

        for (Sensor sensorMoisture : soilMoistureArray) {
            appendJsonObject(doc, sensorMoisture);
        }

        for (Sensor sensorTemperature : soilTemperatureArray) {
            appendJsonObject(doc, sensorTemperature);
        }

        serializeJson(doc, jsonInput);

        Serial.println(jsonInput);

        int httpCode = client.POST(String(jsonInput));

        if (httpCode > 0) {
            Serial.println("Status code: " + String(httpCode));
        } else {
            Serial.println("Error on sending POST: " + String(httpCode));
        }

        client.end();
    } else {
        Serial.println("Error in WiFi connection");
    }
}

void appendJsonObject(StaticJsonDocument<1024> &doc, Sensor sensor) {
    JsonObject sensorDoc = doc.createNestedObject();
    sensorDoc["code"] = sensor.code;

    JsonArray measuresArray = sensorDoc.createNestedArray("measures");

    JsonObject measure = measuresArray.createNestedObject();
    measure["value"] = sensor.measure.value; 
}

void getSectorsInfo() {
    if (WiFi.status() == WL_CONNECTED) {
        String uri = SERVER_URI + "/sectors/" + FARM_ID + "/crop-types";
        String sectorsData = getRequest(uri.c_str());
        Serial.println(sectorsData);

        char jsonOutput[2048];
        sectorsData.replace(" ", "");
        sectorsData.replace("\n", "");
        sectorsData.trim();
        sectorsData.remove(0, 1);
        sectorsData.toCharArray(jsonOutput, 2048);

        StaticJsonDocument<200> doc;
        deserializeJson(doc, jsonOutput);

        String id1 = doc.getElement(0);
        Serial.println("Dato obtenido: " + id1);
    } else {
        Serial.println("Error in WiFi connection");
    }
}

void irrigationEventResolver() {
    Serial.println("Mock irrigation");

    float sms1 = soilMoistureArray[0].measure.value;
    float sms2 = soilMoistureArray[1].measure.value;
    float sms3 = soilMoistureArray[2].measure.value;

    Serial.println("Valores humedad por sector:");
    Serial.println(sms1);
    Serial.println(sms2);
    Serial.println(sms3);
    
    boolean hasToActivateWaterPump = false;
    int hasToDeactivateWaterPump = 0;
    String sectorId;

    if (sms1 != ERROR_VALUE && sms1 < 60.0 && irrigationSectorStatus[0] == false) 
    {
        Serial.println("Abrir valvula 1");
        digitalWrite(PIN_VALVE_1, LOW);
        irrigationSectorStatus[0] = true;
        hasToActivateWaterPump = true;
        sectorId = "S1";
    }

    if(sms1 != ERROR_VALUE && sms1 >= 60.00 && sms1 <= 80.00 
        && irrigationSectorStatus[0] == true) 
    {
        Serial.println("Cerra valvula 1");
        digitalWrite(PIN_VALVE_1, HIGH);
        irrigationSectorStatus[0] = false;
        sectorId = "S1";
    }

    if (sms2 != ERROR_VALUE && sms2 < 60.0 && irrigationSectorStatus[1] == false) 
    {
        Serial.println("Abrir valvula 2");
        digitalWrite(PIN_VALVE_2, LOW);
        irrigationSectorStatus[1] = true;
        hasToActivateWaterPump = true;
        sectorId = "S2";
    }

    if(sms2 != ERROR_VALUE && sms2 >= 60.00 && sms2 <= 80.00 
        && irrigationSectorStatus[1] == true) 
    {
        Serial.println("Cerra valvula 2");
        digitalWrite(PIN_VALVE_3, HIGH);
        irrigationSectorStatus[1] = false;
        sectorId = "S2";
    }

    if (sms3 != ERROR_VALUE && sms3 < 60.0 && irrigationSectorStatus[2] == false) {
        Serial.println("Abrir valvula 3");
        digitalWrite(PIN_VALVE_3, LOW); 
        irrigationSectorStatus[2] = true;
        hasToActivateWaterPump = true;
        sectorId = "S3";
    }

    if(sms3 != ERROR_VALUE && sms3 >= 60.00 && sms3 <= 70.00 
        && irrigationSectorStatus[2] == true) 
    {
        Serial.println("Cerra valvula 3");
        digitalWrite(PIN_VALVE_1, HIGH);
        irrigationSectorStatus[2] = false;
        sectorId = "S3";
    }

    for (boolean irrigationStatus : irrigationSectorStatus) {
        if(irrigationStatus == false) {
            hasToDeactivateWaterPump++;
        }
    }

    int cantSectores = 3;
    if (waterPumpStatus == "ON" && hasToDeactivateWaterPump == cantSectores) {
        Serial.println("Todos los sectores estan bien, apagando bomba de agua");
        digitalWrite(PIN_WATER_PUMP, HIGH);
        waterPumpStatus = "OFF";
        sendIrrigationEventToServer(sectorId.c_str());
    }

    if(waterPumpStatus == "OFF" && hasToActivateWaterPump == true) {
        Serial.println("Prendiendo bomba de agua");
        digitalWrite(PIN_WATER_PUMP, LOW);
        waterPumpStatus = "ON";
        sendIrrigationEventToServer(sectorId.c_str());
    }
    Serial.println("Termine de probar el mock");
}

String getRequest(const char* uri) {
    HTTPClient http;
    http.begin(uri);

    int httpCode = http.GET();

    String payload = "{}";

    if (httpCode > 0) {
        Serial.println("HTTP Response code: " +  String(httpCode));
        payload = http.getString();
    } else {
        Serial.println("Error on GET Request, error code: " + String(httpCode));
    }

    http.end();

    return payload;
}

void sendIrrigationEventToServer(const char* sectorId) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient client;

        String endpoint = SERVER_URI + "/events/newEvent/" + FARM_ID;

        Serial.println(endpoint);

        client.begin(endpoint);
        client.addHeader("Content-Type", "application/json");

        char jsonInput[1024];
        StaticJsonDocument<1024> doc;
        JsonObject object = doc.to<JsonObject>();
        object["eventType"] = "IrrigationEvent";
        object["sectorId"] = sectorId;
        object["status"] = waterPumpStatus;
        //object["date"] = null;

        serializeJson(doc, jsonInput);

        Serial.println(jsonInput);

        int httpCode = client.POST(String(jsonInput));

        if (httpCode > 0) {
            Serial.println("Status code: " + String(httpCode));
        } else {
            Serial.println("Error on sending POST: " + String(httpCode));
        }

        client.end();
    } else {
        Serial.println("Error in WiFi connection");
    }
}