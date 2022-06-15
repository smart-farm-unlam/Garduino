#include <WebServer.h>

// Creamos nuestra propia red -> SSID & Password
const char* ssid = "ESP32Net";
const char* password = "farm1"; // optional softAP()

// Definimos la IP local (recomendadas por la documentación)
IPAddress ip(192, 168, 4, 22);
IPAddress gateway(192, 168, 4, 9);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

void setup() {
    Serial.begin(115200);

    //Creamos el punto de acceso
    WiFi.softAP(ssid, password); //Tiene más parametros opcionales
    //WiFi.softAPConfig(ip, gateway, subnet);
    IPAddress ip = WiFi.softAPIP();  //-> define una ip el ESP32

    Serial.print("WiFi name: ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(ip);

    server.on("/", handleConnectionRoot);

    server.begin();
    Serial.println("HTTP Server Started");
    delay(150);
}

void loop() {
    server.handleClient();
}

//Respuesta en html
String answer = "<!DOCTYPE html>\
<html>\
<body>\
<h1>Hola desde ESP32 - Modo Punto de Acceso (AP) </h1>\
</body>\
</html>";

//Responder a la url raíz (root /)
void handleConnectionRoot() {
    server.send(200, "text/html", answer);
} 
