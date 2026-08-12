# 1. 在 OpenWrt 上安裝與設定 Mosquitto MQTT Broker

## 安裝

SSH 進路由器後（見 `02-ssh-into-openwrt.md`）：

```bash
opkg update
opkg install mosquitto-ssl   # 支援TLS的完整版
# 或
opkg install mosquitto-nossl # 輕量版，不支援TLS
```

啟動並設定開機自動啟動：

```bash
/etc/init.d/mosquitto enable
/etc/init.d/mosquitto start
```

## 設定檔

OpenWrt 用 UCI 系統管理設定，設定檔在 `/etc/config/mosquitto`，語法跟一般 Linux 的
`mosquitto.conf` 不同。範例（測試階段先允許匿名連線，允許區網訪問）：

```
config owrt 'owrt'
        option use_uci '1'
config mosquitto 'mosquitto'
        option no_remote_access '0'
        option allow_anonymous '1'
config persistence 'persistence'
config listener
        option port '1883'
```

重點欄位說明：

- `no_remote_access '0'` → **0 代表允許遠端（區網）連線**，預設值，不用特別改
- `allow_anonymous '1'` → 允許匿名連線，測試階段方便，正式環境建議關閉並改用帳密（見下方）

改完設定檔後要重啟服務套用：

```bash
/etc/init.d/mosquitto restart
```

## 確認 Broker 正常監聽

```bash
netstat -lnp | grep 1883
```

正常應該看到類似：

```
tcp   0   0   0.0.0.0:1883   0.0.0.0:*   LISTEN   xxxx/mosquitto
```

**重點**：左邊 IP 必須是 `0.0.0.0`，如果是 `127.0.0.1` 代表只監聽本機，區網裝置（ESP32）連不到。

## 查詢路由器 LAN IP

```bash
uci get network.lan.ipaddr
```

這個 IP 之後會填進 ESP32 程式的 `MQTT_SERVER`。

## （選用）加上帳號密碼驗證

測試階段建議先用匿名連線，確認整條鏈路都通了之後，再考慮加帳密：

```bash
touch /etc/mosquitto/pwfile
mosquitto_passwd -b /etc/mosquitto/pwfile <帳號> <密碼>
```

`/etc/config/mosquitto` 的 `listener` 段落加上：

```
option password_file '/etc/mosquitto/pwfile'
list allow_anonymous '0'
```

重啟服務套用：

```bash
/etc/init.d/mosquitto restart
```

## 從電腦端測試 Broker

**用 MQTT Explorer（推薦，圖形化）**

新增連線：Host 填路由器 LAN IP，Port `1883`，若匿名連線帳密留空，Encryption 不勾。連線成功後
可以直接在介面上發布/訂閱訊息測試。

**或用指令列**（需要先安裝 mosquitto-clients：Windows 到 [官網](https://mosquitto.org/download/)
下載、Mac 用 `brew install mosquitto`、Linux 用 `apt install mosquitto-clients`）：

```bash
# 訂閱（視窗一）
mosquitto_sub -h <路由器LAN_IP> -t "test/#" -v

# 發布（視窗二）
mosquitto_pub -h <路由器LAN_IP> -t "test/hello" -m "ping"
```

視窗一收到 `test/hello ping` 就代表 Broker 運作正常。

## 防火牆

OpenWrt 預設防火牆通常 LAN 內部裝置互通沒問題，不需額外設定。若真的連不到，確認一下：

```bash
uci show firewall | grep -i 1883
```
