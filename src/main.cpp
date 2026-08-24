/*
 * ESP32 + MQTT + Web OTA 範例（套件版，print自動同步到MQTT）
 */

#include <WirelessOTA.h>
#include "secrets.h"

const char* pub_topic = "esp32/test/pub";
const char* sub_topic = "esp32/test/sub";

WirelessOTA wireless;

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String msg = "收到訊息 [" + String(topic) + "]: ";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  wireless.log.println(msg);   // 直接用log印，自動同時發到MQTT debug topic
}

// 每次MQTT連線成功（含斷線重連）都會呼叫一次，在這裡補訂閱
void onMqttConnected() {
  wireless.subscribe(sub_topic);
  wireless.log.println("ESP32 已連線 MQTT");
}

void setup() {
  Serial.begin(115200);

  wireless.onMessage(onMqttMessage);
  wireless.onConnected(onMqttConnected);

  wireless.begin(
  WIFI_SSID,
  WIFI_PASSWORD, 
  MQTT_SERVER, 
  MQTT_PORT,
  OTA_USERNAME, 
  OTA_PASSWORD,
  DEVICE_NAME   // mqttClientId，debugTopic不用傳了，自動變成 "esp32-livingroom/debug/log"
);

}

unsigned long lastMsg = 0;

void loop() {
  wireless.loop();

  unsigned long now = millis();
  if (now - lastMsg > 3000) {
    lastMsg = now;
    String msg = "hello from esp32, build=" __DATE__ " " __TIME__ ", uptime=" + String(now / 1000);
    wireless.publish(pub_topic, msg.c_str());
    wireless.log.println("已發布: " + msg);
  }
}
