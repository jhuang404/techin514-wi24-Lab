#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 1000;

// HC-SR04 sensor pins
#define TRIG_PIN 2
#define ECHO_PIN 6

// Filter variables for moving average
#define MOVING_AVERAGE_SIZE 10
float distanceReadings[MOVING_AVERAGE_SIZE];
int readIndex = 0;  // Renamed from 'index' to 'readIndex'
float sum = 0;
float filteredDistance = 0;

// TODO: Change the UUIDs to your own (generated UUIDs)
#define SERVICE_UUID        "12cf3b38-02dd-47c5-ae13-60e4eafba7ca"
#define CHARACTERISTIC_UUID "332ffc9d-29a3-4b0b-8d86-95b7a746caed"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    // Set up the HC-SR04 pins
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    // Initialize BLE device
    BLEDevice::init("XIAO_ESP32c3_hjl");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("Hello World");
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

float readDistance() {
    // Send pulse to HC-SR04
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Read the echo pin and calculate distance in cm
    long duration = pulseIn(ECHO_PIN, HIGH);
    float distance = (duration / 2.0) * 0.0343; // Speed of sound is 0.0343 cm/µs
    return distance;
}

void addToMovingAverage(float newDistance) {
    sum -= distanceReadings[readIndex];  // Renamed from 'index' to 'readIndex'
    distanceReadings[readIndex] = newDistance;
    sum += newDistance;
    readIndex = (readIndex + 1) % MOVING_AVERAGE_SIZE;  // Renamed from 'index' to 'readIndex'
    filteredDistance = sum / MOVING_AVERAGE_SIZE;
}

void loop() {
    float rawDistance = readDistance();
    
    // Print the raw distance for debugging
    Serial.print("Raw Distance: ");
    Serial.println(rawDistance);

    // Apply the moving average filter
    addToMovingAverage(rawDistance);

    // Print the filtered (denoised) distance for debugging
    Serial.print("Filtered Distance: ");
    Serial.println(filteredDistance);

    if (deviceConnected) {
        if (filteredDistance < 35.0) {
            pCharacteristic->setValue(filteredDistance);
            pCharacteristic->notify();
            Serial.print("Notifying: ");
            Serial.println(filteredDistance);
        }
    }

    // Manage BLE connection state
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising(); // advertise again
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }

    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    delay(1000);
}
