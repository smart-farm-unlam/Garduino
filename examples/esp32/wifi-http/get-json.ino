#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char *ssid = "Movistar14";
const char *password = "mate2306";

void setup()
{
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println("\nConnected to the WiFi network");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop()
{
    if ((WiFi.status() == WL_CONNECTED)) // Check the current connection status
    {
        long rnd = random(1, 10);
        HTTPClient client;

        client.begin("http://jsonplaceholder.typicode.com/comments?id=" + String(rnd));
        int httpCode = client.GET();

        if (httpCode > 0)
        {
            String payload = client.getString();
            Serial.println("\nStatus code: " + String(httpCode));
            Serial.println(payload);

            char json[500];
            payload.replace(" ", "");
            payload.replace("\n", "");
            payload.trim();
            payload.remove(0, 1);
            payload.toCharArray(json, 500);

            StaticJsonDocument<200> doc;
            deserializeJson(doc, json);

            int id = doc["id"];
            const char* email = doc["email"];

            Serial.println(String(id) + " - " + String(email) + "\n");

            client.end();
        }
        else
        {
            Serial.println("Error on HTTP request");
        }
    }
    else
    {
        Serial.println("Connection lost");
    }

    delay(10000);
}
