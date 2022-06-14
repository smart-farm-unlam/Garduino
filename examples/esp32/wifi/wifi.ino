#include <WiFi.h>

const char* ssid = "Movistar14";
const char* password = "mate2306";

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }

    Serial.println("\nConnected to the WiFi network");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop() {
    if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
        Serial.println("You can try to ping me");
        delay(5000);
    } else {
        Serial.println("Connection lost");
    }

}

