/*
 * ESP32 + MQTT + Web OTA 範例
 *
 * 功能：
 * 1. 連上 WiFi
 * 2. 連上 MQTT Broker（OpenWrt 路由器上的 Mosquitto），定期發布訊息、可訂閱接收指令
 * 3. 提供網頁版 OTA（ElegantOTA），可透過區網或 Tailscale 遠端更新韌體，不需插 USB
 * 4. 除錯訊息同時輸出到 Serial 與 MQTT topic，遠端也能看 log
 *
 * 使用前請先複製 secrets.example.h 為 secrets.h，並填入你自己的 WiFi / MQTT / OTA 資訊
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include "secrets.h"

const char* debug_topic = "esp32/debug/log";
const char* pub_topic   = "esp32/test/pub";
const char* sub_topic   = "esp32/test/sub";

WiFiClient espClient;
PubSubClient client(espClient);
AsyncWebServer server(80);

// 統一的 log 函式：同時印到 Serial（有插USB時看得到）與發布到 MQTT（遠端也能看）
void mqttLog(String msg) {
  Serial.println(msg);
  if (client.connected()) {
    client.publish(debug_topic, msg.c_str());
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "收到訊息 [" + String(topic) + "]: ";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  mqttLog(msg);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("連線 MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("成功");
      client.subscribe(sub_topic);
      mqttLog("ESP32 已連線 MQTT");
    } else {
      Serial.printf("失敗, rc=%d, 5秒後重試\n", client.state());
      delay(5000);
    }
  }
}

void setupOTA() {
  ElegantOTA.begin(&server, OTA_USERNAME, OTA_PASSWORD);   // 幫 /update, /ota/* 加上帳號密碼保護
  server.on("/", [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "ESP32 OTA Server Running");
  });
  server.begin();
  mqttLog("Web OTA 已就緒, 上傳網址: http://" + WiFi.localIP().toString() + "/update");
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi 已連線, IP: " + WiFi.localIP().toString());

  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
  reconnect();

  setupOTA();
}

unsigned long lastMsg = 0;

void loop() {
  ElegantOTA.loop();

  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 3000) {
    lastMsg = now;
    String msg = "hello from esp32, build=" __DATE__ " " __TIME__ ", uptime=" + String(now / 1000);
    client.publish(pub_topic, msg.c_str());
    mqttLog("已發布: " + msg);
  }
}
