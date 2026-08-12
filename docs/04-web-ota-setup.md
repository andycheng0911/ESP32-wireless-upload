# 4. 網頁版 OTA（ElegantOTA）設定與踩坑記錄

## 為什麼不用 ArduinoOTA，改用 ElegantOTA？

一開始嘗試用 Arduino 內建的 `ArduinoOTA`（也就是 PlatformIO 的 `upload_protocol = espota`），
在**同一個區網**下沒問題，但只要電腦不在同一個 Wi-Fi（例如用手機熱點 + Tailscale 遠端連線），
就會一直失敗，顯示 `No response from device`。

**原因**：ArduinoOTA 是雙向交握協定 —— 電腦先送 UDP 邀請封包給 ESP32，ESP32 收到後要**反過來
主動連回**電腦的 IP:Port 上傳資料。但 ESP32 只認識區網（`192.168.1.0/24`），完全不知道電腦在
Tailscale（`100.x.x.x`）或其他 NAT 後面的位址，導致「回撥」這一步永遠失敗。

**解法**：改用 **ElegantOTA**，走 HTTP 單向上傳 —— 電腦主動連到 ESP32 的網頁伺服器上傳檔案，
ESP32 完全不需要「回撥」，天生對 NAT / Tailscale 友善。

## 安裝函式庫

`platformio.ini`：

```ini
lib_deps =
    ayushsharma82/ElegantOTA@^3.1.6
    https://github.com/ESP32Async/AsyncTCP.git
    https://github.com/ESP32Async/ESPAsyncWebServer.git

build_flags =
    -D ELEGANTOTA_USE_ASYNC_WEBSERVER=1
```

### 踩坑1：`lib_deps` 寫成 `AsyncTCP` 抓錯套件

如果 `lib_deps` 只寫 `AsyncTCP`（沒有指定來源），PlatformIO 從註冊表比對名稱時可能抓到
**`AsyncTCP_RP2040W`**（樹莓派 Pico W 專用版本），完全不適用 ESP32，會出現一堆
`RASPBERRY_PI_PICO_W` 相關的編譯錯誤。**解法**：直接寫完整 GitHub 網址指定明確的來源，
如上面範例所示。

### 踩坑2：`HTTP_GET` / `HTTP_POST` 等巨集衝突

如果沒加 `-D ELEGANTOTA_USE_ASYNC_WEBSERVER=1`，ElegantOTA 預設編譯成同步版（用內建的
`WebServer`），但專案裡用的是 `AsyncWebServer`，兩邊的 HTTP method 定義會互相衝突，
出現大量 `'HTTP_GET' conflicts with a previous declaration` 錯誤。加上這個 build flag 後，
ElegantOTA 會改用支援 `AsyncWebServer*` 的 `begin()` 版本。

## 程式碼

```cpp
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

AsyncWebServer server(80);

void setup() {
  // ... WiFi、MQTT 連線 ...

  ElegantOTA.begin(&server, OTA_USERNAME, OTA_PASSWORD);  // 加帳密保護
  server.begin();
}

void loop() {
  ElegantOTA.loop();   // 一定要放在 loop() 裡持續呼叫
  // ...
}
```

## 上傳方式一：瀏覽器手動上傳

先用 USB 燒錄一次含 ElegantOTA 的程式，之後打開瀏覽器連到：

```
http://<ESP32的IP>/update
```

輸入帳密，選擇 `.pio/build/esp32dev/firmware.bin`，點上傳即可。

## 上傳方式二：指令列自動化（curl）

### 踩坑3：PowerShell 的 `curl` 不是真正的 curl

在 PowerShell 裡直接打 `curl`，其實會被解析成 `Invoke-WebRequest` 的別名，參數語法完全
不同，不支援 `-F` 這種 curl 專屬參數，會報錯 `找不到符合參數名稱 'F' 的參數`。
**解法**：明確加上 `.exe`，強制呼叫 Windows 內建的真正 curl 執行檔：

```powershell
curl.exe -F "file=@firmware.bin" http://<ESP32的IP>/ota/upload
```

### 踩坑4：不能只打 `/update`，也不能只打 `/ota/upload`

ElegantOTA v3 實際的上傳端點是 **`/ota/upload`**（`POST`），`/update` 只是顯示網頁介面用的
（`GET`）。但單獨呼叫 `/ota/upload` 上傳，伺服器雖然會回應 `200 OK`，**資料其實沒有真的
寫入**，因為缺少初始化這一步 —— 必須先呼叫 **`/ota/start`**（`GET`），讓 ElegantOTA
內部執行 `Update.begin()` 做初始化，之後 `/ota/upload` 才會真正生效並在完成後觸發重啟。

正確的兩段式流程：

```powershell
# 第一步：初始化
curl.exe -s -u <帳號>:<密碼> "http://<ESP32的IP>/ota/start"

# 第二步：真正上傳檔案
curl.exe -u <帳號>:<密碼> -F "file=@.pio/build/esp32dev/firmware.bin" "http://<ESP32的IP>/ota/upload"
```

如果有加密碼保護（見下方），兩段都要帶 `-u 帳號:密碼`，否則會被要求驗證失敗。

## 加上帳密保護

`/update`、`/ota/start`、`/ota/upload` 預設完全公開，只要知道 IP 任何人都能上傳韌體。
建議一定要加密碼：

```cpp
ElegantOTA.begin(&server, "your_username", "your_password");
```

## 整合進 `pio run -t upload`

`platformio.ini`：

```ini
upload_protocol = custom
upload_command = curl.exe -s -u <帳號>:<密碼> "http://<ESP32的IP>/ota/start" && curl.exe -u <帳號>:<密碼> -F "file=@$SOURCE" "http://<ESP32的IP>/ota/upload"
```

之後不管要不要遠端，都只要：

```bash
pio run -t upload
```

`$SOURCE` 會被 PlatformIO 自動替換成編譯出的 `firmware.bin` 完整路徑。

### 踩坑5：PlatformIO 增量編譯有時候沒偵測到程式碼變更

改了 `main.cpp` 存檔後執行 `pio run`，理論上會重新編譯，但實務上遇過檔案時間戳記比對出問題
導致直接沿用舊的 `.pio/build/.../firmware.bin`，上傳的其實是舊版本。**判斷方式**：比對
`main.cpp` 跟 `firmware.bin` 的最後修改時間：

```powershell
Get-Item src\main.cpp | Select-Object LastWriteTime
Get-Item .pio\build\esp32dev\firmware.bin | Select-Object LastWriteTime
```

如果 `firmware.bin` 的時間比 `main.cpp` 舊，代表沒有重新編譯，先清快取再重編：

```bash
pio run -t clean
pio run
```

這也是為什麼範例程式裡的心跳訊息加了 `__DATE__ __TIME__`（編譯時間戳記）—— 每次编译都
是唯一值，能一眼確認 ESP32 上跑的版本是不是最新編譯的，不用再靠肉眼比對容易混淆的文字內容。

### 踩坑6：獨立開的終端機視窗可能沒有 PlatformIO 的 PATH

如果是另外開一個系統管理員 PowerShell（不是 VSCode 內建終端機）執行 `pio` 系列指令，可能會
出現 `無法辨識 'pio' 詞彙`。**建議直接用 VSCode 內建終端機**（`Ctrl+反引號`），PlatformIO
擴充套件會自動設定好環境，不需要額外處理 PATH。
