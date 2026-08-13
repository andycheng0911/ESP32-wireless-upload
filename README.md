# WirelessOTA

ESP32 的 WiFi + MQTT + Web OTA 通用套件。新專案只要 `#include <WirelessOTA.h>`，
給 WiFi 帳密和 MQTT broker IP，就能直接用，不用每次重寫一次連線、重連、OTA的邏輯。

## 安裝

新專案的 `platformio.ini` 加一行：

```ini
lib_deps =
    https://github.com/andycheng0911/ESP32-wireless-upload.git
```


PlatformIO 會自動抓這個套件，以及它宣告的相依函式庫（AsyncTCP、ESPAsyncWebServer、ElegantOTA、PubSubClient）。

## 最小範例

新專案的 `src/main.cpp`：

```cpp
#include <WirelessOTA.h>
#include "secrets.h"   // 自己建立，見下方「secrets.h」說明

WirelessOTA wireless;

void setup() {
  Serial.begin(115200);
  wireless.begin(WIFI_SSID, WIFI_PASSWORD, MQTT_SERVER, MQTT_PORT,
                  OTA_USERNAME, OTA_PASSWORD);
}

void loop() {
  wireless.loop();                      // 一定要放，處理WiFi/MQTT自動重連 + OTA
  wireless.log.println("hello esp32");  // 跟Serial.println用法一樣，同時發布到MQTT
  delay(3000);
}
```

## secrets.h

每個使用這個套件的專案，自己在專案的 `src/` 裡建立 `secrets.h`（記得加進 `.gitignore`，
不要上傳到GitHub），內容：

```cpp
#ifndef SECRETS_H
#define SECRETS_H

#define WIFI_SSID     "你的WiFi名稱"
#define WIFI_PASSWORD "你的WiFi密碼"

#define MQTT_SERVER   "192.168.1.1"   // 路由器上Mosquitto的IP
#define MQTT_PORT     1883

#define OTA_USERNAME  "admin"              // OTA網頁的登入帳密，避免任何人都能上傳韌體
#define OTA_PASSWORD  "change_this_password"

#endif
```

WiFi帳密、MQTT IP完全不寫在套件裡，套件本身保持通用、不會外洩任何一台裝置的個人資料。

## API

### `wireless.begin(...)`

```cpp
void begin(const char* ssid,
           const char* password,
           const char* mqttHost,
           uint16_t mqttPort = 1883,
           const char* otaUsername = nullptr,   // 傳nullptr表示OTA網頁不設密碼
           const char* otaPassword = nullptr,
           const char* mqttClientId = "esp32",  // MQTT連線用的client id，多台裝置要取不同名字
           const char* debugTopic = "esp32/debug/log");  // wireless.log會發布到這個topic
```

### `wireless.loop()`

放在 `loop()` 最前面呼叫，處理WiFi斷線重連、MQTT斷線重連、MQTT收發、OTA。

### `wireless.log`

跟 `Serial` 用法完全一樣（`print` / `println` / `printf`），差別是每寫完一整行
（遇到換行）會自動把那一行同步發布到 `debugTopic`，USB Serial也照樣看得到。
不用再自己寫 `mqttLog()` 這種輔助函式。

```cpp
wireless.log.println("開機完成");
wireless.log.printf("溫度: %.1f\n", temperature);
```

用 `mosquitto_sub -h <MQTT_IP> -t esp32/debug/log` 就能在電腦上即時看遠端裝置的log，
不需要接USB。

### `wireless.publish(topic, payload, retained=false)` / `wireless.subscribe(topic)`

topic是完整字串，不會自動加任何前綴，自己決定topic命名規則。

```cpp
wireless.publish("esp32/test/pub", "hello");
wireless.subscribe("esp32/test/sub");
```

### `wireless.onMessage(callback)`

註冊收到MQTT訊息時的處理函式：

```cpp
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  // ...
}
wireless.onMessage(onMqttMessage);
```

### `wireless.onConnected(callback)`

每次MQTT連線成功（含斷線重連）都會呼叫一次，適合在裡面呼叫 `subscribe()`，
確保重連後訂閱不會遺失：

```cpp
void onMqttConnected() {
  wireless.subscribe("esp32/test/sub");
}
wireless.onConnected(onMqttConnected);
```

### `wireless.isWifiConnected()` / `wireless.isMqttConnected()`

回傳目前連線狀態的bool。

## OTA更新（遠端，不需接USB）

網頁固定在 `http://<裝置IP>/update`，有設 `otaUsername`/`otaPassword` 的話會跳出登入視窗。

也可以用 PlatformIO 自訂上傳指令，直接按 Upload 就走網路 OTA，不用打開瀏覽器手動上傳。
`platformio.ini` 加：

```ini
upload_protocol = custom
upload_command = curl.exe -s -u admin:你的OTA密碼 http://192.168.1.132/ota/start && curl.exe -u admin:你的OTA密碼 -F "file=@$SOURCE" http://192.168.1.132/ota/upload
```

**重點**：
- 一定要先 `GET /ota/start` 初始化，再 `POST /ota/upload` 上傳，只呼叫 `/ota/upload` 會回 200
  但實際沒寫入flash。
- 有設 OTA 帳密保護的話，兩個請求都要帶 `-u 帳號:密碼`，沒帶會收到401，但curl預設不會讓指令
  失敗、畫面仍會顯示「上傳成功」——這是誤判，裝置其實沒有真的套用新韌體。加 `curl -f`（fail on
  HTTP error）可以避免誤判。
- 因為帳密直接寫在 `platformio.ini` 裡，這個檔案若要公開分享，記得改成非敏感密碼，或把
  `platformio.ini` 也排除在版本控制外。

## 專案內部結構（套件維護者看這段）

```
ESP32-wireless-upload/
├── library.json          <- 套件身分證，build.srcDir指向lib/WirelessOTA
├── lib/
│   └── WirelessOTA/
│       ├── WirelessOTA.h
│       └── WirelessOTA.cpp
├── src/
│   ├── main.cpp           <- 這個repo自己作為範例專案使用套件的方式
│   └── secrets.h           (已gitignore)
└── docs/                   <- 路由器/Mosquitto/Tailscale設定文件
```

套件本體放在 `lib/WirelessOTA/`，不是 `src/`，是為了避免外部專案透過 `lib_deps` 抓這個repo
當套件時，把 `src/main.cpp`（連同它 include 的、不存在的 `secrets.h`）也一起誤判成套件原始碼
去編譯，導致編譯失敗。`library.json` 裡的 `"build": {"srcDir": "lib/WirelessOTA"}` 明確告訴
PlatformIO 只用這個資料夾當套件來源。
