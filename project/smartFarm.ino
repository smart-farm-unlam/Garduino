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
DallasTemperature soilTemperatureSensors(&oneWire);
DeviceAddress first_thermometer, second_thermometer, third_thermometer;

//WIFI Settings
const char* ssid = "Movistar14";
const char* password = "mate2306";

//Software timer
int MEASUREMENT_TIME = 10000;     //10 seconds
unsigned long current_time, previous_time;
int IRRIGATION_LOOP_TIME = 30000;      //30 seconds
unsigned long irrigation_current_time, irrigation_previous_time;

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
const int AIR_VALUE = 3800;
const int WATER_VALUE = 1250;
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
    
    Sensor(std::string value) {
        code = value;
        measure = Measure(ERROR_VALUE);
    }

    Sensor(std::string value, Measure measure) {
        code = value;
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
    pinMode(PIN_WATER_PUMP, OUTPUT);
    digitalWrite(PIN_VALVE_1, HIGH);
    digitalWrite(PIN_VALVE_2, HIGH);
    digitalWrite(PIN_VALVE_3, HIGH);
    digitalWrite(PIN_WATER_PUMP, HIGH);

    //Sensors initialize
    initializeSensors();

    //Retrieved data from server
    getSectorsInfo();
    
    //software timer init
    current_time = millis();
    previous_time = millis();
    irrigation_current_time = millis();
    irrigation_previous_time = millis();
}

void loop() {
    current_time = millis();
    irrigation_current_time = millis();

    if ((current_time - previous_time) > MEASUREMENT_TIME) {
        Serial.println("-----------------------------------");
        Serial.println("Start collecting data from sensors");
        readTemperatureAndHumidity();
        readSoilMoistureSensors();
        readSoilTemperatureSensors();
        sendDataToServer();
        previous_time = millis();
    }

    if ((irrigation_current_time - irrigation_previous_time > IRRIGATION_LOOP_TIME)) {
        Serial.println("-----------------------------------");
        Serial.println("Control irrigation");
        irrigationEventResolver();
        irrigation_previous_time = millis();
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
    soilTemperatureSensors.requestTemperatures();
    setSoilTemperature(first_thermometer, 1);
    setSoilTemperature(second_thermometer, 2);
    setSoilTemperature(third_thermometer, 3);
}

void setSoilTemperature(DeviceAddress deviceAddress, int index)
{
  float tempC = soilTemperatureSensors.getTempC(deviceAddress);
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
    soilTemperatureSensors.begin();
    Serial.println("Locating ds18b20 devices...");
    Serial.println("Found " + String(soilTemperatureSensors.getDeviceCount()) + " devices");
    if (!soilTemperatureSensors.getAddress(first_thermometer, 0)) Serial.println("Unable to find address for Device 0");
    if (!soilTemperatureSensors.getAddress(second_thermometer, 1)) Serial.println("Unable to find address for Device 1");
    if (!soilTemperatureSensors.getAddress(third_thermometer, 2)) Serial.println("Unable to find address for Device 2");

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
    soilTemperatureSensors.setResolution(first_thermometer, TEMPERATURE_PRECISION);
    soilTemperatureSensors.setResolution(second_thermometer, TEMPERATURE_PRECISION);
    soilTemperatureSensors.setResolution(third_thermometer, TEMPERATURE_PRECISION);
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
    Serial.println("Checking sectors humidity");
    float humidity1 = soilMoistureArray[0].measure.value;
    float humidity2 = soilMoistureArray[1].measure.value;
    float humidity3 = soilMoistureArray[2].measure.value;
    
    boolean hasToActivateWaterPump = false;
    int deactivateWaterPumpVotes = 0;
    String sectorId;

    //get from server
    float minValueS1 = 60.0;
    float minValueS2 = 60.0;
    float minValueS3 = 60.0;

    digitalWrite(PIN_WATER_PUMP, HIGH); //turn it off

    //TODO check if we can replace this with a for loop
    //Check Sector 1
    if (humidity1 != ERROR_VALUE && humidity1 < minValueS1) 
    {
        //If the valve is closed open it!
        if(digitalRead(PIN_VALVE_1) == HIGH) { 
            digitalWrite(PIN_VALVE_1, LOW);
            Serial.println("Valve 1 opened");
        }
        irrigationSectorStatus[0] = true;
        hasToActivateWaterPump = true;
        sectorId = "S1";
    } else if (irrigationSectorStatus[0] == true && (humidity1 == ERROR_VALUE || humidity1 >= minValueS1)) {
        digitalWrite(PIN_VALVE_1, HIGH);
        Serial.println("Valve 1 closed");
        irrigationSectorStatus[0] = false;
        sectorId = "S1";
        Serial.println("Sector 1 humidity ok");
    } else {
        Serial.println("Sector 1 humidity ok");
    }
    

    //Check Sector 2
    if (humidity2 != ERROR_VALUE && humidity2 < minValueS2) {
        //If the valve is closed open it!
        if(digitalRead(PIN_VALVE_2) == HIGH) {
            digitalWrite(PIN_VALVE_2, LOW);
            Serial.println("Valve 2 opened");
        }
        irrigationSectorStatus[1] = true;
        hasToActivateWaterPump = true;
        sectorId = "S2";
    } else if (irrigationSectorStatus[1] == true && (humidity2 == ERROR_VALUE || humidity2 >= minValueS2)) {
        digitalWrite(PIN_VALVE_2, HIGH);
        Serial.println("Valve 2 closed");
        irrigationSectorStatus[1] = false;
        sectorId = "S2";
        Serial.println("Sector 2 humidity ok");
    } else {
        Serial.println("Sector 2 humidity ok");
    }

     //Check Sector 3
    if (humidity3 != ERROR_VALUE && humidity3 < minValueS3) {
        //If the valve is closed open it!
        if(digitalRead(PIN_VALVE_3) == HIGH) {
            digitalWrite(PIN_VALVE_3, LOW);
            Serial.println("Valve 3 opened");
        }
        irrigationSectorStatus[2] = true;
        hasToActivateWaterPump = true;
        sectorId = "S3";
    } else if(irrigationSectorStatus[2] == true && (humidity3 == ERROR_VALUE || humidity3 >= minValueS3)) {
        digitalWrite(PIN_VALVE_3, HIGH);
        Serial.println("Valve 3 closed");
        irrigationSectorStatus[2] = false;
        sectorId = "S3";
        Serial.println("Sector 3 humidity ok");
    } else {
        Serial.println("Sector 3 humidity ok");
    }

    for (boolean irrigationStatus : irrigationSectorStatus) {
        if(irrigationStatus == false) {
            deactivateWaterPumpVotes++;
        }
    }

    if(waterPumpStatus == "OFF" && hasToActivateWaterPump == true) {
        MEASUREMENT_TIME = 2000;
        IRRIGATION_LOOP_TIME = 2000; //Check every 2 seconds
        digitalWrite(PIN_WATER_PUMP, LOW); //turn on water pump
        Serial.println("Water pump on");
        waterPumpStatus = "ON";
        sendIrrigationEventToServer(sectorId.c_str());
    } else if (hasToActivateWaterPump == true) {
        delay(3000);
        digitalWrite(PIN_WATER_PUMP, LOW);
        Serial.println("Water pump on again");
    }

    int cantSectores = 3;
    if (waterPumpStatus == "ON" && deactivateWaterPumpVotes == cantSectores) {
        Serial.println("All sectors ok, turn off water pump");
        digitalWrite(PIN_WATER_PUMP, HIGH); //turn it off
        waterPumpStatus = "OFF";
        sendIrrigationEventToServer(sectorId.c_str());
        MEASUREMENT_TIME = 10000;
        IRRIGATION_LOOP_TIME = 30000;
    }
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