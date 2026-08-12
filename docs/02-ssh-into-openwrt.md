# 2. 如何 SSH 進 OpenWrt 路由器

## 前置確認

1. **SSH 服務有開**：OpenWrt 預設開啟 SSH（dropbear），除非手動關過
2. **路由器 IP**：預設常見是 `192.168.1.1`，如果改過 LAN IP 段用你自己設定的
3. **電腦跟路由器在同一個區網**（有線或無線都可以）

## Windows

**方法一：內建 OpenSSH（Windows 10/11 都有）**

打開「終端機」或 PowerShell：

```powershell
ssh root@192.168.1.1
```

第一次連線會問是否信任 host key，輸入 `yes`，接著輸入密碼。

**方法二：PuTTY**

Host Name 填路由器 IP，Port `22`，Connection type 選 SSH，點 Open。

## macOS / Linux

```bash
ssh root@192.168.1.1
```

## 常見問題排查

| 狀況 | 解法 |
|---|---|
| `Connection refused` | SSH 服務沒開或防火牆擋住，先用瀏覽器連路由器IP到LuCI網頁介面，System → Administration 檢查 |
| `Permission denied` | 沒設密碼或密碼錯，先到LuCI網頁介面 System → Administration → 設 root 密碼 |
| host key 警告（換過路由器韌體後常見） | `ssh-keygen -R <路由器IP>` 清掉舊的 known_hosts 紀錄再重連 |
| 忘記路由器IP | 電腦連著這個網路，執行 `ipconfig`（Windows）或 `ip route`（Linux/Mac）看預設閘道 |

連進去後看到類似下面的畫面代表成功：

```
BusyBox v1.xx.x built-in shell (ash)
 _______                     ________        __
|       |.-----.-----.-----.|  |  |  |.----.|  |_
|   -   ||  _  |  -__|     ||  |  |  ||   _||   _|
|_______||   __|_____|__|__||________||__|  |____|
         |__| W I R E L E S S   F R E E D O M
```
