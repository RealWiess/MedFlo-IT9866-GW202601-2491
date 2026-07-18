# MedFlow Gateway (ITE9868) 系統規格書 (System Specifications)

本文件紀錄目前 Gateway 的核心運作規格、參數設定與行為準則。隨著開發與除錯的進行，此文件將持續更新。

**開發者在進行任何新功能開發或架構修改前，必須優先閱讀此文件、[`DEVELOPMENT_LOG.md`](./DEVELOPMENT_LOG.md) (研發日誌)，並查閱 Git 歷史紀錄，確保修改不會破壞既有規格。**

---

## 1. 硬體規格

| 項目 | 規格 |
|------|------|
| **主控晶片** | ITE IT9860（相容 IT9866 / IT9868） |
| **CPU** | FA626（ARM 核心，792 MHz） |
| **作業系統** | OpenRTOS（基於 FreeRTOS） |
| **Wi-Fi / BT 模組** | Realtek RTL8821CS（SDIO 介面，WiFi + BLE 單晶片共用天線） |
| **WiFi 驅動** | RTL8821C (CFG_NET_WIFI_SDIO_NGPL_8821CS, VND_RTK) |
| **藍牙協議棧** | Apache NimBLE (CFG_BUILD_NIMBLE, CFG_RTL8821BLE) |
| **USB** | CDC-ACM（USB 虛擬串口，VCP） |
| **儲存** | NOR Flash（Quad-SPI / AXISPI 4 線模式） |
| **顯示器** | 480×480，RGB565 16-bit |
| **觸控** | Goodix GT911（I2C 介面） |

---

## 2. 系統識別碼 (Gateway ID / GWID)

* **規格**：Gateway 統一使用其 **WiFi 晶片的 MAC 位址** 作為全域唯一識別碼。
* **格式**：`"0000"` + 12 位大寫 hex MAC（無冒號）。例：MAC `38:7A:CC:46:52:5B` → GWID `0000387ACC46525B`
* **讀取方式**：直接從 LwIP `xnetif[0].hwaddr` 記憶體結構讀取（非阻塞）。
* **Cached**：`bt_wifi_get_mac()` 在首次成功讀取後快取，`s_mac_ready = true`，之後不再重讀。
* **安全規則**：**嚴禁**在輪詢迴圈中使用 `ioctl(ITP_DEVICE_WIFI, ...)` 查詢網路狀態，這會阻塞底層 SDIO 匯流排並引發當機。

---

## 3. WiFi 設定

### 3.1 連線參數

| 參數 | 值 | 說明 |
|------|-----|------|
| 模式 | Client (STA) | theConfig.wifi_mode = 0 |
| SSID | `wiess-2.4G` | 來自 ctrlboard.ini |
| 安全模式 | `secumode=4` (WPA2-AES) | ctrlboard.ini 數值，由 `WifiMgr_Secu_ITE_To_RTL()` 轉換為 RTL enum |
| secumode 對照 | 0=OPEN, 4=WPA2-AES, 6=WPA2-Mixed, 7=WPA/WPA2-Mixed | ctrlboard.ini `[wifi]` section |
| DHCP | 啟用 | theConfig.dhcp = y |
| WiFi 初始化延遲 | 5 秒 | `Network_Time_Delay` |
| 開機自動連線延遲 | 10 秒 | `boot_connect_delay = 10` (確保晶片暫存器穩定) |
| 🔴 secumode 空值陷阱 | 空值 → `strtol("")` = 0 → OPEN mode | WPA2 AP 會拒絕連線，必須明確設定數值 |
| 🔴 WifiMgr_Sta_Connect NULL | `strtol(NULL)` = UB | 必須傳入 `theConfig.secumode`，不可傳 NULL |

### 3.2 重連機制

| 參數 | 值 | 說明 |
|------|-----|------|
| Driver 自動重連 | **關閉** | `WifiMgr_Disable_Auto_Reconnect(1)`，由應用層控制 |
| 最大重試次數 | 20 | `WIFI_MAX_RETRIES` |
| 重試間隔 | 10 秒 | `WIFI_RETRY_INTERVAL` |
| 連線成功 | 重試計數歸零 | `wifi_retry_count = 0` |
| Failsafe 守護 | 60 秒斷線強制重連 | `failsafe_down_timer >= 60` |

### 3.3 斷線處理 (Callback → Action)

| Callback | 動作 |
|----------|------|
| `CONNECTION_FINISH` | 標記 `networkWifiIsReady = true`，重試計數歸零 |
| `CONNECTING_FAIL` | `WifiMgr_Sta_Disconnect()`，排程重試（10 秒後） |
| `TEMP_DISCONNECT` | 排程重試（10 秒後），若無現有重試進行中 |
| `DISCONNECT_30S` | 排程重試（10 秒後），若無現有重試進行中 |
| `RECONNECTION` | 僅 log（driver 層級事件） |

### 3.4 USB 衝突處理

* WiFi driver 已知 bug：runtime `WifiMgr_Sta_Connect()` 會干擾 USB controller。
* 重試時若偵測到 USB 連線中 (`ITP_DEVICE_USBDACM IS_CONNECTED`)，將重試延後一個週期（10 秒），避免 USB 斷線。

**相關檔案**：`project/GW202601/itp/network_wifi.c`

---

## 4. 藍牙 (BLE) 設定

### 4.1 啟動流程

| 步驟 | 動作 | 時間 |
|------|------|------|
| g_ble_init_step = 1 | BLE task 啟動，**sleep(8)** 等待 RTL8821 晶片上電 | 開機 +8s |
| g_ble_sem_passed | 標記為 true（取代舊的 sem_wait 阻塞機制） | +8s |
| g_ble_init_step = 2 | `bt_log_init()` 初始化 log buffer | |
| g_ble_init_step = 3 | `nimble_port_init()` | |
| g_ble_init_step = 4 | `nimble_port_run()` 進入 event loop（阻塞） | |

* **設計決策**：原先 `sem_wait(&s_wifi_ready_sem)` 會導致 WiFi 初始化失敗時 BLE 永久死鎖。改為非阻塞 `sleep(8)`，確保 BLE 掃描與 USB 數據功能獨立於 WiFi 狀態，100% 正常運作。

### 4.2 掃描參數

| 參數 | 值 | 說明 |
|------|-----|------|
| 掃描模式 | **主動掃描** | `passive = 0` |
| 重複過濾 | **關閉** | `filter_duplicates = 0` (需即時 RSSI 更新) |
| 掃描間隔 | **160 (100ms)** | `itvl` |
| 掃描視窗 | **80 (50ms)** | `window` |
| Duty Cycle | **50%** | 留時間給 WiFi 收發（共用天線） |

* **設計決策**：原先 160/160 (100% duty cycle) 導致 WiFi 連線成功率僅 ~20%。降至 50% 後 WiFi 與 BLE 可穩定共存。

### 4.3 廣播過濾條件

* **過濾位置**：Gateway 底層 `bt_handle_disc()`，不符條件直接丟棄。
* **過濾規則**：在 AD Structure 中搜尋以下欄位：
  - AD Type 0x08（Shortened Local Name）
  - AD Type 0x09（Complete Local Name）
  - AD Type 0xFF（Manufacturer Specific Data）
* **匹配字串**：
  - `MEDFLO`（6 bytes）— MedFlo 設備
  - `NEXMEDAI`（8 bytes）— NexMedAI 設備
* **MAC 提取**：從 Device Name 格式 `MEDFLO-XXXXXXXXXXXX` 中解析真實 MAC（12 hex → 6 bytes）。
* **Status / Counter**：從 Manufacturer Specific Data (AD Type 0xFF) 中解析 status byte 與 rolling counter。

### 4.4 廣播 (Advertising)

| 參數 | 值 |
|------|-----|
| 裝置名稱 | `ITEY_XXXXXXXXXXXX`（MAC 12 位 hex） |
| Flags | `BLE_HS_ADV_F_DISC_GEN \| BLE_HS_ADV_F_BREDR_UNSUP` |
| TX Power | `BLE_HS_ADV_TX_PWR_LVL_AUTO` |
| Duration | `BLE_HS_FOREVER` |
| Discovery Mode | `BLE_GAP_DISC_MODE_GEN` |

### 4.5 執行緒

| 執行緒 | Stack | 說明 |
|--------|-------|------|
| `bt_ble_task` | 預設 (pthread) | BLE 主事件迴圈（`nimble_port_run` 阻塞） |
| `bt_ip_notify_task` | **8 KB** | 每 2 秒 notify IP 至連接的 Central |

**相關檔案**：`project/GW202601/bd/bt_main.c`, `project/GW202601/bd/bt_gatt.c`, `project/GW202601/bd/bt_wifi.c`

---

## 5. BLE 資料統計

以下全域變數供 STATUS JSON 回報 PC App：

| 變數 | 說明 |
|------|------|
| `g_ble_started` | BLE 是否已啟動 |
| `g_ble_sem_passed` | 開機延遲是否完成（取代 sem 阻塞） |
| `g_ble_init_step` | BLE 初始化步驟 (0-5) |
| `g_ble_disc_total` | 收到的 BLE 廣播總數 |
| `g_ble_disc_passed` | 通過 MEDFLO 過濾的數量 |

---

## 6. MQTT 設定

### 6.1 連線參數

| 參數 | 值 |
|------|-----|
| Broker | `mqtt.go6.tw` |
| Port | `1883` |
| Username | `DCareW` |
| Password | `4rfghy6` |
| Client ID | GWID（同 WiFi MAC） |
| Keepalive | **60 秒** |
| Clean Session | **是** (flag 0xC2) |
| QoS | **0** (fire-and-forget) |

### 6.2 Topic 結構

| Topic | 方向 | 用途 |
|-------|------|------|
| `DCare/d/{GWID}` | Gateway → Broker | BLE log batch 上傳 |
| `DCare/d/{GWID}/status` | Gateway → Broker | Gateway 狀態（開機、OTA 進度） |
| `DCare/d/{GWID}/ctrl` | PC → Gateway | **OTA 命令接收** |
| `DCare/d/broadcast/ota` | PC → Gateway | OTA 廣播（所有 gateway 接收） |

### 6.3 連線維持

| 參數 | 值 | 說明 |
|------|-----|------|
| PINGREQ 間隔 | 30 秒 idle | `last_activity` 超過 30 秒送 ping |
| PINGRESP 等待 | 最多 5 秒 | 安全迴圈，不會誤吃 OTA PUBLISH |
| SO_RCVTIMEO | 2 秒 | recv timeout |

### 6.4 OTA 命令處理

* MQTT PUBLISH 封包手動解析（不依賴 MQTT library）。
* PINGREQ/PINGRESP 與 OTA PUBLISH 使用統一接收函數 `mqtt_check_incoming()`。
* OTA 命令格式：`{"cmd":"ota","url":"http://..."}`
* OTA 下載：libcurl → RAM buffer (16MB) → CRC 驗證 → `ugUpgradePackage()` → Watchdog 重置。
* OTA 完成後以 Watchdog 硬體重啟，10ms timeout。

**相關檔案**：`project/GW202601/bd/bt_log.c`, `ota_trigger.py`

---

## 7. 與 PC App 的通訊協定 (USB UART / JSON)

Gateway 透過 USB CDC-ACM 以 JSON 格式與 PC App 通訊。

### 7.1 指令 (PC → Gateway)

| 指令 | 說明 |
|------|------|
| `GET_STATUS` | 查詢 Gateway 狀態 |
| `GET_LOGS` | 索取累積的 BLE log |
| `SET_WIFI <SSID>,<Password>` | 設定 WiFi（儲存，重啟後生效） |
| `REBOOT_TO_BOOTLOADER` | 軟體觸發 Watchdog 重啟進入燒錄模式 |
| `OTA:<URL>` | USB 端觸發 OTA 更新 |

### 7.2 狀態回報 (Gateway → PC) — `STATUS` JSON

```json
{
  "cmd": "STATUS",
  "gw_name": "MedFlow-GW",
  "fw_ver": "<FW_BUILD_TIME>",
  "mac": "38:7A:CC:46:52:5B",
  "wifi_connected": true,
  "ble_started": true,
  "ble_sem_passed": true,
  "ble_init_step": 5,
  "ble_disc_total": 1234,
  "ble_disc_passed": 56,
  "wifi_ssid": "Getnew6336",
  "ip": "192.168.1.100",
  "log_count": 45,
  "buf_usage": 30.0,
  "wifi_retry": 0,
  "time": "2026-07-12T01:30:00+08:00",
  "target_url": "mqtt://mqtt.go6.tw:1883",
  "sys_log": "<escaped system debug log>"
}
```

### 7.3 藍牙資料轉發 — `LOGS` JSON

```json
{
  "cmd": "LOGS",
  "logs": [
    {
      "mac": "F44EFD123456",
      "name": "MEDFLO-F44EFD123456",
      "rssi": -65,
      "status": 0,
      "counter": 42,
      "data": "0201060303E0FF0A16...",
      "time": "2026-07-12T01:30:05+08:00",
      "uploaded": false
    }
  ]
}
```

* 單次上限：**80 筆** (安全邊際)
* 發送前檢查緩衝區剩餘空間 ≥ 300 bytes，不足時提前截斷。

### 7.4 系統日誌 (sys_log)

* Gateway 內建 8KB 環形緩衝區（Ring Buffer），受 `portENTER_CRITICAL()` / `portEXIT_CRITICAL()` 保護。
* `app_printf` 同時輸出至 UART0 與環形緩衝區。
* PC App 發送 `GET_STATUS` 時，緩衝區內容以 JSON 安全轉義後放入 `"sys_log"` 欄位回傳。
* `#define printf app_printf` **僅在應用層 .c 檔局部定義**（bt_main.c, bt_log.c, peripheral.c），嚴禁放 header。

---

## 8. 系統穩定性規則

1. **避免阻塞 I/O**：任何對底層硬體的查詢不可在頻繁迴圈中呼叫 `ioctl(ITP_DEVICE_WIFI, ...)`。
2. **printf 安全**：`#define printf app_printf` 僅限應用層 .c 檔局部定義。全域 header 中定義會導致 SDIO/WiFi 驅動 ISR 中調用非 ISR 臨界區，破壞中斷巢狀暫存器，造成 MAC 全零與晶片癱瘓。
3. **Stack 安全**：`vsnprintf` 回傳值必須做邊界截斷（C99 標準：回傳理論所需長度，非實際寫入長度）。
4. **Watchdog**：10 秒 timeout，系統各處 1 秒餵狗。OTA 燒錄前 disable，完成後以 10ms WD reset 重啟。
5. **USB 與 WiFi 互斥**：USB 連線中時 WiFi 重試自動延後。
6. **BLE 與 WiFi 獨立**：BLE 啟動不依賴 WiFi 狀態（sleep(8) 取代 sem_wait）。

---

## 9. 編譯與燒錄

| 項目 | 說明 |
|------|------|
| 工具鏈 | `C:\ITEGCC_98x\bin\arm-none-eabi-gcc` (FA626 ARM) |
| 編譯 | `.\build_gw_20260707.bat` 或 `make -j 8` |
| ROM 輸出 | `build\openrtos\GW202601\project\GW202601\ITE_NOR.ROM` |
| PKG 輸出 | `build\openrtos\GW202601\project\GW202601\ITEPKG03.PKG` (OTA 用) |
| 燒錄 | `python burn_rom.py` 或 PcNorWriter GUI |
| ROM 備份 | `GW202601_YYYYMMDD_HHMMSS.ROM` |
| CMake | 編譯前務必清除 `CMakeCache.txt`（絕對路徑陷阱） |
