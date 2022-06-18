#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "********";
const char* password = "********";

//Url's para hacer las peticiones
const char* example = "http://example.net/";
//const char* example2 = "http://example.com/sensor1";
const char* duckduckgo = "http://duckduckgo.com/";
//const char* usingIP = "http://192.168.4.1/";

String answer;
const int requestInterval = 7000; //7s


void setup() {
    Serial.begin(115200);

    //nos conectados a la red
    WiFi.begin(ssid, password);
    Serial.println("Connecting");

    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Conectado a la red con la IP: ");
    Serial.print(WiFi.localIP());
    Serial.println();
}

void loop() {
    if(WiFi.status() == WL_CONNECTED) {
        answer = getRequest(example);
        Serial.println("\nRespuesta de example.net");
        Serial.println(answer);

        delay(requestInterval);

        answer = getRequest(duckduckgo);
        Serial.println("\n\nRespuesta de duckduckgo.com:");
        Serial.println(answer);
    }

    while(true) {
        //bucle infinito
    }
     
}

String getRequest(const char* serverName) {
    HTTPClient http;
    http.begin(serverName);

    //Enviamos petición HTTP
    int httpResponseCode = http.GET();

    String payload = "...";

    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        payload = http.getString();
    } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }
    //liberamos
    http.end();

    return payload;
}