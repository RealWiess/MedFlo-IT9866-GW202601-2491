# Gateway BT 廣播格式規格 (NMGW2601)

> 版本: 2026-07-14
> 對應 FW: `ITE9868_GWBuild_20260707`

---

## 1. Gateway BT 角色

Gateway 透過 RTL8821CS 同時執行 **BLE Peripheral (Advertising)** + **BLE Observer (Scanning)**：

| 角色 | 用途 |
|------|------|
| Advertising | 上報 Gateway 自身狀態，供 PC App 連線設定 WiFi |
| Scanning | 掃描周遭 MedFlo/NEXMEDAI 設備廣播，提取 GPIO 狀態 |

---

## 2. 廣播參數 (Advertising)

| 參數 | 設定值 | 說明 |
|------|--------|------|
| 廣播間隔 | **100ms** (`BLE_GAP_ADV_ITVL_MS(100)`) | 每秒約 10 次，狀態監控 |
| 隨機抖動 | +50ms (`itvl_max=150ms`) | 避免多台 GW 同時發送碰撞 |
| 廣播類型 | **Connectable** | `BLE_GAP_CONN_MODE_UND` |
| 發現模式 | General Discoverable | `BLE_GAP_DISC_MODE_GEN` |
| 持續時間 | `BLE_HS_FOREVER` | Gateway 不休眠，持續運作 |
| 裝置名稱 | `NMGW2601-XXXXXXXX` | 前綴 9 chars + MAC 後 4 bytes (8 hex) |
| TX Power | **不發送** | Gateway 不需測距定位 |

---

## 3. 掃描參數 (Scanning)

| 參數 | 設定值 | 說明 |
|------|--------|------|
| 掃描間隔 | **200ms** (`itvl=320`) | 320 × 0.625ms |
| 掃描視窗 | **40ms** (`window=64`) | 64 × 0.625ms, 20% duty cycle |
| 掃描模式 | **Active** (`passive=0`) | 可收到 Scan Response 中的裝置名稱 |
| 去重複過濾 | **關閉** (`filter_duplicates=0`) | 接收每個重複廣播以維持即時 RSSI |
| 持續時間 | `BLE_HS_FOREVER` | 持續掃描 |

### 時間分配 (每 200ms 週期)

```
├─ 40ms：BLE 掃描 (20%)
├─ ~3ms：BLE 廣告 (~1.5%)
├─ 157ms：空閒 → 留給 WiFi (78.5%)
```

---

## 4. 廣播封包結構 (Advertising Data)

**總長度: 27 bytes** (BLE 上限 31 bytes)

> ⚠️ UUID 0xAAAA (WiFi 控制服務) 已從廣告封包中移除，因為加入 Manufacturer Data 後會超過 31 bytes。
> GATT Service 0xAAA0 僅在連線後可見，不影響廣播階段。

### 結構圖

```
Offset  Bytes  欄位                    值
────────────────────────────────────────────────────
0       1      AD Length               0x02
1       1      AD Type (Flags)         0x01
2       1      Flags                   0x06
               ├─ BIT0: LE Limited Discoverable
               ├─ BIT1: LE General Discoverable  ✓
               └─ BIT2: BR/EDR Not Supported     ✓

3       1      AD Length               0x12 (18 bytes)
4       1      AD Type                0x09 (Complete Local Name)
5-21    17     Device Name (ASCII)     "NMGW2601-XXXXXXXX"
               ├─ 5-13:  前綴 "NMGW2601-"
               └─ 14-21: MAC 後 4 bytes (8 hex chars)

22      1      AD Length               0x04
23      1      AD Type                0xFF (Manufacturer Specific)
24-25   2      Company ID             0xFF 0xFF (BT SIG 測試用, LSB first)
26      1      Status Flags           bitmask (見下方)
────────────────────────────────────────────────────
Total: 27 bytes (剩餘 4 bytes 可用)
```

---

## 5. Manufacturer Data

### 結構體定義

```c
struct nmgw_manufacturer_data {
    uint16_t company_id;      // 0xFFFF (Bluetooth SIG 測試用 ID)
    uint8_t  status_flags;    // Gateway 狀態 bitmask
};
```

### Status Flags

| Bit | 巨集 | 說明 |
|-----|------|------|
| 0 | `NMGW_FLAG_WIFI_OK` | WiFi 已連線 (IP ≠ 0.0.0.0) |
| 1 | `NMGW_FLAG_BLE_OK` | BLE 掃描/廣播運行中 |
| 2 | `NMGW_FLAG_MQTT_OK` | MQTT DCareW 已連線 |
| 3 | `NMGW_FLAG_OTA_OK` | MQTT DCareR (OTA) 已連線 |
| 4-7 | — | 預留 |

### 常見狀態值

| 值 | 二進位 | 狀態 |
|-----|--------|------|
| `0x0F` | `0000 1111` | 🟢 全部正常 |
| `0x0E` | `0000 1110` | 🟡 WiFi 斷線 |
| `0x0D` | `0000 1101` | 🟡 BLE 未啟動 |
| `0x0C` | `0000 1100` | 🟠 WiFi + BLE 都斷 |
| `0x0B` | `0000 1011` | 🟡 MQTT (DCareW) 斷線 |
| `0x07` | `0000 0111` | 🟡 OTA (DCareR) 斷線 |
| `0x03` | `0000 0011` | 🟠 BLE + MQTT + OTA 斷 (僅 WiFi) |
| `0x00` | `0000 0000` | 🔴 全部離線 |

---

## 6. Byte 陣列範例

### 裝置 MAC: A1:B2:C3:D4:E5:F6 → 後 4 bytes = C3D4E5F6

```
全部正常 (0x0F):
02 01 06 12 09 4E 4D 47 57 32 36 30 31 2D 43 33 44 34 45 35 46 36
04 FF FF FF 0F

WiFi 斷線 (0x0E):
02 01 06 12 09 4E 4D 47 57 32 36 30 31 2D 43 33 44 34 45 35 46 36
04 FF FF FF 0E

全部離線 (0x00):
02 01 06 12 09 4E 4D 47 57 32 36 30 31 2D 43 33 44 34 45 35 46 36
04 FF FF FF 00
```

### Hex 解析快速檢查

```
Byte 2: 必須 = 06 (Flags OK)
Byte 4: 必須 = 09 (Complete Name)
Byte 5-13: "NMGW2601-" (4E 4D 47 57 32 36 30 31 2D)
Byte 23: AD Type = FF (Manufacturer Data)
Byte 26: Status Flags (目標值 = 0F)
```

---

## 7. 狀態更新機制

| Flag | 更新來源 | 觸發時機 |
|------|---------|---------|
| `WIFI_OK` | `bt_adv_set_fields()` | 每次重建廣告欄位時檢查 `bt_wifi_get_ip()` |
| `BLE_OK` | `on_sync()` | NimBLE host-controller sync 完成後設為 true |
| `MQTT_OK` | `MqttUploadTask()` | DCareW CONNACK OK → true; close(sock) → false |
| `OTA_OK` | `MqttUploadTask()` | DCareR CONNACK OK → true; close(ota_sock) → false |

廣告中的 Manufacturer Data 會在以下時機刷新：
- `nmgw_status_update()` 被呼叫時（若 `g_ble_started == true` 則立即重建廣告欄位）
- `bt_advertise()` 被呼叫時（重新連線 / 斷線重連等自然事件）

---

## 8. WiFi/BLE 共存策略

| 模式 | 佔空比 | 說明 |
|------|--------|------|
| BLE 掃描 | 20% (40ms/200ms) | 主動掃描 MedFlo 設備 |
| BLE 廣告 | ~0.03% (3ms/5000ms) | 每 5 秒廣播一次狀態 |
| WiFi 可用 | ~78.5% | MQTT 上報、OTA 下載 |

RTL8821CS 單 RF 晶片，WiFi 與 BT 透過硬體 PTA (Packet Traffic Arbitration) 協調時間分配。

---

## 9. 原始碼對照

| 功能 | 檔案 |
|------|------|
| 廣播/掃描/狀態管理 | `project/GW202601/bd/bt_main.c` |
| Status flags 定義 | `project/GW202601/bd/bt_main.h` |
| MQTT/OTA 狀態同步 | `project/GW202601/bd/bt_log.c` |
| GATT Service (WiFi 控制) | `project/GW202601/bd/bt_gatt.c` |

---

## 10. 變更記錄

| 日期 | 變更 |
|------|------|
| 2026-07-14 | 初始版本：名稱改為 NMGW2601-、MAC 縮為 4 bytes、移除 TX Power、新增 Manufacturer Data (4 flags)、廣告間隔 100ms、掃描 200ms/40ms |
| 2026-07-14 | **修正**：移除廣告中的 UUID 0xAAAA（與 Mfg Data 合計超 31 bytes → BLE_HS_EMSGSIZE）。GATT 0xAAA0 僅連線後可見 |
