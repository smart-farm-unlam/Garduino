//Serial Peripheral Interface Flash File System
#include "SPIFFS.h"

void setup() {
    Serial.begin(115200);

    SPIFFS.begin(true);

    getAllFiles();   
}

void loop() {
    
}

void readFile() {
    File file = SPIFFS.open("/subscribe.txt");
    while(file.available()) {
        Serial.write(file.read());
    }
    file.close();
    delay(1000);
}

void writeFile() {
    File file = SPIFFS.open("/subscribe.txt", FILE_WRITE);
    file.print("Subscribe to my channel \n");
    file.close();
}

void getAllFiles() {
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    
    while (file) {
        Serial.print("FILE: ");
        Serial.print(file.name());
        file = root.openNextFile();
    }
}

void deleteFile() {
    SPIFFS.remove("/subscribe.txt");
}

void setupWithValidation() {
    Serial.begin(115200);

    if(!SPIFFS.begin(true)) {
        Serial.println("Error: mounting SPIFFS");
        return;
    }

    File file = SPIFFS.open("/subscribe.txt", FILE_WRITE);

    if(!file) {
        Serial.println("Error: opening file");
        return;
    }

    if(file.print("Subscribe to my channel \n")) {
        Serial.println("File was written");
    } else {
        Serial.println("File write failed");
    }

    file.close();
}
