# Gateway BT 廣播格式規格 (NMGW2601)

> **FW 版本**: `GW202601_20260716_184500.ROM`
> **更新日期**: 2026-07-16 19:15
> **對應原始碼**: `ITE9868_GWBuild_20260707`

---

## 1. 角色與用途

Gateway 透過 RTL8821CS 同時執行 **BLE Peripheral (Advertising)** + **BLE Observer (Scanning)**：

| 角色 | 用途 |
|------|------|
| Advertising | 上報 Gateway 自身狀態，供 PC App / BLE Scanner 遠端診斷 |
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

## 5. Status Flags (Byte 26)

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

## 6. Disconnect Reason (Byte 27) + Count (Byte 28)

### Reason 定義

| 值 | 巨集 | 說明 |
|:--:|------|------|
| 0 | — | 無斷線記錄（開機初始值） |
| 1 | `NMGW_DISC_REASON_TEMP` | **TEMP_DISCONNECT** — 瞬間斷線，driver 偵測到短暫 link loss |
| 2 | `NMGW_DISC_REASON_30S` | **DISCONNECT_30S** — 持續斷線超過 30 秒 |
| 3 | `NMGW_DISC_REASON_CONN_FAIL` | **CONNECTING_FAIL** — 連線失敗（認證拒絕、AP 無回應等） |

### Count

- 每次 WiFi callback 觸發斷線事件時 +1
- 0-255 wrapping
- 觀察增速可推算斷線頻率（例如 1 小時增加 50 → 約每 72 秒斷線一次）

---

## 7. Hex 封包範例

### 全部正常，無斷線記錄 (Status=0x1F, Reason=0, Count=0)

```
02 01 06 12 09 4E 4D 47 57 32 36 30 31 2D 43 33 44 34 45 35 46 36
06 FF FF FF 1F 00 00
```

### WiFi driver OK 但連不上 AP (Status=0x10, Reason=TEMP=1, Count=5)

```
02 01 06 12 09 4E 4D 47 57 32 36 30 31 2D 43 33 44 34 45 35 46 36
06 FF FF FF 10 01 05
```

### 持續斷線中的裝置 (Status=0x10, Reason=30S=2, Count=200)

```
02 01 06 12 09 4E 4D 47 57 32 36 30 31 2D 43 33 44 34 45 35 46 36
06 FF FF FF 10 02 C8
```

### Hex 解析快速檢查

```
Byte 2:   必須 = 06 (Flags OK)
Byte 4:   必須 = 09 (Complete Name)
Byte 5-13: "NMGW2601-" (4E 4D 47 57 32 36 30 31 2D)
Byte 14-21: MAC 後 4 bytes (8 hex chars)
Byte 23:  AD Type = FF (Manufacturer Data)
Byte 26:  Status Flags
            BIT4=1 → WiFi hardware OK
            BIT0=1 → WiFi connected + IP obtained
Byte 27:  Disconnect Reason
            0=無, 1=TEMP, 2=30S, 3=CONN_FAIL
Byte 28:  Disconnect Count (0-255, wrapping)
```

---

## 8. 狀態更新機制

| Flag / Byte | 更新來源 | 觸發時機 |
|-------------|---------|---------|
| `WIFI_READY` | `network_wifi.c` | `WifiMgr_Init()` 回傳 `WIFIMGR_ECODE_OK` 時設為 1 |
| `WIFI_OK` | `bt_adv_set_fields()` | 每次重建廣告欄位時檢查 `bt_wifi_get_ip()` |
| `BLE_OK` | `on_sync()` | NimBLE host-controller sync 完成 |
| `MQTT_OK` | `MqttUploadTask()` | DCareW CONNACK OK → true; close(sock) → false |
| `OTA_OK` | `MqttUploadTask()` | DCareR CONNACK OK → true; close(ota_sock) → false |
| Disconnect Reason | `wifiCallbackFucntion()` | 每次 TEMP_DISCONNECT / DISCONNECT_30S / CONNECTING_FAIL callback |
| Disconnect Count | `wifiCallbackFucntion()` | 每次斷線 callback 時 +1 |

廣告中的 Manufacturer Data 在以下時機刷新：
- `nmgw_status_update()` 被呼叫時（若 `g_ble_started == true` 則立即重建廣告欄位）
- `bt_advertise()` 被呼叫時（重新連線 / 斷線重連等自然事件）

> **Thread safety**: `nmgw_disconnect_record()` 和 `nmgw_status_set_flag()` 僅寫入全域變數，不呼叫 NimBLE API，可安全從 WiFi thread 呼叫。

---

## 9. WiFi/BLE 共存策略

| 模式 | 佔空比 | 說明 |
|------|--------|------|
| BLE 掃描 | 20% (40ms/200ms) | 主動掃描 MedFlo 設備 |
| BLE 廣告 | ~0.03% (3ms/100ms) | 每 100ms 廣播一次狀態 (~30ms/sec) |
| WiFi 可用 | ~78.5% | MQTT 上報、OTA 下載 |

RTL8821CS 單 RF 晶片，WiFi 與 BT 透過硬體 PTA (Packet Traffic Arbitration) 協調時間分配。

---

## 10. 原始碼對照

| 功能 | 檔案 |
|------|------|
| Status Flags 定義 | `project/GW202601/bd/bt_main.h` |
| Disconnect Reason 定義 | `project/GW202601/bd/bt_main.h` |
| 廣播/掃描/狀態管理 | `project/GW202601/bd/bt_main.c` |
| Manufacturer Data 結構 | `project/GW202601/bd/bt_main.c` |
| 斷線事件記錄 | `project/GW202601/bd/bt_main.c` (`nmgw_disconnect_record`) |
| WiFi callback 觸發點 | `project/GW202601/itp/network_wifi.c` |
| LCD 狀態顯示 | `project/GW202601/scene.c` |

---

## 11. 變更記錄

| 日期 | 變更 |
|------|------|
| 2026-07-16 | **新增 Disconnect Reason (Byte 27) + Disconnect Count (Byte 28)**：用於遠端診斷 WiFi 斷線頻率與原因 |
| 2026-07-16 | **新增 BIT4 `WIFI_READY`**：區分 WiFi 硬體故障 vs 連不上 AP |
| 2026-07-14 | 名稱改為 NMGW2601-、MAC 縮為 4 bytes、移除 TX Power、新增 Manufacturer Data (BIT0-3) |
| 2026-07-14 | 移除廣告中的 UUID 0xAAAA（與 Mfg Data 合計超 31 bytes → BLE_HS_EMSGSIZE） |
