@echo off
REM 使用前請把下面三個變數改成你自己的設定
REM ESP32_IP：你的 ESP32 目前的區網 IP（在家用區網 IP；遠端用 Tailscale 時填同一個區網IP，
REM           但要確保路由器已開啟 Tailscale Subnet Router，詳見 docs/05-tailscale-remote-access.md）
REM OTA_USER / OTA_PASS：要跟 src/secrets.h 裡的 OTA_USERNAME / OTA_PASSWORD 一致

set ESP32_IP=192.168.1.132
set OTA_USER=admin
set OTA_PASS=change_this_password

echo 編譯中...
call pio run
if %errorlevel% neq 0 (
    echo 編譯失敗！
    exit /b 1
)

echo 初始化 OTA...
curl.exe -s -u %OTA_USER%:%OTA_PASS% "http://%ESP32_IP%/ota/start"

echo.
echo 上傳韌體...
curl.exe -u %OTA_USER%:%OTA_PASS% -F "file=@.pio/build/esp32dev/firmware.bin" "http://%ESP32_IP%/ota/upload"

echo.
echo 完成！
