int RELE = 2;

void setup() {
    pinMode(RELE, OUTPUT); //inicializa el rele como salida
}

void loop() {
    digitalWrite(RELE, LOW);
    delay(5000);
    digitalWrite(RELE, HIGH);
    delay(5000);
}