# ESP32 + OpenWrt MQTT + Web OTA

在 OpenWrt 路由器上跑 Mosquitto MQTT Broker，讓 ESP32 連上去收發訊息，並透過網頁（ElegantOTA）
遠端更新韌體 —— 不管人在家裡（區網直連）還是在外面（透過 Tailscale）都能一行指令上傳新程式，
不需要每次都插 USB 線。

## 架構總覽

```
你的電腦 ──(Wi-Fi 或 Tailscale)── OpenWrt 路由器(Mosquitto + Tailscale Subnet Router)
                                          │
                                          └──(區網 Wi-Fi)── ESP32(MQTT Client + Web OTA)
```

- **在家**：電腦跟 ESP32 在同一個 Wi-Fi，直接用區網 IP 溝通
- **遠端**：電腦用任何網路（手機熱點等）+ Tailscale，路由器把區網廣播成 Tailscale Subnet Route，
  電腦一樣用 ESP32 的區網 IP 就能連到，不需要額外設定 port forwarding

## 功能

- ESP32 自動連上 WiFi、連上 MQTT Broker
- 每 3 秒發布一次心跳訊息，可訂閱指定 topic 接收指令
- 所有 log 同時輸出到 Serial 跟 MQTT topic（`esp32/debug/log`），不插 USB 也能遠端看 log
- 透過 ElegantOTA 提供網頁版韌體上傳（有帳密保護），支援瀏覽器手動上傳或 `pio run -t upload`
  一行指令自動上傳

## 專案結構

```
.
├── platformio.ini          # PlatformIO 設定（函式庫、編譯、上傳方式）
├── upload_ota.bat          # Windows：一鍵編譯+OTA上傳腳本
├── src/
│   ├── main.cpp             # 主程式
│   └── secrets.example.h    # WiFi/MQTT/OTA帳密 範例檔（複製成 secrets.h 使用）
└── docs/
    ├── 01-openwrt-mosquitto-setup.md   # 路由器安裝設定 Mosquitto
    ├── 02-ssh-into-openwrt.md          # 如何 SSH 進 OpenWrt
    ├── 03-esp32-wifi-mqtt-test.md      # ESP32 WiFi + MQTT 基本測試
    ├── 04-web-ota-setup.md             # 網頁版 OTA（ElegantOTA）設定與踩坑記錄
    └── 05-tailscale-remote-access.md   # 路由器 + 電腦裝 Tailscale，達成遠端OTA
```

## 快速開始

### 1. 環境需求

- [VSCode](https://code.visualstudio.com/) + [PlatformIO 擴充套件](https://platformio.org/install/ide?install=vscode)
- OpenWrt 路由器（已安裝 Mosquitto，見 `docs/01-openwrt-mosquitto-setup.md`）
- ESP32 開發板 + USB 線（第一次燒錄一定要用）
- [MQTT Explorer](http://mqtt-explorer.com/)（推薦，圖形化查看 MQTT 訊息）
- curl（Windows 10/11 內建，注意 PowerShell 裡要打 `curl.exe` 不是 `curl`，見踩坑記錄）

### 2. 設定機密資訊

```bash
cp src/secrets.example.h src/secrets.h
```

打開 `src/secrets.h`，填入你自己的 WiFi 帳密、MQTT Broker IP、OTA 上傳帳密。
這個檔案已被 `.gitignore` 排除，不會被上傳到 GitHub。

### 3. 第一次燒錄（USB）

```bash
pio run -t upload
```

（`platformio.ini` 預設沒有指定 `upload_protocol`，會直接走 USB，PlatformIO 自動抓 COM port）

打開序列埠監控確認：

```bash
pio device monitor
```

應該會看到 WiFi 連線成功、MQTT 連線成功、`Web OTA 已就緒` 等訊息，並附上 OTA 上傳網址。

### 4. 之後改用 Web OTA 更新（不用再插 USB）

**方法 A：瀏覽器手動上傳**

打開 `http://<ESP32的IP>/update`，輸入帳密，選擇 `.pio/build/esp32dev/firmware.bin` 上傳。

**方法 B：一鍵腳本（Windows）**

編輯 `upload_ota.bat`，把 `ESP32_IP`、`OTA_USER`、`OTA_PASS` 改成你自己的設定，之後：

```bash
upload_ota.bat
```

**方法 C：`pio run -t upload` 一行搞定**

把 `platformio.ini` 裡「上傳方式二」那幾行取消註解，填好 IP 跟帳密，之後跟 USB 上傳用同一個指令：

```bash
pio run -t upload
```

詳細原理、遇到的坑（例如為什麼要先呼叫 `/ota/start` 才能呼叫 `/ota/upload`）都寫在
`docs/04-web-ota-setup.md`。

### 5. 遠端（不在家）也能 OTA

需要先讓路由器跟你的電腦都加入同一個 [Tailscale](https://tailscale.com/) 網路，並讓路由器開啟
Subnet Router 廣播區網。設定步驟見 `docs/05-tailscale-remote-access.md`。設定好之後，
不管人在哪，上傳指令都完全一樣，不需要額外調整。

## MQTT Topics

| Topic | 方向 | 說明 |
|---|---|---|
| `esp32/test/pub` | ESP32 → 外部 | 每 3 秒發布一次心跳訊息（含編譯時間戳記，方便確認版本） |
| `esp32/test/sub` | 外部 → ESP32 | 訂閱此 topic，可用來下指令給 ESP32 |
| `esp32/debug/log` | ESP32 → 外部 | 除錯 log，不插 USB 也能看 |

用 MQTT Explorer 連到你的路由器 IP，port 1883，即可即時查看。

## 授權

MIT
