# Gateway BT 廣播格式規格 (NMGW2601)

> **FW 版本**: `GW202601_20260717_152123.ROM`
> **更新日期**: 2026-07-17 15:30
> **對應原始碼**: `ITE9868_GWBuild_20260707`

---

## 1. 角色與用途

Gateway 透過 RTL8821CS 同時執行 **BLE Peripheral (Advertising + Scan Response)** + **BLE Observer (Scanning)**：

| 角色 | 用途 |
|------|------|
| Advertising | 上報 Gateway 自身狀態，供 PC App / BLE Scanner 遠端診斷 |
| Scan Response | 攜帶 FW 版本號 (`FW_BUILD_TIME`)，供遠端辨識 |
| Scanning | 掃描周遭 MedFlo / NEXMEDAI 設備廣播 |

---

## 2. 廣播參數 (Advertising)

| 參數 | 設定值 | 說明 |
|------|--------|------|
| 廣播間隔 | **100ms** (`BLE_GAP_ADV_ITVL_MS(100)`) | 每秒約 10 次 |
| 隨機抖動 | +50ms (`itvl_max=150ms`) | 避免多台 GW 同時發送碰撞 |
| 廣播類型 | **Connectable** | `BLE_GAP_CONN_MODE_UND` |
| 發現模式 | General Discoverable | `BLE_GAP_DISC_MODE_GEN` |
| 持續時間 | `BLE_HS_FOREVER` | Gateway 不休眠 |
| 裝置名稱 | `NMGW2601-XXXXXXXX` | 前綴 9 chars + MAC 後 4 bytes (8 hex) |
| TX Power | **不發送** | |

---

## 3. 掃描參數 (Scanning)

| 參數 | 設定值 | 說明 |
|------|--------|------|
| 掃描間隔 | **200ms** (`itvl=320`) | 320 × 0.625ms |
| 掃描視窗 | **40ms** (`window=64`) | 64 × 0.625ms, 20% duty cycle |
| 掃描模式 | **Active** (`passive=0`) | 可收到 Scan Response |
| 去重複過濾 | **關閉** (`filter_duplicates=0`) | 維持即時 RSSI |
| 過濾條件 | Name 含 `MEDFLO` 或 `NEXMEDAI` | Gateway 底層過濾，不符直接丟棄 |

### 時間分配 (每 200ms 週期)

```
├─ 40ms：BLE 掃描 (20%)
├─ ~3ms：BLE 廣告 (~1.5%)
├─ 157ms：空閒 → 留給 WiFi (78.5%)
```

---

## 4. 廣播封包結構 (Advertising Data)

**總長度: 29 bytes** (BLE 上限 31 bytes, 剩餘 2 bytes)

```
Offset  Bytes  欄位                    值
────────────────────────────────────────────────────
0       1      AD Length               0x02
1       1      AD Type (Flags)         0x01
2       1      Flags                   0x06
               ├─ BIT1: LE General Discoverable  ✓
               └─ BIT2: BR/EDR Not Supported     ✓

3       1      AD Length               0x12 (18 bytes)
4       1      AD Type                 0x09 (Complete Local Name)
5-21    17     Device Name (ASCII)     "NMGW2601-XXXXXXXX"
               ├─ 5-13:  前綴 "NMGW2601-"
               └─ 14-21: MAC 後 4 bytes (8 hex chars)

22      1      AD Length               0x06 (6 bytes)
23      1      AD Type                 0xFF (Manufacturer Specific)
24-25   2      Company ID              0xFF 0xFF (BT SIG 測試用, LSB first)
26      1      Status Flags            bitmask (見 §5)
27      1      Disconnect Reason       上次斷線原因 (見 §6)
28      1      Disconnect Count        累計斷線次數 (見 §6)
────────────────────────────────────────────────────
Total: 29 bytes
```

---

## 5. Scan Response 封包結構

**總長度: 19 bytes** (獨立 31 bytes 上限，不影響主廣告)

```
Offset  Bytes  欄位                    值
────────────────────────────────────────────────────
0       1      AD Length               0x12 (18 bytes)
1       1      AD Type                 0xFF (Manufacturer Specific)
2-3     2      Company ID              0xFF 0xFF (BT SIG 測試用, LSB first)
4-18    15     FW Version (ASCII)      FW_BUILD_TIME (YYYYMMDD_HHMMSS)
────────────────────────────────────────────────────
Total: 19 bytes
```

**範例**: `FF FF 32 30 32 36 30 37 31 37 5F 31 35 32 31 32 33` → Company 0xFFFF + "20260717_152123"

---

## 6. Status Flags (Byte 26)

### Bitmap 定義

| Bit | 巨集 | 說明 |
|:---:|------|------|
| 0 | `NMGW_FLAG_WIFI_OK` | WiFi 已連線且取得 IP (≠ 0.0.0.0) |
| 1 | `NMGW_FLAG_BLE_OK` | BLE 掃描/廣播運行中 |
| 2 | `NMGW_FLAG_MQTT_OK` | MQTT DCareW 已連線 |
| 3 | `NMGW_FLAG_OTA_OK` | MQTT DCareR (OTA) 已連線 |
| **4** | **`NMGW_FLAG_WIFI_READY`** | **WiFi driver 初始化成功 (WifiMgr_Init OK)** |
| 5-7 | — | 預留 |

### WiFi 診斷矩陣 (BIT4 + BIT0)

| BIT4 (HW Ready) | BIT0 (Connected) | 值 | 診斷 |
|:---:|:---:|:---:|------|
| 0 | 0 | `0x00` | 🔴 WiFi 硬體/driver 故障 — RTL8821 無回應、SDIO 不通 |
| 1 | 0 | `0x10` | 🟡 WiFi 晶片正常，連不上 AP — SSID/密碼/訊號問題 |
| 1 | 1 | `0x11` | 🟢 WiFi 完全正常 |

### 常見完整 Status Flags 值

| 值 | 二進位 | 狀態 |
|:---:|--------|------|
| `0x1F` | `0001 1111` | 🟢 全部正常 |
| `0x1E` | `0001 1110` | 🟡 WiFi driver OK，但連不上 AP |
| `0x0E` | `0000 1110` | 🔴 WiFi driver 故障 |
| `0x1D` | `0001 1101` | 🟡 BLE 未啟動 |
| `0x1B` | `0001 1011` | 🟡 MQTT DCareW 斷線 |
| `0x17` | `0001 0111` | 🟡 OTA DCareR 斷線 |
| `0x00` | `0000 0000` | 🔴 全部離線 |

---

## 7. Disconnect Reason (Byte 27) + Count (Byte 28)

### Reason 定義

| 值 | 巨集 | 說明 |
|:--:|------|------|
| 0 | — | 無斷線記錄（開機初始值） |
| 1 | `NMGW_DISC_REASON_TEMP` | **TEMP_DISCONNECT** — 瞬間斷線，driver 偵測到短暫 link loss |
| 2 | `NMGW_DISC_REASON_30S` | **DISCONNECT_30S** — 持續斷線超過 30 秒 |
| 3 | `NMGW_DISC_REASON_CONN_FAIL` | **CONNECTING_FAIL** — 連線失敗（認證拒絕、AP 無回應等） |

### Count

- 三種斷線類型各自獨立計數（`s_nmgw_disc_count_temp / _30s / _fail`）
- Byte 28 = 三種合計 (`temp + 30s + fail`)，0-255 wrapping
- 觀察增速可推算斷線頻率（例如 1 小時增加 50 → 約每 72 秒斷線一次）

---

## 8. 狀態與計數更新機制

### Status Flags 更新

| Flag / Byte | 更新來源 | 觸發時機 |
|-------------|---------|---------|
| `WIFI_READY` | `network_wifi.c` | `WifiMgr_Init()` 回傳 `WIFIMGR_ECODE_OK` 時設為 1 |
| `WIFI_OK` | `bt_adv_set_fields()` | 每次重建廣告欄位時檢查 `bt_wifi_get_ip()` |
| `BLE_OK` | `on_sync()` | NimBLE host-controller sync 完成 |
| `MQTT_OK` | `MqttUploadTask()` | DCareW CONNACK OK → true; close(sock) → false |
| `OTA_OK` | `MqttUploadTask()` | DCareR CONNACK OK → true; close(ota_sock) → false |

### Disconnect Counter 即時刷新 (🆕 2026-07-17)

- `nmgw_disconnect_record()` 僅寫入全域變數並設 `s_adv_dirty = true`（thread-safe，WiFi thread 可安全呼叫）
- `bt_ip_notify_task` 每秒檢查 `s_adv_dirty`，若為 true 則呼叫 `bt_adv_set_fields()` 刷新廣告
- **DC 值更新延遲 ≤ 1 秒**

### 其他刷新時機

- `nmgw_status_update()` 被呼叫時（若 `g_ble_started == true` 則立即重建廣告欄位）
- `bt_advertise()` 被呼叫時（重新連線 / 斷線重連等自然事件）

### Scan Response (FW 版本)

- `bt_advertise()` 內呼叫 `bt_adv_set_scan_rsp()`，設定 Scan Response Manufacturer Data
- 內容：Company ID 0xFFFF + `FW_BUILD_TIME` 字串
- 僅在 BLE Scanner 執行 Active Scan 時回傳

---

## 9. WiFi/BLE 共存策略

| 模式 | 佔空比 | 說明 |
|------|--------|------|
| BLE 掃描 | 20% (40ms/200ms) | 主動掃描 MedFlo 設備 |
| BLE 廣告 | ~3% (3ms/100ms) | 每 100ms 廣播一次狀態 |
| WiFi 可用 | ~77% | MQTT 上報、OTA 下載 |

RTL8821CS 單 RF 晶片，WiFi 與 BT 透過硬體 PTA (Packet Traffic Arbitration) 協調時間分配。

---

## 10. TEMP_DISCONNECT 重試策略 (🆕 2026-07-17)

| 項目 | 設定 |
|------|------|
| TEMP callback 行為 | 僅設 debounce pending，**不立即排程重試** |
| Debounce 時間 | 3 秒（若 WiFi 恢復則取消，不計數） |
| Debounce 確認後 | 記錄 TEMP + **排程重試** |
| Backoff | 連續 TEMP 時拉長重試間隔：10s → 20s → 40s → 80s (max) |
| Backoff 重置 | `CONNECTION_FINISH` 成功連線後歸零 |
| DISCONNECT_30S / CONNECTING_FAIL | 照常立即排程重試（10s），不受 backoff 影響 |

---

## 11. 原始碼對照

| 功能 | 檔案 |
|------|------|
| Status Flags / Disconnect Reason 定義 | `project/GW202601/bd/bt_main.h` |
| 廣播/掃描/狀態管理 | `project/GW202601/bd/bt_main.c` |
| Scan Response (FW 版本) | `project/GW202601/bd/bt_main.c` → `bt_adv_set_scan_rsp()` |
| Disconnect 記錄與 dirty flag | `project/GW202601/bd/bt_main.c` → `nmgw_disconnect_record()` |
| DC 即時廣告刷新 | `project/GW202601/bd/bt_main.c` → `bt_ip_notify_task()` |
| TEMP debounce + backoff 重試 | `project/GW202601/itp/network_wifi.c` |
| WiFi callback 觸發點 | `project/GW202601/itp/network_wifi.c` |

---

## 12. 變更記錄

| 日期 | 變更 |
|------|------|
| 2026-07-17 | **恢復 BLE 掃描**（WiFi TEMP 問題與 BLE 無關）+ **DC dirty flag** 即時刷新廣告 |
| 2026-07-17 | **新增 Scan Response** 攜帶 FW_BUILD_TIME (19 bytes) |
| 2026-07-17 | **TEMP_DISCONNECT 重試策略改進**：debounce 確認後才重試 + exponential backoff |
| 2026-07-16 | 新增 Disconnect Reason (Byte 27) + Disconnect Count (Byte 28) |
| 2026-07-16 | 新增 BIT4 `WIFI_READY`：區分 WiFi 硬體故障 vs 連不上 AP |
| 2026-07-14 | 名稱改為 NMGW2601-、MAC 縮為 4 bytes、移除 TX Power、新增 Manufacturer Data (BIT0-3) |
| 2026-07-14 | 移除廣告中的 UUID 0xAAAA（與 Mfg Data 合計超 31 bytes → BLE_HS_EMSGSIZE） |
