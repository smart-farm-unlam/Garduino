#define Threshold 40

RTC_DATA_ATTR int counter = 0;

void callback() {

}

void setup() {
    Serial.begin(115200);
    Serial.println("booting");
    ++counter;

    pinMode(2, OUTPUT);
    delay(500);

    digitalWrite(2, HIGH);
    delay(2000);

    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();

    if (ESP_SLEEP_WAKEUP_TOUCHPAD == wakeup_reason) {
        counter = 0;
        Serial.println("reset");
    }

    for(int i=0; i<=counter; i++) {
        digitalWrite(2, LOW);
        delay(200); 
        digitalWrite(2, HIGH);
        delay(200);
    }

    delay(2000);

    //Setup interrumpt on Touch Pad 3 (GPIO13)
    touchAttachInterrupt(T4, callback, Threshold);

    //Configura Touchpad as wakeup source
    esp_sleep_enable_touchpad_wakeup();

    //sleep time in micro seconds!!!
    esp_sleep_enable_timer_wakeup(3000000); //in microseconds
    esp_deep_sleep_start();
}

void loop() {
    
}