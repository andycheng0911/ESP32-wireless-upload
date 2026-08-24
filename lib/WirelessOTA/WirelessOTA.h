#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

class WirelessOTA;

// 用法跟 Serial 完全一樣（print/println/printf），
// 但每次遇到換行，會把這一行同時發布到 MQTT debug topic，遠端也能看log
class WirelessLog : public Print {
public:
    size_t write(uint8_t c) override;
    size_t write(const uint8_t* buffer, size_t size) override;

    WirelessOTA* _owner = nullptr;
    String _topic = "esp32/debug/log";

private:
    String _lineBuf;
    void _flushLine();
};

// MQTT 收到訊息時的callback型別
typedef void (*MqttMessageCallback)(char* topic, byte* payload, unsigned int length);
// 每次MQTT (重新)連線成功時會呼叫一次，適合在這裡做subscribe()
typedef void (*MqttConnectedCallback)();

class WirelessOTA {
public:
    WirelessOTA();

    // otaUsername/otaPassword 傳 nullptr 表示OTA網頁不設密碼保護
    // mqttClientId：MQTT連線識別碼，多台裝置一定要取不同名字，不然會互相把對方擠下線
    // debugTopic：wireless.log 印出的每一行會發布到這個topic。傳nullptr（預設）
    //             會自動用 "<mqttClientId>/debug/log"，多台裝置的log topic自動分開，不用手動拼字串
    void begin(const char* ssid,
               const char* password,
               const char* mqttHost,
               uint16_t mqttPort = 1883,
               const char* otaUsername = nullptr,
               const char* otaPassword = nullptr,
               const char* mqttClientId = "esp32",
               const char* debugTopic = nullptr);

    // 放在 loop() 裡呼叫，處理WiFi/MQTT自動重連 + MQTT收發 + OTA
    void loop();

    // topic 為完整字串，不會自動加任何前綴
    bool publish(const char* topic, const char* payload, bool retained = false);
    bool subscribe(const char* topic);

    void onMessage(MqttMessageCallback callback);
    // 每次MQTT連線成功（含重連）都會呼叫一次，適合在裡面呼叫subscribe()
    void onConnected(MqttConnectedCallback callback);

    bool isWifiConnected();
    bool isMqttConnected();

    // 讓使用者可以自己在 server 上加額外路由（例如首頁）
    AsyncWebServer& server();

    // 跟 Serial 用法一樣，但同時會發布到 MQTT，例如：wireless.log.println("hi");
    WirelessLog log;

private:
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;
    AsyncWebServer _server;

    const char* _ssid = nullptr;
    const char* _password = nullptr;
    const char* _mqttHost = nullptr;
    uint16_t _mqttPort = 1883;
    const char* _mqttClientId = "esp32";

    MqttConnectedCallback _onConnected = nullptr;

    unsigned long _lastWifiCheck = 0;
    unsigned long _lastMqttAttempt = 0;

    void _connectWiFi();
    void _connectMQTT();
};
