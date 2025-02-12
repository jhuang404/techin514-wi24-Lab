#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <FirebaseClient.h>
#include <WiFiClientSecure.h>

#define WIFI_SSID "UW MPSK"
#define WIFI_PASSWORD "6+Ad(nAX}$"

#define DATABASE_SECRET "AIzaSyDsECxTfxnq2T9s-Rr3vSoCp2oomAt4Cwg"
#define DATABASE_URL "https://esp32-battery-lab-default-rtdb.firebaseio.com/"

#define MAX_WIFI_RETRIES 5
#define DISTANCE_THRESHOLD 50.0 // 50 cm threshold
#define TIME_THRESHOLD 10000    // 10 seconds in milliseconds
#define SLEEP_DURATION 20000000 // 20 seconds in microseconds

WiFiClientSecure ssl;
DefaultNetwork network;
AsyncClientClass client(ssl, getNetwork(network));

FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result;
LegacyToken dbSecret(DATABASE_SECRET);

unsigned long sendDataPrevMillis = 0;
unsigned long highDistanceStartTime = 0;
bool highDistanceFlag = false;

const int trigPin = 10;
const int echoPin = 9;
const float soundSpeed = 0.034;
#define UPLOAD_INTERVAL 2000  // 0.5 Hz

void connectToWiFi();
void initFirebase();
void sendDataToFirebase(float distance);
float measureDistance();

void setup() {
    Serial.begin(115200);
    delay(500);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    connectToWiFi();
    initFirebase();
}

void loop() {
    float currentDistance = measureDistance();
    sendDataToFirebase(currentDistance);

    if (currentDistance > DISTANCE_THRESHOLD) {
        if (!highDistanceFlag) {
            highDistanceStartTime = millis();
            highDistanceFlag = true;
        } else if (millis() - highDistanceStartTime >= TIME_THRESHOLD) {
            Serial.println("Distance exceeded 50cm for 10 seconds. Entering deep sleep for 20 seconds...");
            WiFi.disconnect();
            esp_sleep_enable_timer_wakeup(SLEEP_DURATION);
            esp_deep_sleep_start();
        }
    } else {
        highDistanceFlag = false;
    }
    
    delay(UPLOAD_INTERVAL);
}

 
