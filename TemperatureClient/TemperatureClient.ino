#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>

// Data wire is connected to GPIO4
#define ONE_WIRE_BUS 5

// Setup oneWire instance to communicate with OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass oneWire reference to Dallas Temperature library
DallasTemperature sensors(&oneWire);

// WiFi login
const char* ssid = "adel";
const char* wifipass = "adel1234";

// Goqtt server location
const char* server_ip = "140.238.215.2";
const uint16_t server_port = 8080;
const char* client_id = "Sjever 2";

// client
WiFiClient client;

void setup() {
  Serial.begin(115200);
  Serial.println("DS18B20 Temperature Sensor Example");

  WiFi.begin(ssid, wifipass);
  Serial.print("Connecting to WiFi ....");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");

  sensors.begin();

  Serial.print("Connection to server ...");
  while(!client.connect(server_ip, server_port)) {
    Serial.println("Connection failed, retrying in 5 seconds ...");
    delay(5000);
  }

  Serial.println("Connected to server!");
  client.println(String("ClientID: ") + client_id);
}

void loop() {
  if (!client.connected()) {
    Serial.println("Lost connection to server. Reconnecting...");
    
    while (!client.connect(server_ip, server_port)) {
      Serial.println("Reconnection failed, retrying in 5 seconds...");
      delay(5000);
    }

    Serial.println("Reconnected to server!");
    client.println(String("ClientID: ") + client_id);
  }

  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);

  if (temperatureC != DEVICE_DISCONNECTED_C) {
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["type"] = "PUB";
    jsonDoc["topic"] = "temp/zivinice";
    jsonDoc["value"] = String(temperatureC, 2);
    jsonDoc["qos"] = 0;

    String jsonString;
    serializeJson(jsonDoc, jsonString);

    client.print(jsonString);
    String response = client.readString();
    Serial.println("Sent: " + jsonString);
    Serial.println("Response: " + response);
  } else {
    Serial.println("Error: Could not read temperature data");
  }

  delay(5000);
}