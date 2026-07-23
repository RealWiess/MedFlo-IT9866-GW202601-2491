# MedFlow Gateway UI 實作指南 (雙環科技風)

本目錄存放了針對 MedFlow Gateway **480x480 像素正方形 LCD 螢幕** 設計的 UI 概念圖與相關實作規格。

## 📁 目錄內容
*   `ui_design_three_styles_comparison.png`：三種原始風格提案對照圖 (科技暗黑、極簡亮色、醫療儀表板)。
*   `ui_concept.png`：方案 A (單進度條) 原始暗黑風概念圖。
*   `ui_concept_double_ring.png`：方案 A - 同心雙環設計概念圖 (Concentric Dual Rings)。
*   `ui_concept_error_state.png`：方案 A - 同心雙環異常/故障狀態概念圖。
*   `ui_concept_vgh_concentric.png`：方案 A (同心雙環) - 融入榮總院徽 (VGH Logo) 之設計概念圖。
*   `ui_concept_sbs_double_ring.png`：方案 B - 左右對稱雙環設計概念圖 (Side-by-Side Dual Rings)。
*   `ui_concept_sbs_error_state.png`：方案 B - 左右雙環異常/故障狀態概念圖。
*   `ui_concept_vgh_sbs.png`：方案 B (左右雙環) - 融入榮總院徽 (VGH Logo) 之設計概念圖。
*   `README.md`：本實作與說明文件。

---

## 🎨 方案設計概念與擺放樣式

不論您選擇 **同心雙環 (Concentric)** 還是 **左右雙環 (Side-by-Side)**，底層 C 語言程式碼 (`scene.c`) 的控制邏輯是**完全共用且一致的**（都是尋找並更新對應的 `ituText`、`ituCircleProgressBar` 元件）。您可以直接在 ITE GUI Designer 中決定要採用哪種排版樣式。

### 1. 同心雙環樣式 (Concentric Rings)
*   **視覺風格**：中心有兩個大小不同的同心圓環。外環（青色）代表 Wi-Fi 狀態，內環（藍色）代表 Bluetooth 狀態，最中心為網關圖案。
*   **素材要求**：需要一個較大的外環圖檔（如 `wifi_ring.png`）和一個較小的內環圖檔（如 `ble_ring.png`）。

### 2. 左右雙環樣式 (Side-by-Side Left-Right Rings)
*   **視覺風格**：螢幕左右對稱分布兩個大小相同的進度環。左環（青色）內嵌 WiFi 圖示，右環（藍色）內嵌 Bluetooth 圖示。下方各自呈現 SSID/IP 與連線裝置數。
*   **素材要求**：WiFi 環與 Bluetooth 環尺寸相同，可共用進度條背景或直接在圖檔中畫好。

---

## 🛠️ 第一步：在 ITE GUI Designer 中拉設元件

請在 Windows 電腦上開啟 `GUIDesigner.exe`（載入專案的 `project/GW202601/itu/1200x1920/automotive_display.xml` 佈局檔），並在 **`demoLayer`** 中進行以下設定：

1.  **刪除舊文字框**：刪除原有的 `TextBox1`。
2.  **更換背景**：將 `fakeBackground` 的背景圖設為您設計的 `bg_dark.png`。
3.  **新增以下文字元件 (ITUText)**：
    *   **`wifi_ssid_txt`**：用來顯示 WiFi SSID。
    *   **`wifi_ip_txt`**：用來顯示 WiFi IP。
    *   **`ble_count_txt`**：用來顯示已掃描到的 BLE 設備數（如 "12 Devices"）。
4.  **新增雙進度環元件 (ITUCircleProgressBar)**（同心重疊放置）：
    *   **`wifi_status_ring`**（外環）：前景貼圖設為 `wifi_ring.png`，外徑較大。
    *   **`ble_scan_ring`**（內環）：前景貼圖設為 `ble_ring.png`，外徑較小。

---

## 💻 第二步：C 語言程式碼對接 (scene.c)

當您在 GUI 介面工具中設計好元件，並儲存/編譯產生新的 `automotive_display.itu` 後。請通知 AI，我們將會把以下代碼應用於 `project/GW202601/scene.c` 的 `StatusDisplay_Update` 函數中：

```c
static void StatusDisplay_Update(void)
{
    uint32_t now = SDL_GetTicks();
    if (now - s_status_last_update < STATUS_UPDATE_MS)
        return;
    s_status_last_update = now;

    uint8_t flags = nmgw_status_get_flags();
    int medflo_count = nmgw_medflo_device_count();
    char ip[16] = "0.0.0.0";
    char ssid[64] = "Not Connected";

    bt_wifi_get_ip(ip, sizeof(ip));
    if (theConfig.ssid[0]) {
        strncpy(ssid, theConfig.ssid, sizeof(ssid) - 1);
    }

    // 1. 尋找畫面上的控制元件
    ITUText* wifi_ssid_txt = (ITUText*)ituSceneFindWidget(&theScene, "wifi_ssid_txt");
    ITUText* wifi_ip_txt = (ITUText*)ituSceneFindWidget(&theScene, "wifi_ip_txt");
    ITUText* ble_count_txt = (ITUText*)ituSceneFindWidget(&theScene, "ble_count_txt");

    ITUCircleProgressBar* wifi_status_ring = (ITUCircleProgressBar*)ituSceneFindWidget(&theScene, "wifi_status_ring");
    ITUCircleProgressBar* ble_scan_ring = (ITUCircleProgressBar*)ituSceneFindWidget(&theScene, "ble_scan_ring");

    // 2. 更新文字內容
    if (wifi_ssid_txt) ituTextSetString(wifi_ssid_txt, ssid);
    if (wifi_ip_txt) ituTextSetString(wifi_ip_txt, ip);
    
    if (ble_count_txt) {
        char count_buf[16];
        snprintf(count_buf, sizeof(count_buf), "%d Devices", medflo_count);
        ituTextSetString(ble_count_txt, count_buf);
    }

    // 3. 控制 WiFi 外環狀態 (連線成功時 100% 常亮，連線中/斷線時閃爍或設為 0)
    if (wifi_status_ring) {
        bool wifi_ok = (flags & NMGW_FLAG_WIFI_OK);
        if (wifi_ok) {
            ituCircleProgressBarSetValue(wifi_status_ring, 100);
            ituWidgetSetVisible(wifi_status_ring, true);
        } else {
            // 斷線時可設為 0，或用計時器做閃爍效果
            static bool blink_toggle = false;
            blink_toggle = !blink_toggle;
            ituCircleProgressBarSetValue(wifi_status_ring, blink_toggle ? 100 : 0);
            ituWidgetSetVisible(wifi_status_ring, true);
        }
    }

    if (ble_scan_ring) {
        bool ble_ok = (flags & NMGW_FLAG_BLE_OK);
        if (ble_ok) {
            int progress = ituCircleProgressBarGetValue(ble_scan_ring);
            progress = (progress + 8) % 100; // 每秒推進 8%，循環旋轉
            ituCircleProgressBarSetValue(ble_scan_ring, progress);
            ituWidgetSetVisible(ble_scan_ring, true);
        } else {
            ituCircleProgressBarSetValue(ble_scan_ring, 0);
            ituWidgetSetVisible(ble_scan_ring, false);
        }
    }
}
```

---

## ⚠️ 第三步：異常 / 故障狀態顯示設計 (Abnormal UI Design)

為了讓現場人員能立刻發現網關異常，我們為**同心雙環**與**左右雙環**設計了以下的異常狀態對應邏輯：

### 1. 異常狀態視覺對照
*   **方案 A (同心雙環)**：參考 [ui_concept_error_state.png](file:///C:/SW%20code/source%20code/ITE9868_GWBuild_20260707/UI/ui_concept_error_state.png) 概念圖。
    *   **外環 (WiFi)**：斷線重連時變為 **橘色 (Warning)** 閃爍。
    *   **內環 (BLE)**：驅動/協議棧故障時變為 **紅色 (Error)** 常亮。
    *   **中心圖標**：自動切換為發光的 **驚嘆號 / 警告標誌 (Warning Icon)**。
*   **方案 B (左右雙環)**：參考 [ui_concept_sbs_error_state.png](file:///C:/SW%20code/source%20code/ITE9868_GWBuild_20260707/UI/ui_concept_sbs_error_state.png) 概念圖。
    *   **左環 (WiFi)**：異常時變為 **橘色 (Warning)** 閃爍，且中心 WiFi 圖標變為發光橘色驚嘆號。
    *   **右環 (BLE)**：異常時變為 **紅色 (Error)** 常亮，且中心藍牙圖標變為發光紅色警告標誌。
*   **下方狀態文字**：異常的項目會呈現紅色或橘色，並顯示具體錯誤原因（如 `WiFi: Disconnected` 或 `BLE: Driver Error`）。

### 2. ITE GUI Designer 元件配置建議
為了實作變色與警告圖標，我們建議在 GUI 中多拉設這幾個重疊元件：
*   **外環 (WiFi ProgressBar)**：
    *   `wifi_status_ring` (青色 - 正常狀態)
    *   `wifi_warn_ring` (橘色 - 重新連線中)
*   **內環 (BLE ProgressBar)**：
    *   `ble_scan_ring` (藍色 - 正常掃描中)
    *   `ble_err_ring` (紅色 - 驅動/協議棧故障)
*   **中心警告圖示 (ITUImage)**：
    *   `gateway_center_icon` (網關圖案 - 正常狀態)
    *   `warn_center_icon` (驚嘆號/三角警告標誌 - 異常狀態)

### 3. C 語言端切換邏輯 (scene.c)
在程式碼中，我們只要根據底層狀態 Flag，用 `ituWidgetSetVisible` 來動態顯示對應顏色與形狀的進度條和圖標。例如：
```c
bool wifi_ok = (flags & NMGW_FLAG_WIFI_OK);
bool wifi_connecting = (flags & NMGW_FLAG_WIFI_CONNECTING);

if (wifi_ok) {
    ituWidgetSetVisible(wifi_status_ring, true);  // 顯示青色環
    ituWidgetSetVisible(wifi_warn_ring, false);   // 隱藏橘色環
} else if (wifi_connecting) {
    ituWidgetSetVisible(wifi_status_ring, false);  
    ituWidgetSetVisible(wifi_warn_ring, true);    // 顯示橘色警告環（可配合閃爍）
} else {
    ituWidgetSetVisible(wifi_status_ring, false);
    ituWidgetSetVisible(wifi_warn_ring, false);   // 斷開時全暗
}
```
這種「多重疊元件、動態控制 Visible」的作法，是 ITE 系統中最節省 CPU 運算資源、且最不容易出錯的介面實作方式。

```
