# MedFlo Gateway (ITE9868) SDK 2491 研發日誌

本文件記錄基於 ITE SDK V2.4.9.1 + LVGL v9.3.0 的 Gateway 開發歷程。

> **穩定版**：`C:\SW code\source code\ITE9868_GWBuild_20260707\`（ITU 架構）
> **本專案**：`C:\SW code\source code\ITE9868_GWBuild_2491_20260718\`（LVGL 架構遷移中）

---

## 2026-07-19

### 1. 新 SDK 環境建置
* **目標**：從 SDK V2.4.9.1 建立新工作目錄，逐步將 Gateway 從 ITU 遷移到 LVGL v9.3.0
* **SDK 來源**：`C:\公司\電子元件\ITE\IT986x\SDK\SDK_V2.4.9.1\ite_sdk\`
* **目錄結構**：
  ```
  ITE9868_GWBuild_2491_20260718/
  ├── sdk/           ← SDK V2.4.9.1
  ├── openrtos/      ← SDK V2.4.9.1
  ├── cmake/         ← SDK V2.4.9.1
  ├── project/
  │   ├── GW202601/       ← 穩定版直接複製
  │   ├── GW202601_LVGL/  ← 移植中
  │   └── lv_demos/       ← SDK 原生
  └── CMakeLists.txt
  ```
* **Repo**：https://github.com/RealWiess/MedFlo-IT9866-GW202601-2491

### 2. 編譯修正
* ASM 語言：`enable_language(C CXX ASM)`
* extensions.cmake：`include(extensions)` 定義 `target_sources_ifdef`
* CFG_LFS_CACHE_SIZE：設為 `0x2000`
* math_compat.h：INFINITY/NAN `#ifndef` 檢查
* NimBLE headers：`sdk/include/nimble/`
* LCD/DDR init scripts 從舊 SDK 複製

### 3. 編譯狀態
| Target | 狀態 |
|--------|:--:|
| mbedtls, curl v8.12.1, lvgl v9.3.0, nimble | ✅ |
| GW202601 (舊 CMakeLists) | ❌ |
| GW202601_LVGL (移植中) | 🔶 |

### 4. 下一步
* ~~修正 GW202601_LVGL include 路徑~~ ✅ 完成
* ~~LVGL-based main.c 合併 Gateway 模組~~ ✅ 完成
* ~~讓 GW202601_LVGL 編譯通過~~ ✅ 完成
* 安裝 lvgl-mcp-server

---

## 2026-07-19 (下午 — 編譯成功)

### 5. GW202601_LVGL 初次編譯成功

#### 編譯修正明細
- **CMakeLists.txt**: 移除 bleLinkWiFi/ 衝突程式（舊 NimBLE demo 與 bd/ 衝突）
- **main.c**: 從穩定版 GW202601 合併所有 Gateway 初始化（WiFi/BLE/MQTT/USB）+ LVGL init
- **scene.c**: `ituPngLoadFile` 加入第 4 參數 (flags)
- **json_inttypes.h**: 加入 `#ifndef` 衛語句 (PRId64/SCNd64/PRIu64)
- **bleprph.h**: 加入 `#ifndef` 衛語句 (MYNEWT_VAL_BLE_ROLE_*)
- **network_wifi.c**: forward declare `createTcpServerThread`, 加入 SDL include, fix accept() cast
- **backup.c**: itcFileStreamClose/itcStreamRead/itcStreamWrite → 使用 `.stream` member
- **canbus_fmt.c/h**: `uint32_t**` → `uint32_t*`, `struct WidgetNode_t*` → `struct _WidgetNode*`
- **dbWidget.h**: 加入 `#include "ite/itu.h"`, declare `deleteMileCalcWidget`
- **dbArray.c**: `static destoryDataDbArray` → `static void destoryDataDbArray`
- **dbTimer.c/dbWidget.c/dashboard_data.c/uart_parser.c**: 加入 SDL include
- **bt_gatt.c**: `const ble_uuid_t *uuid` → `uint16_t uuid` (NimBLE API 變更)
- **bt_wifi.c**: 加入 `#ifndef NET_IF_NUM` 衛語句
- **bt_main.c**: `ble_hs_id_set_pub` → `ble_hs_id_set_rnd`
- **bootloader/boot.c/logo.c**: fix void return + const correctness
- **mqueue**: add missing function declarations to headers
- **util.h/peripheral.h/mqueue_uart_in.h**: add missing declarations

#### 已知 Workaround
- **pkgtool**: SDK 2491 的 build.cmake 使用新版 long options (`--unformatted-device`) 但 pkgtool V3.2.5 不支援。暫時使用舊 SDK 的 pkgtool 並手動執行舊式命令產生 ROM。
- **IT2D**: 禁用 `LV_USE_DRAW_ITE_IT2D`（driver 未建置）
- **Brotli**: stub `BrotliDecoderDecompress`
- **time_pv**: stub `time_pv_stretch/init/free`
- **LTO**: 暫時禁用 `CFG_GCC_LTO`
- **libitu_private/libusbhcc**: 從舊 build 複製 pre-built libraries
- **startup.o/tlb_wt.o**: 從舊 build 複製
- **codec files**: 從舊 build 複製 mp3.codecs / wave.codecs

#### 產出
- `ITE_NOR.ROM` (16 MB) — Gateway FW with LVGL v9.3.0
- `ITEPKG03.PKG` (4.5 MB) — OTA package
- ROM 備份：`GW202601_LVGL_20260719_181315.ROM`
- 手動 PKG 指令（pkgtool 新舊版不相容的 workaround）

### 6. lvgl-mcp-server 安裝
- `npm install -g lvgl-mcp-server` (94 packages)
- `.claude/settings.json` 建立於專案根目錄
- MCP server 啟動測試通過
- 功能：LVGL C code → 模擬器編譯 → PNG 截圖 + JSON widget tree
- 後續可用於快速 UI 開發迭代

---

## 明日 (2026-07-20) 待辦

1. **MCP 模擬器測試** — 重啟 Claude Code 後，用 lvgl-mcp-server 測試 LVGL 畫面渲染
2. **LVGL Gateway UI 開發** — 用模擬器開發 Gateway 主畫面（取代 ITU scene）
3. **pkgtool 相容性** — 修正 SDK 2491 build.cmake 的新版 long options，讓 make 能一次完成
4. **LTO 恢復** — 解決 LTO linking 警告後重新啟用
5. **IT2D driver 建置** — 確認 it2d driver 編譯設定，恢復硬體加速
6. **燒錄測試** — 將 ROM 燒錄到 Gateway 板子驗證實際運作

---

## 2026-07-22 — IT2D 啟用 + 雙環 HMI + WiFi 移植

### 重大突破：IT2D 硬體加速啟用
* **根因**: `LV_USE_DRAW_ITE_IT2D` = 0 → `lvgl_init()` 被跳過 → display 未註冊
* **修復**: `lv_conf.h` 設為 1, config 加 `CFG_BUILD_IT2D`/`CFG_BUILD_BROTLI`, CMakeLists 手動 link `it2d` `brotli`
* **結果**: LCD 點亮，`lv_demo_widgets` 成功，BACKLIGHT_ON

### 雙環 HMI
* Splash (NMGW2601 logo, 3s) → Main HMI (arc rings + status panels + version)
* 全部從零手寫 (`ui_screen.c/h`)

### WiFi 移植
* **SDIO 成功**: RTL8821CS @50MHz 4-bit
* **WifiMgr 未解**: Init 始終返回 2
* SDK bug 修復: `itp_init_openrtos.c:900` 未宣告 `err`

### 最終 ROM
* `gw_lvgl_test_20260722_235659.ROM` — 純 LVGL 顯示，無 WiFi，系統穩定

---

## 開發規則
1. 所有 SDK 2491 相關對話及開發記錄，均記錄於此檔案及 `md/` 目錄下
2. 穩定版的文件在該目錄的 `md/` 下，兩個專案獨立記錄
