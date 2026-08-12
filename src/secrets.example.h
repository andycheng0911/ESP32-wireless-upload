// secrets.h 範例檔
// 使用方式：複製這份檔案並改名為 secrets.h，填入你自己的實際資訊
// secrets.h 已被 .gitignore 排除，不會被上傳到 GitHub，請放心填寫真實密碼

#ifndef SECRETS_H
#define SECRETS_H

// ---- WiFi 設定 ----
#define WIFI_SSID     "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// ---- MQTT Broker 設定（OpenWrt 路由器上的 Mosquitto）----
#define MQTT_SERVER   "192.168.1.1"   // 路由器 LAN IP
#define MQTT_PORT     1883

// ---- ElegantOTA 網頁上傳保護（建議設定，避免任何人都能上傳韌體）----
#define OTA_USERNAME  "admin"
#define OTA_PASSWORD  "change_this_password"

#endif
