#include <BluetoothSerial.h>

BluetoothSerial SerialBT;

const uint8_t led = 2; // led integrado gpio 2 para mi devkit
const String deviceName = "facu_esp32";
byte dataRx;

/* Check if Bluetooth configurations are enabled in the SDK */
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to enable itimerspec
#endif

void setup()
{
    Serial.begin(115200);
    pinMode(led, OUTPUT);
    digitalWrite(led, LOW);

    SerialBT.begin(deviceName);
    Serial.println(deviceName + " listo!...");
}

void loop()
{
    serialEvent();

    if (dataRx == '1')
        digitalWrite(led, HIGH);
    else if (dataRx == '0')
        digitalWrite(led, LOW);
}

void serialEvent()
{
    if (SerialBT.available())
    {
        dataRx = SerialBT.read();
        Serial.write(dataRx);
    }
}
