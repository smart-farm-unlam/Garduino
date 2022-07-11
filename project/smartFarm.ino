//External Libraries
#include <DHT.h>
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <SPIFFS.h>
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

//Flash Memory
Preferences preferences;


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

class Sector {
    public:
        String id;
        String crop;
        float minHumidity;

    Sector() {
        id = "";
        crop = "";
        minHumidity = 0.0;
    }

    Sector(String idValue, String cropValue, float minHumidityValue) {
        id = idValue;
        crop = cropValue;
        minHumidity = minHumidityValue;
    }

};

//----------------------------------------------
//Sensors declaration
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
const int VALVES[] = {PIN_VALVE_1, PIN_VALVE_2, PIN_VALVE_3};

int cantSectors = 0;
Sector sectors[3] = {};

//----------------------------------------------

void setup() {
    Serial.begin(115200);

    initWiFi();

    configPins();
    initializeSensors();

    //Preferences (Flash Memory)
    preferences.begin("sectors", false);
    //Flash File System
    SPIFFS.begin(true);

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
        sendMeasuresToServer();
        previous_time = millis();
    }

    if ((irrigation_current_time - irrigation_previous_time > IRRIGATION_LOOP_TIME)) {
        Serial.println("-----------------------------------");
        Serial.println("Control irrigation");
        irrigationEventResolver();
        irrigation_previous_time = millis();
        retryEvents();
    }
    
}

void initWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    int retryConnection = 0;
    while (WiFi.status() != WL_CONNECTED && retryConnection < 10) {
        Serial.print(".");
        delay(500);
        retryConnection++;
    }
    Serial.print("Connect to the WiFi network with IP: ");
    Serial.println(WiFi.localIP());
}

void configPins() {
    pinMode(PIN_VALVE_1, OUTPUT);
    pinMode(PIN_VALVE_2, OUTPUT);
    pinMode(PIN_VALVE_3, OUTPUT);
    pinMode(PIN_WATER_PUMP, OUTPUT);
    digitalWrite(PIN_VALVE_1, HIGH);
    digitalWrite(PIN_VALVE_2, HIGH);
    digitalWrite(PIN_VALVE_3, HIGH);
    digitalWrite(PIN_WATER_PUMP, HIGH);
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

void sendMeasuresToServer() {
    char body[1024];
    StaticJsonDocument<1024> doc;
    appendJsonObject(doc, tempSensor);
    appendJsonObject(doc, humiditySensor);

    for (Sensor sensorMoisture : soilMoistureArray) {
        appendJsonObject(doc, sensorMoisture);
    }

    for (Sensor sensorTemperature : soilTemperatureArray) {
        appendJsonObject(doc, sensorTemperature);
    }

    serializeJson(doc, body);

    String endpoint = SERVER_URI + "/sensors/" + FARM_ID + "/events";

    if (WiFi.status() == WL_CONNECTED) {
        sendRequest(endpoint.c_str(), body);
    } else {
        Serial.println("Error Posting measures, causes WiFi connection");
        saveRetry(endpoint.c_str(), body);
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
    Serial.println("-----------------------------------");
    Serial.println("Get Sectors info");
    if (WiFi.status() == WL_CONNECTED) {
        String endpoint = SERVER_URI + "/sectors/" + FARM_ID + "/crop-types";
        String sectorsData = getRequest(endpoint.c_str());

        sectorsData.replace("\n", "");
        sectorsData.trim();

        char jsonOutput[1536];
        sectorsData.toCharArray(jsonOutput, 1536);

        Serial.println("SectorsData: " + String(jsonOutput));

        StaticJsonDocument<1536> doc;
        DeserializationError err = deserializeJson(doc, jsonOutput);

        if (err) {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(err.f_str());
        }

        JsonArray sectorsArr = doc.as<JsonArray>();
        
        int i = 0;
        for (JsonObject obj : sectorsArr) {
            String id = obj["id"];
            String cropName = obj["cropType"]["name"];
            float minHumidityValue = obj["cropType"]["parameters"][0]["min"];

            Sector sector = Sector(id, cropName, minHumidityValue);
            sectors[i] = sector;
            i++;
        }
        cantSectors = i;
        preferences.putInt("cantSectors", cantSectors);
    } else {
        Serial.println("Error in WiFi connection");
        cantSectors = preferences.getInt("cantSectors");
    }
}

void irrigationEventResolver() {
    Serial.println("Checking sectors humidity");

    digitalWrite(PIN_WATER_PUMP, HIGH); //Turn off the pump

    boolean hasToActivateWaterPump = false;
    int deactivateWaterPumpVotes = 0;

    for (int i = 0; i < cantSectors; i++) {
        float humidity = soilMoistureArray[i].measure.value;
        Sector sector = sectors[i];

        if(humidity != ERROR_VALUE && humidity < sector.minHumidity) {
            //If the valve is closed open it
            if(digitalRead(VALVES[i]) == HIGH) { 
                digitalWrite(VALVES[i], LOW);
                Serial.println("Valve " + String(i+1) + " opened");
                sendIrrigationEventToServer(sector.id.c_str(), "ON");
            }
            hasToActivateWaterPump = true;
        } else if (humidity == ERROR_VALUE || humidity >= sector.minHumidity) {
            //If the valve is open closed it
            if(digitalRead(VALVES[i]) == LOW) { 
                digitalWrite(VALVES[i], HIGH);
                Serial.println("Valve " + String(i+1) + " closed");
                sendIrrigationEventToServer(sector.id.c_str(), "OFF");
            }
            Serial.println("Humidity from " + sector.id + " is ok.");
        }
    }

    if(waterPumpStatus == "OFF" && hasToActivateWaterPump == true) {
        //Timers check every 2 seconds
        MEASUREMENT_TIME = 2000;
        IRRIGATION_LOOP_TIME = 2000; 
        digitalWrite(PIN_WATER_PUMP, LOW); //turn on water pump
        Serial.println("Water pump on");
        waterPumpStatus = "ON";
    } else if (hasToActivateWaterPump == true) {
        delay(3000);
        digitalWrite(PIN_WATER_PUMP, LOW); //turn it again
        Serial.println("Water pump on again");
    }

    if (waterPumpStatus == "ON" && hasToActivateWaterPump == false) {
        Serial.println("All sectors ok, turn off water pump");
        digitalWrite(PIN_WATER_PUMP, HIGH); //turn it off
        waterPumpStatus = "OFF";
        //reset timers to normal
        MEASUREMENT_TIME = 10000;
        IRRIGATION_LOOP_TIME = 30000;
    }
}

String getRequest(const char* endpoint) {
    HTTPClient client;
    client.begin(endpoint);

    int httpCode = client.GET();

    String response = "{}";

    if (httpCode > 0) {
        Serial.println("HTTP Response code: " +  String(httpCode));
        response = client.getString();
    } else {
        Serial.println("Error on GET Request, error code: " + String(httpCode));
    }

    client.end();

    return response;
}

void sendRequest(const char* endpoint, const char* body) {
    Serial.println("\nSending POST request to " + String(endpoint));
    Serial.println("Body: " + String(body));

    HTTPClient client;
    client.begin(endpoint);
    client.addHeader("Content-Type", "application/json");   

    int httpCode = client.POST(String(body));

    if (httpCode > 0) {
        Serial.println("HTTP Response code: " +  String(httpCode));
    } else {
        Serial.println("Error on POST Request, error code: " + String(httpCode));
    }

    client.end();
}

void sendIrrigationEventToServer(const char* sectorId, const char* waterPumpStatus) {
    char body[1024];
    StaticJsonDocument<1024> doc;
    JsonObject object = doc.to<JsonObject>();
    object["eventType"] = "IrrigationEvent";
    object["sectorId"] = sectorId;
    object["status"] = waterPumpStatus;
    //object["date"] = null;

    serializeJson(doc, body);

    String endpoint = SERVER_URI + "/events/" + FARM_ID;

    if (WiFi.status() == WL_CONNECTED) {
        sendRequest(endpoint.c_str(), body);
    } else {
        Serial.println("Error posting IrrigationEvent, cause WiFi connection");
        saveRetry(endpoint.c_str(), body);
    }
}

void saveRetry(const char* endpoint, const char* body) {
    Serial.println("Save retry event");
    File file = SPIFFS.open("/events.txt", FILE_WRITE);
    file.print(String(endpoint) + "-" + String(body));
    file.close();
}

void retryEvents() {
    if (WiFi.status() == WL_CONNECTED) {
        File file = SPIFFS.open("/events.txt");
        while(file.available()) {
            Serial.println("-----------------------------------");
            Serial.println("Retrying event " + file.read());
            //post to server with endpoint and body
        }
        file.close();
        SPIFFS.remove("/events.txt"); //Delete file when finish to reprocess all lost events
    } else {
        Serial.println("Error retrying events cause WiFi connection");
    } 
}