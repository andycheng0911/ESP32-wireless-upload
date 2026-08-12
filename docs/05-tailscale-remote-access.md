# 5. 用 Tailscale 達成遠端 OTA（不在家也能上傳韌體）

## 架構概念

ESP32 沒辦法安裝 Tailscale，所以要讓「路由器」代替它加入 Tailscale 網路，並開啟
**Subnet Router** 功能，把整個區網（例如 `192.168.1.0/24`）廣播給你的 Tailnet。這樣你的
電腦（裝了 Tailscale）不管在哪，都能像在家一樣直接用 ESP32 的區網 IP 連過去。

```
你的電腦(裝Tailscale) --Tailscale隧道--> 路由器(裝Tailscale, 廣播區網) --區網--> ESP32
```

## 路由器端：安裝 Tailscale 並廣播 Subnet Route

### 安裝

```bash
opkg update
opkg install tailscale
```

### 設定廣播路由

```bash
tailscale up --advertise-routes=192.168.1.0/24
```

把 `192.168.1.0/24` 換成你實際的區網段。

用 UCI 檢查設定：

```bash
uci show tailscale
```

應該會看到類似：

```
tailscale.settings.advertise_routes='192.168.1.0/24'
```

### 到 Tailscale Admin Console 手動核准路由

Tailscale 預設**需要手動核准**廣播出來的 subnet route，光靠指令 `--advertise-routes` 還不夠。

1. 開啟 [Tailscale Admin Console](https://login.tailscale.com/admin/machines)
2. 找到你的路由器那台機器
3. 點進去的設定裡會有 **Subnet routes** 區塊，把對應的網段（如 `192.168.1.0/24`）打勾核准

沒核准的話，即使指令有設定，電腦端還是連不到區網裝置。

## 電腦端驗證

暫時斷開電腦原本的 Wi-Fi / 有線網路（改用手機熱點之類的外部網路，確保不在家用區網），
確保 Tailscale 還連著，測試：

```powershell
ping <ESP32的區網IP>
```

Ping 得通（延遲會比在家直連高一些，這是正常的，因為封包繞了 Tailscale 隧道），
代表 Subnet Router 設定成功，遠端也能連到 ESP32。

## 常見坑：在家時 Tailscale 反而干擾了原本的直連路徑

如果電腦同時「在家」又「開著 Tailscale」，可能會發現連線變慢（延遲從 1~5ms 變成
80~100ms 以上），這是因為 Windows 的路由表可能被導向 Tailscale 虛擬網卡，而不是直接走
Wi-Fi。如果你**確定人在家**，想強制走 Wi-Fi 直連（比較快），可以手動加一條路由規則：

系統管理員 PowerShell：

```powershell
route add <ESP32的IP> mask 255.255.255.255 <路由器IP> metric 1
```

這條規則指定「去 ESP32 的封包一律走這個閘道」，不受 Tailscale 路由表影響。

**注意**：如果人真的在外面（不在家），不要下這條規則，或是要拿掉，因為這時候你的電腦
根本不在區網網段裡，強制指定的閘道反而連不通，這時候應該讓封包正常走 Tailscale 隧道。

## 排查用：確認封包實際走的路徑

```powershell
tracert <ESP32的IP>
```

第一跳（hop 1）如果不是你路由器的區網IP，代表封包被導去別的介面卡了（常見於同時開著
VPN 或虛擬網卡如 VMware 的情況）。

## 為什麼選 HTTP-based OTA（ElegantOTA）而不是 ArduinoOTA

詳見 `04-web-ota-setup.md`，簡單說：ArduinoOTA 需要 ESP32 主動「回撥」連到電腦，這在
NAT / Tailscale 環境下幾乎不可能成功；ElegantOTA 走單向 HTTP 上傳，電腦主動連過去，
不需要 ESP32 反向連線，天生適合這種跨網路的遠端情境。
