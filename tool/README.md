# ITE LVGL Auto Pipeline

> **從 JSON UI 設計稿到 IT9866 ROM，一條指令完成。**

本工具鏈將 LVGL UI 開發流程完全自動化：設計師編寫 JSON 描述 UI → 自動產生 LVGL C 程式碼 + 圖片資源 → 編譯 → 燒錄到 IT9866/IT9868 晶片。

---

## 目錄結構

```
ITE_LVGL_auto/
├── README.md                          ← 本文件
├── workflow.py                        ← 一鍵流水線主控台
├── tools/
│   ├── json2lvgl.py                   ← JSON 設計稿 → LVGL C code
│   └── assets_pipeline.py             ← PNG 圖片 → LVGL C array
├── schema/
│   └── lvgl_ui_schema.json            ← JSON Schema 規範定義
├── examples/
│   └── gateway_hmi_example.json       ← Gateway 雙環 HMI 範例
└── output/                            ← 生成檔案輸出目錄
```

---

## 快速開始

### 環境需求

| 需求 | 說明 |
|------|------|
| Python 3.8+ | 執行工具鏈 |
| Pillow | `pip install Pillow` |
| ITEGCC 98x | 交叉編譯器，預設路徑 `C:\ITEGCC_98x\` |
| ITE SDK 2491 | 專案根目錄 |
| LVGL MCP Server | (選用) `npm install -g lvgl-mcp-server`，用於模擬驗證 |

### 一鍵執行

```bash
cd C:\SW code\source code\ITE_LVGL_auto

# 完整流程：PNG轉換 + JSON轉LVGL + 編譯
python workflow.py examples/gateway_hmi_example.json

# 預覽模式（只看 UI 統計資料，不產生檔案）
python workflow.py examples/gateway_hmi_example.json --preview

# 只產生程式碼，不編譯
python workflow.py my_ui.json --skip-build

# 完整流程 + 自動燒錄
python workflow.py my_ui.json --flash
```

---

## 工作流架構

```
┌─────────────────────────────────────────────────────────────┐
│                    workflow.py (主控台)                       │
│                                                               │
│  ┌─────────────────┐   ┌─────────────────┐   ┌────────────┐ │
│  │ Stage 1          │   │ Stage 2          │   │ Stage 3+4  │ │
│  │ assets_pipeline  │   │ json2lvgl        │   │ CMake+make │ │
│  │ PNG → C array    │   │ JSON → LVGL C    │   │ → ROM      │ │
│  └────────┬────────┘   └────────┬────────┘   └─────┬──────┘ │
│           │                     │                   │         │
│           ▼                     ▼                   ▼         │
│     img_*.c/.h            screen.c/.h          ITE_NOR.ROM   │
│     (LVGL 圖片資源)        (LVGL 畫面程式碼)     (韌體檔)     │
└─────────────────────────────────────────────────────────────┘
```

---

## 工具詳解

### 1. `tools/assets_pipeline.py` — 圖片資源轉換器

**用途**：將 PNG/JPEG/BMP 圖片轉換為 LVGL 可用的 C 陣列格式，專門針對 IT9866 的 IT2D GPU 優化。

**為什麼需要它**：LVGL 在嵌入式系統上無法直接讀取 PNG 檔案，所有圖片必須預先轉換為 C 語言陣列（像素資料），編譯進韌體中。

**功能**：

| 功能 | CLI 參數 | 說明 |
|------|---------|------|
| RGB565 轉換 | `--rgb565` | 16-bit 色彩，無透明通道（預設） |
| ARGB8888 轉換 | `--argb8888` | 32-bit 色彩，含 8-bit 透明通道 |
| Alpha 遮罩 | `--alpha8` | 只取透明通道，用於遮罩合成 |
| **硬體變色模式** | `--grayscale` | 灰階輸出 → IT2D 硬體 `ite_it2d_recolor` 可即時變色 |
| Chroma Key 去背 | `--chroma-key` | 用指定顏色標記透明區域，IT2D 硬體去背 |
| 縮放 | `--resize 480x480` | 自動縮放圖片至目標尺寸 |
| 批次處理 | `--manifest assets.json` | 依 JSON 清單批次轉換 |
| 掃描目錄 | `--all --input-dir UI/` | 自動轉換目錄內所有圖片 |
| 產生清單 | `--gen-manifest` | 掃描目錄後產生 manifest JSON 模板 |

**IT2D GPU 優化說明**：

IT9866 的 IT2D 是一個 2.5D GPU，具備以下硬體加速功能，`assets_pipeline.py` 的輸出格式直接對應：

| 輸出格式 | IT2D 硬體對應 | 效果 |
|---------|--------------|------|
| `--grayscale` | `ite_it2d_recolor` | 一張灰階圖可被 GPU 即時染成任意顏色，不需多套彩色 PNG |
| `--chroma-key` | `ite_it2d_color_key` | 硬體去背，比軟體 alpha blend 快 10 倍 |
| `--argb8888` | IT2D ARGB surface | 完整透明通道，用於圖層疊加 |

**使用範例**：

```bash
# 單檔轉換
python tools/assets_pipeline.py icon.png -o output/assets/

# 轉換為灰階（給 IT2D 硬體變色用）
python tools/assets_pipeline.py ring.png --grayscale -o output/assets/

# Chroma key 去背
python tools/assets_pipeline.py overlay.png --chroma-key --chroma-color #FF00FF -o output/assets/

# 縮放 + 指定格式
python tools/assets_pipeline.py bg.png --resize 480x480 --rgb565 -o output/assets/

# 批次：掃描整個目錄
python tools/assets_pipeline.py --all --input-dir UI/raw/ -o output/assets/

# 產生 manifest 清單模板
python tools/assets_pipeline.py --gen-manifest --input-dir UI/ -o assets.json
```

---

### 2. `tools/json2lvgl.py` — JSON 設計稿轉 LVGL C 程式碼

**用途**：將遵循 `lvgl_ui_schema.json` 規範的 JSON UI 設計檔，自動產生完整的 LVGL C 原始碼（`.c` + `.h`）。

**為什麼需要它**：手寫 LVGL C 程式碼繁瑣且容易出錯，特別是多層次 UI、動畫、狀態機的程式碼量很大。JSON 描述比 C 程式碼簡潔 10 倍，且可以被 Figma 外掛或其他設計工具自動產生。

**功能**：

| 功能 | JSON Schema 關鍵字 | 產生的 C 程式碼 |
|------|-------------------|----------------|
| Widget 創建 | `type`, `x`, `y`, `w`, `h` | `lv_obj_create()`, `lv_obj_set_pos()`, `lv_obj_set_size()` |
| 樣式設定 | `style` | `lv_obj_set_style_*()` |
| **硬體變色** | `recolor` | `lv_img_set_recolor()` — IT2D GPU 加速 |
| **光暈效果** | `glow` | `ite_it2d_surface_t` + gradient fill + alpha blend |
| **毛玻璃** | `glass` | stack blur + IT2D composite surface |
| 動畫 | `anim` | `lv_anim_t` — 旋轉、淡入淡出、縮放、數值變化 |
| 事件 | `events` | `lv_obj_add_event_cb()` |
| 多狀態 | `states` | 狀態機 enum + `ui_apply_state()` 切換函數 |
| 子母元件 | `children` | 巢狀 widget tree |
| 設計 Token | `design_tokens.colors` | `{token_name}` 參照解析 |

**支援的 Widget 類型**（共 22 種）：

`screen`, `container`, `panel`, `label`, `btn`, `image`, `arc`, `bar`, `slider`, `switch`, `checkbox`, `dropdown`, `roller`, `textarea`, `keyboard`, `list`, `table`, `chart`, `spinner`, `tabview`, `msgbox`, `canvas`, `line`, `led`, `imgbtn`, `calendar`, `meter`, `circle_progress`

**支援的動畫類型**：

| 動畫類型 | 說明 | IT2D 硬體加速 |
|---------|------|:---:|
| `rotation` | 圖片/元件旋轉 | ✅ `ite_it2d_transform_copy` |
| `alpha_pulse` | 透明度呼吸效果 | ✅ `ite_it2d_const_alpha` |
| `scale` | 縮放動畫 | ✅ `ite_it2d_transform` |
| `value` | 數值變化（arc/bar/slider） | ✅ |
| `color_transition` | 顏色漸變 | ✅ `ite_it2d_recolor` |

**使用範例**：

```bash
# 產生 C code 到指定目錄
python tools/json2lvgl.py examples/gateway_hmi_example.json -o output/

# 預覽模式（不產生檔案，顯示 UI 統計）
python tools/json2lvgl.py examples/gateway_hmi_example.json --preview

# 輸出到 stdout（偵錯用）
python tools/json2lvgl.py examples/gateway_hmi_example.json
```

---

### 3. `workflow.py` — 一鍵自動化流水線

**用途**：將上述兩個工具 + ITE SDK 編譯流程串接為一條指令。從 JSON UI 設計稿到燒錄好的 ROM，全自動執行。

**執行階段**：

```
Stage 1: assets_pipeline.py     PNG 圖片 → LVGL C array (img_*.c, img_*.h)
Stage 2: json2lvgl.py           JSON 設計稿 → LVGL C code (screen.c, screen.h)
Stage 3: CMake + make           編譯全部原始碼 → ITE_NOR.ROM
Stage 4: burn_rom.py (可選)     USB 燒錄到 IT9866
```

**使用範例**：

```bash
# 最常用：完整流程
python workflow.py examples/gateway_hmi_example.json

# 只看 UI 統計 + 資產清單
python workflow.py examples/gateway_hmi_example.json --preview

# 產生程式碼但不編譯（開發中快速迭代）
python workflow.py my_ui.json --skip-build

# 完整流程 + 自動燒錄
python workflow.py my_ui.json --flash

# 跳過資產轉換（圖片沒改時）
python workflow.py my_ui.json --skip-assets

# 指定專案目錄
python workflow.py my_ui.json --project-dir D:/MyProject/

# 完整輸出
python workflow.py my_ui.json -v
```

---

## JSON Schema 規範

`schema/lvgl_ui_schema.json` 定義了 JSON UI 設計檔的完整格式規範，包含：

- **顯示設定** (`display`)：解析度、色深、更新率
- **設計 Token** (`design_tokens`)：顏色、字型、漸層、時間常數的集中定義
- **Widget 樹** (`screen.children`)：巢狀的 UI 元件樹
- **IT2D GPU 特效**：`recolor`, `glow`, `glass`, `gradient`, `shadow`, `composite`
- **動畫系統** (`anim`)：旋轉、縮放、透明度、數值動畫，含 easing 曲線
- **狀態機** (`states`)：多狀態定義與覆寫規則
- **事件系統** (`events`)：點擊、長按、數值變化等觸發

---

## 範例：Gateway HMI 雙環儀表

`examples/gateway_hmi_example.json` 是一個完整的 Gateway 狀態顯示 HMI，包含：

| 元件 | 說明 | IT2D 特效 |
|------|------|----------|
| WiFi 外環 | 青色旋轉光環，40 秒循環 | `recolor` + `gradient_ring` glow |
| BLE 內環 | 藍色逆向旋轉，30 秒循環 | `recolor` + `gradient_ring` glow |
| 中心圖示 | 路由器 / 警告圖示切換 | `stack_blur` glow + opacity pulse |
| 毛玻璃面板 ×2 | WiFi / BLE 狀態資訊 | `glass` blur cache + alpha blend |
| 觸發按鈕 | 切換正常 / 異常狀態 | `gradient_fill` glow + hover 狀態 |
| **異常狀態** | 全部元件變橘紅色 | 硬體 recolor 即時切換，無需第二套圖片 |

---

## 到 IT9866 的完整路徑

```
UI 設計師編寫 JSON
       │
       ▼
┌─────────────────────┐
│ workflow.py          │  ← 你在這裡執行
│ python workflow.py   │
│   my_ui.json         │
└─────────┬───────────┘
          │
    ┌─────┴─────┐
    │           │
    ▼           ▼
assets/     screen.c
img_*.c     screen.h
img_*.h
    │           │
    └─────┬─────┘
          │
          ▼
    CMake + make
    (ITEGCC cross-compile)
          │
          ▼
    ITE_NOR.ROM
          │
          ▼
    burn_rom.py
    (USB flash)
          │
          ▼
    ┌──────────────┐
    │  IT9866 硬體   │
    │  480×480 LCD  │
    │  IT2D GPU     │
    │  雙環 HMI     │
    └──────────────┘
```

---

## 與 LVGL MCP Server 的整合

`lvgl-mcp-server` (v2.0.0) 可以在 PC 上進行 LVGL 的 headless 渲染。工作流如下：

```bash
# 1. 產生 LVGL C code
python tools/json2lvgl.py my_ui.json -o output/

# 2. 將 screen.c 餵給 LVGL MCP Server
#    MCP 會在 PC 上用 SDL 模擬 LVGL，回傳截圖

# 3. 比對截圖與設計稿
#    若一致 → 編譯燒錄
#    若不一致 → 修改 JSON → 回到步驟 1
```

---

## 常見問題

**Q: 為什麼用 JSON 而不是 HTML？**
A: HTML/CSS 的 flexbox/grid 佈局模型與 LVGL 的絕對定位有根本差異，直接轉換會大量失真。JSON Schema 的欄位與 LVGL API 是 1:1 對應的，零失真。

**Q: 設計師不會寫 JSON 怎麼辦？**
A: JSON 的結構非常直觀（`type: "label"`, `text: "Hello"`, `x: 100, y: 200`）。未來可以開發 Figma plugin 自動輸出此格式。

**Q: 我的圖片很大，產生的 C array 會多大？**
A: 一張 480×480 的 RGB565 圖片約 460 KB。NOR Flash 通常有 16MB+，足夠存放數十張圖片。建議用 `--grayscale` + IT2D recolor 減少需要多套彩色圖片的需求。

**Q: 可以在 Linux/Mac 上跑嗎？**
A: `json2lvgl.py` 和 `assets_pipeline.py` 是純 Python，跨平台。但 Stage 3 的 CMake + ITEGCC 編譯需要在 Windows 上執行（ITEGCC 是 Windows 工具鏈）。

**Q: 如何新增自訂的 Widget 或特效？**
A: 修改 `tools/json2lvgl.py` 中的 `generate_widget()` 函數，加入新的 `creators` dict 項目即可。
