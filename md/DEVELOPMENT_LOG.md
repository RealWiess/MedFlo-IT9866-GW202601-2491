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
* 修正 GW202601_LVGL include 路徑
* LVGL-based main.c 合併 Gateway 模組
* 安裝 lvgl-mcp-server

---

## 開發規則
1. 所有 SDK 2491 相關對話及開發記錄，均記錄於此檔案及 `md/` 目錄下
2. 穩定版的文件在該目錄的 `md/` 下，兩個專案獨立記錄
