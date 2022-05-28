#define SENSOR A0

void setup() {

}

void loop() {
    int valorLuz = analogRead(SENSOR);
    Serial.println("Luz: " + String(valorLuz));
    delay(1000);
}