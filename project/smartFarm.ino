//------------------------EXTERNAL LIBRARIES------------------------
#include <DHT.h>
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <ESP32Time.h>

using namespace std;

//------------------------CONSTANTS------------------------
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
const int PIN_VALVE_ANTI_FROST = 18;
const int PIN_WATER_PUMP = 21;
const int PIN_LDR = 36;
const int PIN_LED_ESP = 2;

//Sensor configuration
DHT dht(PIN_DHT, DHT22);
OneWire oneWire(PIN_DS18B20);
DallasTemperature soilTemperatureSensors(&oneWire);
DeviceAddress first_thermometer, second_thermometer, third_thermometer;

//Flash preferences
Preferences preferences;

//RTC (Real Time Clock)
ESP32Time rtc;
boolean rtc_sync = false;

//Time values
int FIVE_SECONDS = 5000;
int TEN_SECONDS = 10000;
int THIRTY_SECONDS = 30000;
int ONE_MINUTE = 60000;
int TWO_MINUTES = 2 * ONE_MINUTE;
int FIVE_MINUTES = 5 * ONE_MINUTE;
int TEN_MINUTES = 10 * ONE_MINUTE;

//Software timer
int MEASUREMENT_TIME = THIRTY_SECONDS;
unsigned long measurenment_current_time, measurenment_previous_time;

int IRRIGATION_LOOP_TIME = ONE_MINUTE;
unsigned long irrigation_current_time, irrigation_previous_time;

int ANTI_FROST_LOOP_TIME = TWO_MINUTES;
unsigned long anti_frost_current_time, anti_frost_previous_time;

int RTC_LOOP_TIME = TEN_MINUTES;
unsigned long rtc_current_time, rtc_previous_time;

int CHECK_WIFI_LOOP_TIME = FIVE_MINUTES;
unsigned long check_wifi_current_time, check_wifi_previous_time;

//Server path
String SERVER_URI = String("http://192.168.1.50:8080");
//String SERVER_URI = "https://smartfarmunlam.azurewebsites.net";
String TIME_URL_URI = "https://worldtimeapi.org/api/timezone/America/Argentina/Buenos_Aires";

//FARM ID
String FARM_ID = "62acc77270752f2b6bbb88ee";

//Values
const double ERROR_VALUE = -99.0;
//DHT22
float temp = ERROR_VALUE;
float hum = ERROR_VALUE;
//Capacitive soil moisture limit
const int AIR_VALUE = 3800;
const int WATER_VALUE = 1250;

//ZERO DEGRESS
const float ZERO_DEGRESS = 0.0;

//------------------------STRUCTS------------------------
typedef struct {
    float value;
    String dateTime;
} Measure;

typedef struct {
    String code;
    Measure measure;
} Sensor;

typedef struct {
    String id;
    String crop;
    float minHumidity;
} Sector;

//------------------------SENSORS DECLARATION------------------------
//Sensors declaration
Sensor tempSensor = {"AT1", ERROR_VALUE};
Sensor humiditySensor = {"AH1", ERROR_VALUE};
Sensor soilMoistureSensor1 = {"SH1", ERROR_VALUE};
Sensor soilMoistureSensor2 = {"SH2", ERROR_VALUE};
Sensor soilMoistureSensor3 = {"SH3", ERROR_VALUE};
Sensor soilTempSensor1 = {"ST1", ERROR_VALUE};
Sensor soilTempSensor2 = {"ST2", ERROR_VALUE};
Sensor soilTempSensor3 = {"ST3", ERROR_VALUE};
Sensor ldrSensor = {"LL1", ERROR_VALUE};

Sensor soilMoistureArray[] = {soilMoistureSensor1, soilMoistureSensor2, soilMoistureSensor3}; 
Sensor soilTemperatureArray[] = {soilTempSensor1, soilTempSensor2, soilTempSensor3}; 

//Irrigation system
String waterPumpStatus = "OFF";
const int IRRIGATION_VALVES[] = {PIN_VALVE_1, PIN_VALVE_2, PIN_VALVE_3};

int sectorsCount = 0;
Sector sectors[3] = {};

//AntiFrostSystem
String antiFrostSystem = "OFF";

//------------------------MAIN PROGRAM------------------------

void setup() {
    Serial.begin(115200);

    //Pins configuration
    configPins();

    //Initialize sensors
    initializeSensors();

    //Preferences (Flash Memory)
    preferences.begin("smartFarm", false);
    //Flash File System
    SPIFFS.begin(true);

    //Connect to WiFi
    configWiFi();  

    //Sync RTC Clock
    syncRTC();

    //Retrieved data from server
    getSectorsInfo();

    //Close preferences
    preferences.end();
    
    //software timer init
    initTimers();
}

void loop() {
    long current_time = millis();
    measurenment_current_time = current_time;
    irrigation_current_time = current_time;
    anti_frost_current_time = current_time;
    rtc_current_time = current_time;
    check_wifi_current_time = current_time;

    if ((measurenment_current_time - measurenment_previous_time) > MEASUREMENT_TIME) {
        Serial.println("-----------------------------------");
        Serial.println("Start collecting data from sensors");
        measuresResolver();
        measurenment_previous_time = millis();
    }

    if ((irrigation_current_time - irrigation_previous_time) > IRRIGATION_LOOP_TIME && antiFrostSystem == "OFF") {
        Serial.println("-----------------------------------");
        Serial.println("Control irrigation");
        //irrigationEventResolver();
        irrigation_previous_time = millis();
    }

    if ((anti_frost_current_time - anti_frost_previous_time) > ANTI_FROST_LOOP_TIME) {
        Serial.println("-----------------------------------");
        Serial.println("Control antifrost system");
        //antiFrostEventResolver();
        anti_frost_previous_time = millis();
        //Also use this timer to retry lost events
        retryEvents();
    }

    if ((rtc_current_time - rtc_previous_time) > RTC_LOOP_TIME) {
        //Synchonized RTC clock with server
        syncRTC();
        getSectorsInfo();
        rtc_previous_time = millis();
    }

    if ((check_wifi_current_time - check_wifi_previous_time) > CHECK_WIFI_LOOP_TIME) {
        //Check if Wifi is disconnected and reconnect
        checkWiFi();
        check_wifi_previous_time = millis();
    }

}

//------------------------INIT FUNCTIONS------------------------
void initTimers() {
    measurenment_current_time = millis();
    measurenment_previous_time = millis();

    irrigation_current_time = millis();
    irrigation_previous_time = millis();

    anti_frost_current_time = millis();
    anti_frost_previous_time = millis();

    rtc_current_time = millis();
    rtc_previous_time = millis();

    check_wifi_current_time = millis();
    check_wifi_previous_time = millis();
}

void configPins() {
    pinMode(PIN_VALVE_1, OUTPUT);
    pinMode(PIN_VALVE_2, OUTPUT);
    pinMode(PIN_VALVE_3, OUTPUT);
    pinMode(PIN_VALVE_ANTI_FROST, OUTPUT);
    pinMode(PIN_WATER_PUMP, OUTPUT);
    pinMode(PIN_LED_ESP, OUTPUT);

    //Turn off all rele
    digitalWrite(PIN_VALVE_1, HIGH);
    digitalWrite(PIN_VALVE_2, HIGH);
    digitalWrite(PIN_VALVE_3, HIGH);
    digitalWrite(PIN_VALVE_ANTI_FROST, HIGH);
    digitalWrite(PIN_WATER_PUMP, LOW); //This rele is HIGH LEVEL TRIGGER
}

void syncRTC() {
    Serial.println("-----------------------------------");
    Serial.println("Synchronizing clock");
    if (WiFi.status() == WL_CONNECTED) {
        String endpoint = TIME_URL_URI;
        String responseRTC = getRequest(endpoint.c_str());

        if (responseRTC != "{}") {
            Serial.println("RTC updated successfully");
            RTC_LOOP_TIME = TEN_MINUTES;
            rtc_sync = true;
        } else {
            Serial.println("RTC failed to synchronized");
            RTC_LOOP_TIME = FIVE_SECONDS;
            reconnectWiFi();
            rtc_sync = false;
            return;
        }

        responseRTC.replace("\n", "");
        responseRTC.trim();

        char jsonOutput[768];
        strcpy(jsonOutput, responseRTC.c_str());

        Serial.println("JsonOutput: " + String(jsonOutput));

        StaticJsonDocument<768> doc;
        DeserializationError err = deserializeJson(doc, jsonOutput);

        if (err) {
            Serial.print("deserializeJson() failed with code ");
            Serial.println(err.c_str());
            return;
        }

        long unixtime = doc["unixtime"];

        rtc.setTime(unixtime);

        String dateTime = getDateTime();

        Serial.println("DateTime: " + String(dateTime));

    } else {
        Serial.println("Error in WiFi connection, synchronized clock fail");
    }
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

String getDateTime() {
    char buff[25];
    time_t now = time(NULL);
    strftime(buff, 25, "%FT%TZ", localtime(&now));
    return buff;
}

//------------------------------RESOLVERS------------------------------------
void measuresResolver() {
    readTemperatureAndHumidity();
    readSoilMoistureSensors();
    readSoilTemperatureSensors();
    readLightValue();
    sendMeasuresToServer();
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

    String dateTime = getDateTime();

    tempSensor.measure.value = temp;
    tempSensor.measure.dateTime = dateTime;
    humiditySensor.measure.value = hum;
    humiditySensor.measure.dateTime = dateTime;
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
    Serial.print("Soil Humidity value " + String(index) + ": " + String(soilMoistureValue));
    Serial.println(" (" + String(soilMoistureValuePercent) + "%)");
    soilMoistureArray[index-1].measure.value = soilMoistureValuePercent;
    soilMoistureArray[index-1].measure.dateTime = getDateTime();
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
  Serial.println("Soil Temperature " + String(index) + ": " + String(tempC, 1) + "°C");
  soilTemperatureArray[index-1].measure.value = tempC;
  soilTemperatureArray[index-1].measure.dateTime = getDateTime();
}

void readLightValue() {
    int lightValue = analogRead(PIN_LDR);
    int lightValuePercent = map(lightValue, 0, 4095, 0, 100);

    Serial.print("Light value: " + String(lightValue));
    Serial.println(" (" + String(lightValuePercent) + "%)");

    ldrSensor.measure.value = lightValuePercent;
    ldrSensor.measure.dateTime = getDateTime();
}

void irrigationEventResolver() {
    Serial.println("Checking sectors humidity");

    // digitalWrite(PIN_WATER_PUMP, LOW); //Turn off the pump
    boolean hasToActivateWaterPump = false;

    for (int i = 0; i < sectorsCount; i++) {
        float humidity = soilMoistureArray[i].measure.value;
        Sector sector = sectors[i];

        if(humidity != ERROR_VALUE && humidity < sector.minHumidity) {
            //If the valve is closed open it
            if(digitalRead(IRRIGATION_VALVES[i]) == HIGH) { 
                digitalWrite(IRRIGATION_VALVES[i], LOW);
                Serial.println("Valve " + String(i+1) + " opened");
                sendIrrigationEventToServer(sector.id.c_str(), "ON");
            }
            hasToActivateWaterPump = true;
        } else if (humidity == ERROR_VALUE || humidity >= sector.minHumidity) {
            //If the valve is open closed it
            if(digitalRead(IRRIGATION_VALVES[i]) == LOW) { 
                digitalWrite(IRRIGATION_VALVES[i], HIGH);
                Serial.println("Valve " + String(i+1) + " closed");
                sendIrrigationEventToServer(sector.id.c_str(), "OFF");
            }
            Serial.println("Humidity from " + sector.id + " is ok.");
        }
    }

    if(waterPumpStatus == "OFF" && hasToActivateWaterPump == true) {
        //Timers check every 10 seconds
        MEASUREMENT_TIME = TEN_SECONDS;
        IRRIGATION_LOOP_TIME = TEN_SECONDS; 
        digitalWrite(PIN_WATER_PUMP, HIGH); //turn on water pump
        Serial.println("Water pump on");
        waterPumpStatus = "ON";
    }

    if (waterPumpStatus == "ON" && hasToActivateWaterPump == false) {
        Serial.println("All sectors ok, turn off water pump");
        digitalWrite(PIN_WATER_PUMP, LOW); //turn it off
        waterPumpStatus = "OFF";
        //reset timers to normal
        MEASUREMENT_TIME = THIRTY_SECONDS;
        IRRIGATION_LOOP_TIME = ONE_MINUTE;
    }
}

void antiFrostEventResolver() {
    Serial.println("Checking ambient temperature");

    float temperature = tempSensor.measure.value;
    
    if (temperature != ERROR_VALUE && temperature < ZERO_DEGRESS) {
        if(antiFrostSystem == "OFF") {
            closeIrrigationValves();
            digitalWrite(PIN_VALVE_ANTI_FROST, LOW);    //open the anti_frost valve
            digitalWrite(PIN_WATER_PUMP, HIGH);         //turn on water pump
            Serial.println("Water pump on");
            waterPumpStatus = "ON";

            Serial.println("AntiFrost system activated");
            antiFrostSystem = "ON";
            sendAntiFrostEventToServer("ON");
        } else {
            Serial.println("Temperature is still below 0°C, AntiFrost system working");
        }
        
    } else if (temperature == ERROR_VALUE || temperature >= ZERO_DEGRESS) {
        //If valve is open closed it
        if(digitalRead(PIN_VALVE_ANTI_FROST) == LOW) {
            digitalWrite(PIN_VALVE_ANTI_FROST, HIGH);   //close the anti_frost valve
            digitalWrite(PIN_WATER_PUMP, LOW);         //turn off water pump
            Serial.println("Water pump off");
            waterPumpStatus = "OFF";

            Serial.println("AntiFrost system deactivated");
            antiFrostSystem = "OFF";
            sendAntiFrostEventToServer("OFF");
        }
        Serial.println("Ambient temperature is ok.");
    }
    
}

//Close all irrigation valves cause we are using antifrost system
void closeIrrigationValves() {
    for (int i = 0; i < sectorsCount; i++) {
        //If valve if open is because it started an irrigation event for that sector,
        //so we have to notify that irrigation event off to the server
        if(digitalRead(IRRIGATION_VALVES[i] == LOW)) {
            Sector sector = sectors[i];
            digitalWrite(IRRIGATION_VALVES[i], HIGH); //turn off valve
            sendIrrigationEventToServer(sector.id.c_str(), "OFF");
        }
    }
}

//------------------------HTTP REQUEST------------------------

void getSectorsInfo() {
    Serial.println("-----------------------------------");
    Serial.println("Get Sectors info");
    if (WiFi.status() == WL_CONNECTED) {
        String endpoint = SERVER_URI + "/sectors/" + FARM_ID + "/crop-types";
        String sectorsData = getRequest(endpoint.c_str());

        if(sectorsData == "{}") {
            getSectorsFromFlashMem();
            return;
        }

        sectorsData.replace("\n", "");
        sectorsData.trim();

        char jsonOutput[1536];
        sectorsData.toCharArray(jsonOutput, 1536);

        Serial.println("SectorsData: " + String(jsonOutput));

        StaticJsonDocument<1536> doc;
        DeserializationError err = deserializeJson(doc, jsonOutput);

        if (err) {
            Serial.print("deserializeJson() failed with code ");
            Serial.println(err.c_str());
            return;
        }

        JsonArray sectorsArr = doc.as<JsonArray>();
        
        int i = 0;
        for (JsonObject obj : sectorsArr) {
            String id = obj["id"];
            String cropName = obj["cropType"]["name"];
            float minHumidityValue = obj["cropType"]["parameters"][0]["min"];

            Sector sector = {id, cropName, minHumidityValue};
            sectors[i] = sector;
            i++;
        }
        sectorsCount = i;
        preferences.putBytes("sectors", sectors, sizeof(sectors));
        preferences.putInt("sectorsCount", sectorsCount);
        Serial.println("Persist sectors info on flash memory");
    } else {
        Serial.println("Error in WiFi connection");
        getSectorsFromFlashMem();
    }
}

void sendMeasuresToServer() {
    char body[1024];
    StaticJsonDocument<1024> doc;
    appendSensorJsonObject(doc, tempSensor);
    appendSensorJsonObject(doc, humiditySensor);
    appendSensorJsonObject(doc, ldrSensor);

    for (Sensor sensorMoisture : soilMoistureArray) {
        appendSensorJsonObject(doc, sensorMoisture);
    }

    for (Sensor sensorTemperature : soilTemperatureArray) {
        appendSensorJsonObject(doc, sensorTemperature);
    }

    serializeJson(doc, body);

    String endpoint = SERVER_URI + "/sensors/" + FARM_ID + "/events";

    if (WiFi.status() == WL_CONNECTED) {
        int result = sendRequest(endpoint.c_str(), body);
        if (result != 200) {
            saveRetry(endpoint.c_str(), body);
        }
    } else {
        Serial.println("Error Posting measures, cause WiFi connection");
        saveRetry(endpoint.c_str(), body);
    }
}

void appendSensorJsonObject(StaticJsonDocument<1024> &doc, Sensor sensor) {
    JsonObject sensorDoc = doc.createNestedObject();
    sensorDoc["code"] = sensor.code;

    JsonObject measure = sensorDoc.createNestedObject("measure");
    measure["value"] = sensor.measure.value;
    if(rtc_sync == true) {
        measure["dateTime"] = sensor.measure.dateTime;
    }
}

void sendIrrigationEventToServer(const char* sectorId, const char* status) {
    char body[1024];
    StaticJsonDocument<1024> doc;
    JsonObject object = doc.to<JsonObject>();
    object["eventType"] = "IrrigationEvent";
    object["sectorId"] = sectorId;
    object["status"] = status;
    object["date"] = getDateTime();

    serializeJson(doc, body);

    String endpoint = SERVER_URI + "/events/" + FARM_ID;

    if (WiFi.status() == WL_CONNECTED) {
        int result = sendRequest(endpoint.c_str(), body);
        if (result != 200) {
            saveRetry(endpoint.c_str(), body);
        }
    } else {
        Serial.println("Error posting IrrigationEvent, cause WiFi connection");
        saveRetry(endpoint.c_str(), body);
    }
}

void sendAntiFrostEventToServer(const char* antiFrostSystemStatus) {
    char body[1024];
    StaticJsonDocument<1024> doc;
    JsonObject object = doc.to<JsonObject>();
    object["eventType"] = "AntiFrostEvent";
    object["status"] = antiFrostSystemStatus;
    object["date"] = getDateTime();

    serializeJson(doc, body);

    String endpoint = SERVER_URI + "/events/" + FARM_ID;

    if (WiFi.status() == WL_CONNECTED) {
        int result = sendRequest(endpoint.c_str(), body);
        if (result != 200) {
            saveRetry(endpoint.c_str(), body);
        }
    } else {
        Serial.println("Error posting IrrigationEvent, cause WiFi connection");
        saveRetry(endpoint.c_str(), body);
    }
}

//HTTP CLIENT IMPLEMENTATION
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

int sendRequest(const char* endpoint, const char* body) {
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

    return httpCode;
}

//------------------------GET FROM FLASH MEMORY------------------------


void getSectorsFromFlashMem() {
    Serial.println("Getting sectors info from flash");
    size_t sectorsLenght = preferences.getBytesLength("sectors");
    char buffer[sectorsLenght];
    preferences.getBytes("sectors", buffer, sectorsLenght);
    memcpy(sectors, buffer, sectorsLenght);

    sectorsCount = preferences.getInt("sectorsCount");
    Serial.println("\nSectors count: " + String(sectorsCount));
    for (int i = 0; i < sectorsCount; i++) {
        Serial.println("\nSector " + String(i+1));
        Serial.println("Id: " + sectors[i].id);
        Serial.println("Crop: " + sectors[i].crop);
        Serial.println("Minhumidity: " + String(sectors[i].minHumidity));
    }
}

//------------------------WIFI CONFIGURATION------------------------
void configWiFi() {
    //Delete old config
    WiFi.disconnect(true);

    WiFi.onEvent(WiFiStationConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFiGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);

    //TODO remove ssid and password
    //WIFI Settings
    String ssid = preferences.getString("ssid", "Movistar14");
    String password = preferences.getString("password", "mate2306");

    if (ssid != "" || password != "") {
        WiFi.begin(ssid.c_str(), password.c_str());
        Serial.println("Connecting to WiFi...");
        int retries = 0;
        int maxRetries = 10;
        while (WiFi.status() != WL_CONNECTED && retries < maxRetries) {
            delay(500);
            retries++;
        }
        delay(TEN_SECONDS);
    } else { 
        Serial.println("No values saved for ssid or password");
    }
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("Connected to WiFi successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.print("WiFi connected with IP: ");
    Serial.println(WiFi.localIP());
    digitalWrite(PIN_LED_ESP, HIGH);
    delay(2000);
    digitalWrite(PIN_LED_ESP, LOW);    
}

void checkWiFi() {
    if (WiFi.status() == WL_DISCONNECTED) {
        reconnectWiFi();
    }
}

void reconnectWiFi() {
    Serial.println("Trying to Reconnect WiFi");
    WiFi.reconnect();
}

//------------------------RETRY SCHEMA------------------------
void saveRetry(const char* endpoint, const char* body) {
    Serial.println("Save retry event");
    File file = SPIFFS.open("/events.txt", FILE_APPEND);
    file.print(String(endpoint) + "\n");
    file.print(String(body) + "\n");
    file.close();
}

void retryEvents() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Retrying lost events");
        File file = SPIFFS.open("/events.txt");
        vector<String> v;
        while(file.available()) {
            v.push_back(file.readStringUntil('\n'));
        }
        file.close();

        //Retry events
        String endpoint;
        String body;
        for (int i = 0; i < v.size(); i=i+2) {
            if (i % 2 == 0) {
                endpoint = v[i];
                body = v[i+1];
                sendRequest(endpoint.c_str(), body.c_str());
            }
        }

        SPIFFS.remove("/events.txt"); //Delete file when finish to reprocess all lost events
    } else {
        Serial.println("Error retrying events cause WiFi connection");
    } 
}
