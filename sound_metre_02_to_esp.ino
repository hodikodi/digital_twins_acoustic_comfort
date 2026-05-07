#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <WiFi.h>
#include <PubSubClient.h>

// --- CONFIGURATION ---
const char* ssid = "Zyxel_8C61";
const char* password = "CFAJ7QHJPM";
const char* mqtt_server = "192.168.43.228";

// TARGET FOR SOLDERED ESP
const char* target_mac = "e8:d0:3c:51:d5:8c"; 
const char* mqtt_topic = "room/noise/meter1"; // MUST MATCH PI SCRIPT

WiFiClient espClient;
PubSubClient mqttClient(espClient);
BLEScan* pBLEScan;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (advertisedDevice.getAddress().toString() == target_mac) {
            String rawHex = BLEUtils::buildHexData(nullptr, advertisedDevice.getPayload(), advertisedDevice.getPayloadLength());
            int dbaIndex = rawHex.indexOf("644241"); 
            
            if (dbaIndex > 8) {
                String hexDB = rawHex.substring(dbaIndex - 8, dbaIndex);
                String dbValue = "";
                for (int i = 0; i < hexDB.length(); i += 2) {
                    char c = (char)strtol(hexDB.substring(i, i+2).c_str(), NULL, 16);
                    if (isDigit(c) || c == '.') dbValue += c;
                }

                if (dbValue.length() > 0) {
                    Serial.println("M1 Broadcasting: " + dbValue + " dBA");
                    mqttClient.publish(mqtt_topic, dbValue.c_str());
                }
            }
        }
    }
};

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  
  mqttClient.setServer(mqtt_server, 1883);
  BLEDevice::init("ESP32_Node_M1"); // Unique Client ID

  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); 
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); 
}

void loop() {
  if (!mqttClient.connected()) {
    mqttClient.connect("ESP32_Node_M1"); 
  }
  mqttClient.loop();
  pBLEScan->start(1, false);
  pBLEScan->clearResults(); 
  delay(10);
}