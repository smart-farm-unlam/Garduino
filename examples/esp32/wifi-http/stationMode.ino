#include <WebServer.h>
#include <WiFi.h>

const char* ssid = "********";
const char* password = "********";

WebServer server(80); //puerto por defecto 80

void setup() {
    Serial.begin(115200);
    Serial.println("Trying to connect to " + String(ssid));

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    //método para gestionar las consultas o peticiones a la URL
    server.on("/", responseRequest);
    
    server.begin();
    Serial.println("HTTP Server started");
    delay(100);

}

void loop() {
    server.handleClient();
}

String response = "<h1> Hola desde ESP32 </h1>";

void responseRequest() {
    server.send(200, "text/html", response);
}