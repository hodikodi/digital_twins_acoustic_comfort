#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <WiFi.h>
#include <PubSubClient.h>

// --- NETWORK CONFIGURATION ---
const char* ssid = "Zyxel_8C61";
const char* password = "CFAJ7QHJPM";
const char* mqtt_server = "192.168.43.228";

// --- METER #2 SPECIFIC CONFIGURATION ---
const char* target_meter_mac = "18:90:67:f1:94:f9"; 
const char* mqtt_topic = "room/noise/meter2";

WiFiClient espClient;
PubSubClient mqttClient(espClient);
BLEScan* pBLEScan;

// --- BROADCAST SNIFFER CALLBACK ---
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        // Compare found device MAC with our target MAC
        if (advertisedDevice.getAddress().toString() == target_meter_mac) {
            Serial.print(">>> Meter #2 Found! RSSI: ");
            Serial.println(advertisedDevice.getRSSI());
            
            // Extract the raw hex payload from the advertisement
            String rawHex = BLEUtils::buildHexData(nullptr, advertisedDevice.getPayload(), advertisedDevice.getPayloadLength());
            
            // Look for the 'dBA' ASCII signature (Hex: 644241)
            int dbaIndex = rawHex.indexOf("644241"); 
            
            if (dbaIndex > 8) {
                // The 4 characters (8 hex digits) before 'dBA' are the numeric reading
                String hexDB = rawHex.substring(dbaIndex - 8, dbaIndex);
                String dbValue = "";
                
                for (int i = 0; i < hexDB.length(); i += 2) {
                    char c = (char)strtol(hexDB.substring(i, i+2).c_str(), NULL, 16);
                    if (isDigit(c) || c == '.') dbValue += c;
                }

                if (dbValue.length() > 0) {
                    // Publish to the unique topic for Zone 2
                    mqttClient.publish(mqtt_topic, dbValue.c_str());
                    Serial.println("Sent to Pi [meter2]: " + dbValue + " dBA");
                }
            } else {
                Serial.println("Debug: Meter found but sound data not in this packet.");
            }
        }
    }
};

void setup() {
    Serial.begin(115200);
    pinMode(2, OUTPUT); // Built-in LED for status
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(2, !digitalRead(2)); // Blink while connecting
        delay(500); 
    }
    digitalWrite(2, HIGH); // Solid LED = WiFi Connected
    Serial.println("\nNode #2 WiFi Connected");
    
    mqttClient.setServer(mqtt_server, 1883);
    BLEDevice::init("ESP32_Acoustic_Node_2");
    
    // Configure BLE Scanning
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); 
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99); 
}

void loop() {
    // Ensure MQTT is connected
    if (!mqttClient.connected()) {
        Serial.println("Reconnecting to MQTT...");
        // Use the topic as a unique ClientID to avoid conflicts with Node #1
        if (mqttClient.connect("ESP32_Node_2_Bridge")) {
            Serial.println("MQTT Connected");
        }
    }
    mqttClient.loop();

    // Start 1-second passive scan
    pBLEScan->start(1, false);
    pBLEScan->clearResults(); 
    delay(10);
}