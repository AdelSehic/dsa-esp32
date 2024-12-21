#include <Arduino.h>
#include <U8g2lib.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define SDA_PIN 12 //GPIO12 / 6
#define SCL_PIN 14 //GPIO14 / 5
#define PING_INTERVAL 45

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, SCL_PIN, SDA_PIN, U8X8_PIN_NONE);

// Buffer for JSON parsing
StaticJsonDocument<200> jsonDoc;
String sub_msg, keepalive;

// WiFi
const char* ssid = "adel";
const char* pass = "adel1234";
const char* server_ip = "140.238.215.2";
const uint16_t server_port = 8080;
const char* client_id = "temperature_display";
char readbuf[2048];

// client
WiFiClient client;
unsigned long lastPingTime = 0;

void connect_and_sub() {
  Serial.print("Connecting to server... ");
  while(!client.connect(server_ip, server_port)) {
    Serial.println("Connection failed, retrying in 5 seconds ...");
    delay(2500);
  }

  Serial.println("Connected to server!");
  client.println(String("ClientID: ") + client_id);
  String response = client.readString();
  Serial.println(String("Server response: ") + response);
  
  client.print(sub_msg);
  response = client.readString();
  Serial.println(String("Server response: ") + response);

}

void send_keepalive() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastPingTime < PING_INTERVAL * 1000) {
    return;
  }
  lastPingTime = currentMillis;
  client.print(keepalive);
  Serial.println("Sent keepalive msg");
}

void setup(void) {
  u8g2.begin();
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting!");

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("\r\nWiFi connected!");

  StaticJsonDocument<200> temp_json;
  temp_json["type"] = "LIVE";
  serializeJson(temp_json, keepalive);
  
  temp_json["type"] = "SUB";
  temp_json["topic"] = "temp/#";
  serializeJson(temp_json, sub_msg);
}

void loop(void) {
  if (!client.connected()) {
    connect_and_sub();
  }
  send_keepalive();

  int bytesRead = client.read((uint8_t*)readbuf, sizeof(readbuf) - 1);
  if (bytesRead <= 0) {
    // Serial.println("Nothing to do");
    delay(100);
    return;
  }
  readbuf[bytesRead] = '\0';
  Serial.print("Recieved:" );
  Serial.println(readbuf);

  DeserializationError error = deserializeJson(jsonDoc, readbuf);
  if (error) {
    Serial.print("JSON Parsing failed: ");
    Serial.println(error.c_str());
    return;
  } else {
    Serial.println("JSON Parsed!");
  }

  const char* device = jsonDoc["sender"];
  const char* topic = jsonDoc["topic"];
  float value = jsonDoc["value"];

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_VCR_OSD_mn);
  u8g2.setCursor(0, 16);
  u8g2.print(value, 2);

  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.drawStr(0,32,device);
  u8g2.drawStr(0,48,topic);

  u8g2.sendBuffer();
  delay(50);
}