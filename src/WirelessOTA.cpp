#include "WirelessOTA.h"

// ---------- WirelessLog ----------

size_t WirelessLog::write(uint8_t c) {
    Serial.write(c);   // 維持原本USB Serial也看得到

    if (c == '\n') {
        _flushLine();
    } else if (c != '\r') {
        _lineBuf += (char)c;
    }
    return 1;
}

size_t WirelessLog::write(const uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size; i++) write(buffer[i]);
    return size;
}

void WirelessLog::_flushLine() {
    if (_lineBuf.length() > 0 && _owner != nullptr) {
        _owner->publish(_topic.c_str(), _lineBuf.c_str());
    }
    _lineBuf = "";
}

// ---------- WirelessOTA ----------

WirelessOTA::WirelessOTA() : _mqttClient(_wifiClient), _server(80) {}

void WirelessOTA::begin(const char* ssid,
                         const char* password,
                         const char* mqttHost,
                         uint16_t mqttPort,
                         const char* otaUsername,
                         const char* otaPassword,
                         const char* mqttClientId,
                         const char* debugTopic) {
    _ssid = ssid;
    _password = password;
    _mqttHost = mqttHost;
    _mqttPort = mqttPort;
    _mqttClientId = mqttClientId;

    log._owner = this;
    log._topic = debugTopic;

    _connectWiFi();

    _mqttClient.setServer(_mqttHost, _mqttPort);
    _connectMQTT();

    // 正確流程：GET /ota/start 初始化 -> POST /ota/upload 上傳，由ElegantOTA內部處理
    if (otaUsername != nullptr && otaPassword != nullptr) {
        ElegantOTA.begin(&_server, otaUsername, otaPassword);
    } else {
        ElegantOTA.begin(&_server);
    }
    _server.begin();

    log.printf("[WirelessOTA] OTA網頁已啟動: http://%s/update\n", WiFi.localIP().toString().c_str());
}

void WirelessOTA::_connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;

    Serial.printf("[WirelessOTA] 連線WiFi: %s\n", _ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(300);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WirelessOTA] WiFi已連線, IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n[WirelessOTA] WiFi連線逾時，稍後loop()會自動重試");
    }
}

void WirelessOTA::_connectMQTT() {
    if (WiFi.status() != WL_CONNECTED) return;
    if (_mqttClient.connected()) return;

    Serial.printf("[WirelessOTA] 連線MQTT broker: %s:%u\n", _mqttHost, _mqttPort);

    if (_mqttClient.connect(_mqttClientId)) {
        Serial.println("[WirelessOTA] MQTT已連線");
        if (_onConnected != nullptr) {
            _onConnected();   // 讓使用者在這裡補做subscribe()
        }
    } else {
        Serial.printf("[WirelessOTA] MQTT連線失敗, rc=%d\n", _mqttClient.state());
    }
}

void WirelessOTA::loop() {
    if (WiFi.status() != WL_CONNECTED && millis() - _lastWifiCheck > 5000) {
        _lastWifiCheck = millis();
        _connectWiFi();
    }

    if (!_mqttClient.connected() && millis() - _lastMqttAttempt > 5000) {
        _lastMqttAttempt = millis();
        _connectMQTT();
    }

    _mqttClient.loop();
    ElegantOTA.loop();
}

bool WirelessOTA::publish(const char* topic, const char* payload, bool retained) {
    if (!_mqttClient.connected()) return false;
    return _mqttClient.publish(topic, payload, retained);
}

bool WirelessOTA::subscribe(const char* topic) {
    if (!_mqttClient.connected()) return false;
    return _mqttClient.subscribe(topic);
}

void WirelessOTA::onMessage(MqttMessageCallback callback) {
    _mqttClient.setCallback(callback);
}

void WirelessOTA::onConnected(MqttConnectedCallback callback) {
    _onConnected = callback;
}

bool WirelessOTA::isWifiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool WirelessOTA::isMqttConnected() {
    return _mqttClient.connected();
}

AsyncWebServer& WirelessOTA::server() {
    return _server;
}
