int PIN_RELE = 5;

void setup() {
    pinMode(PIN_RELE, OUTPUT); //inicializa el rele como salida
}

void loop() {
    digitalWrite(PIN_RELE, LOW);
    delay(5000);
    digitalWrite(PIN_RELE, HIGH);
    delay(5000);
}