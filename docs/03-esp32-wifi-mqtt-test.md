# 3. ESP32 WiFi + MQTT 基本測試

## PlatformIO 專案設定

`platformio.ini` 最簡版（USB 燒錄用）：

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    knolleary/PubSubClient@^2.8
```

**注意**：不要自己加 `upload_protocol = esp`（曾經誤植的錯誤設定），PlatformIO 不認得這個值，
會顯示 `Warning! Unknown upload protocol esp`，導致編譯「看起來成功」但其實完全沒有燒錄進去，
序列埠monitor就會完全沒反應。USB 燒錄不需要指定 `upload_protocol`，留空讓它自動偵測即可。

## 燒錄與監控

```bash
pio run -t upload
pio device monitor
```

正常應該依序看到：

```
.....
WiFi 已連線, IP: 192.168.1.xxx
連線 MQTT...成功
已發布: hello from esp32, uptime=3
```

如果序列埠monitor完全沒反應：
- 確認 upload 那步真的有出現 `Hard resetting via RTS pin...`（代表真的燒錄成功），
  而不是只顯示編譯 `[SUCCESS]`
- 按一下開發板上的 **EN/RST** 按鈕重啟
- 確認 USB 線是傳輸線，不是只能充電的線

## 常見連線問題對照

| 序列埠monitor卡在哪 | 可能原因 |
|---|---|
| 一直印 `.....` 連不上WiFi | SSID/密碼錯，或該SSID是純5GHz頻段（ESP32只支援2.4GHz，要確認路由器有廣播2.4GHz的SSID） |
| WiFi連上但 MQTT 連線失敗，rc=-2 | MQTT Broker IP 或 port 錯，或 Broker 沒開 |
| rc=-4 或連線逾時 | 防火牆或網路不通 |

## 用 MQTT Explorer 驗證雙向通訊

連線設定：Host 填路由器 LAN IP，Port `1883`，帳密視你的 Broker 設定而定。

連線成功後：
- 左側 topic tree 會自動長出 `esp32/test/pub`，每 3 秒更新一次
- 在發布區塊填 `esp32/test/sub`，發個訊息，回頭看序列埠monitor應該會印出收到的內容
