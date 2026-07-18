# ITE SoC 技術資料索引 (AI Reference Index)

> **根目錄**：`C:\公司\電子元件\ITE`
> **最後更新**：2026-07-11
> **目的**：供 AI 助手快速定位 ITE SoC 相關的 Datasheet、HDK 硬體設計文件、SDK 軟體開發套件、問答文件等。

---

## 目錄結構總覽

```
C:\公司\電子元件\ITE\
├── IT6122\                          ← HDMI 轉換晶片
├── IT97x\                           ← IT97x 系列 SoC (IT972/IT976/IT978)
│   ├── HDK\                         ← 硬體設計套件 (7z 壓縮)
│   └── datasheet\                   ← Datasheet 資料表
├── IT983x\                          ← IT983x 系列測試板設計檔
├── IT986x\                          ← IT986x 系列 SoC（主力開發平台）
│   ├── Datasheet\                   ← 各模組暫存器規格書 (29 份)
│   ├── HDK\                         ← 硬體設計套件 V1.1.7 (含 Q&A、AP Note、EVB)
│   └── SDK\                         ← 軟體開發套件 V2.4.9.1 (含 GCC 工具鏈)
├── [根目錄散落檔案]                  ← 總表 Datasheet、GPIO 分配、產品簡報
```

---

## 1. 根目錄檔案 (`ITE\`)

| 檔案名稱 | 格式 | 大小 | 功能說明 |
|----------|------|------|---------|
| [IT986x Standard EVB GUIDE_V2_GPIO.xls](file:///C:/公司/電子元件/ITE/IT986x Standard EVB GUIDE_V2_GPIO.xls) | XLS | 76 KB | IT986x Standard EVB V2 的 GPIO 腳位分配表，定義各 GPIO 功能 |
| [IT986x Standard EVB USER GUIDE_V2 .pdf](file:///C:/公司/電子元件/ITE/IT986x Standard EVB USER GUIDE_V2 .pdf) | PDF | 3.8 MB | IT986x Standard EVB V2 使用者手冊，包含板子介紹、接口說明、操作指引 |
| [IT986x-AT_Datasheet.pdf](file:///C:/公司/電子元件/ITE/IT986x-AT_Datasheet.pdf) | PDF | 2.3 MB | IT986x-AT **車用等級** SoC 總體 Datasheet，含 Pinout、電氣特性、功能方塊圖 |
| [IT986x_Datasheet.pdf](file:///C:/公司/電子元件/ITE/IT986x_Datasheet.pdf) | PDF | 2.3 MB | IT986x 消費性/工控級 SoC 總體 Datasheet |
| [IT986x_Datasheet_1.pdf](file:///C:/公司/電子元件/ITE/IT986x_Datasheet_1.pdf) | PDF | 1.9 MB | IT986x Datasheet 補充版本 |
| [IT986x_GPIO_Assignment_20220921.xls](file:///C:/公司/電子元件/ITE/IT986x_GPIO_Assignment_20220921.xls) | XLS | 71 KB | IT986x GPIO 腳位功能分配總表 (2022/09/21 版本) |
| [ITE BU1 Profile Q3 2023_Kaga.pdf](file:///C:/公司/電子元件/ITE/ITE BU1 Profile Q3 2023_Kaga.pdf) | PDF | 2.3 MB | ITE BU1 事業部產品簡介 (2023 Q3)，包含產品線總覽 |
| [ITE official_readme.pdf](file:///C:/公司/電子元件/ITE/ITE official_readme.pdf) | PDF | 309 KB | ITE 官方 SDK/工具使用 readme |
| [ITE車用產品集錦_0814_public.pptx](file:///C:/公司/電子元件/ITE/ITE車用產品集錦_0814_public.pptx) | PPTX | 263 MB | ITE 車用產品集錦簡報，涵蓋車載 SoC、影像處理、儀表板等應用案例 |
| [手機投屏導航流程demo.mp4](file:///C:/公司/電子元件/ITE/手機投屏導航流程demo.mp4) | MP4 | 198 MB | 手機無線投屏 + 導航應用 Demo 影片 |

---

## 2. IT6122 — HDMI 轉換器晶片

**路徑**：`ITE\IT6122\`
**晶片簡介**：IT6122 是一顆 HDMI 轉換器/接收器 IC，常用於 HDMI 訊號轉換至 LVDS/TTL 等面板介面。

| 檔案名稱 | 格式 | 大小 | 功能說明 |
|----------|------|------|---------|
| [IT61223FN_V11_20200902.DSN](file:///C:/公司/電子元件/ITE/IT6122/IT61223FN_V11_20200902.DSN) | DSN | 254 KB | IT61223FN 參考電路原理圖 (OrCAD Capture 格式)，V1.1 版 |
| [IT61223FN_V11_20200902.pdf](file:///C:/公司/電子元件/ITE/IT6122/IT61223FN_V11_20200902.pdf) | PDF | 54 KB | IT61223FN 參考電路原理圖 PDF 輸出版本 |
| [IT6122_Datasheet_V0.90.pdf](file:///C:/公司/電子元件/ITE/IT6122/IT6122_Datasheet_V0.90.pdf) | PDF | 1.3 MB | IT6122 Datasheet V0.90，含功能說明、暫存器定義、腳位表、電氣特性 |
| [IT6122_design_guide_V10.pdf](file:///C:/公司/電子元件/ITE/IT6122/IT6122_design_guide_V10.pdf) | PDF | 92 KB | IT6122 硬體設計指南 V1.0，含 BOM、Layout 注意事項 |

---

## 3. IT97x — 車用/工控 AP SoC 系列

**路徑**：`ITE\IT97x\`
**晶片簡介**：IT97x 系列包含 IT972、IT976、IT978 等 ARM-based AP SoC，用於車用資訊娛樂、HMI 與工業控制場景。支援多種 Display 介面、Video Input、Ethernet 等。

### 3.1 Datasheet (`IT97x\datasheet\`)

| 檔案名稱 | 格式 | 大小 | 功能說明 |
|----------|------|------|---------|
| [IT972_Datasheet_20251111.pdf](file:///C:/公司/電子元件/ITE/IT97x/datasheet/IT972_Datasheet_20251111.pdf) | PDF | 2.1 MB | IT972 SoC Datasheet (2025/11/11 版)，含完整 Pinout、功能方塊圖、電氣規格 |
| [IT976_Datasheet_V0.0.1_20201125.pdf](file:///C:/公司/電子元件/ITE/IT97x/datasheet/IT976_Datasheet_V0`0`1_20201125.pdf) | PDF | 2.3 MB | IT976 SoC Datasheet 初版 |
| [IT976_IT978_Datasheet_20240711.pdf](file:///C:/公司/電子元件/ITE/IT97x/datasheet/IT976_IT978_Datasheet_20240711.pdf) | PDF | 2.3 MB | IT976/IT978 合併 Datasheet (2024/07/11 版)，涵蓋兩款晶片差異比較 |

### 3.2 HDK 硬體設計套件 (`IT97x\HDK\`)

| 檔案名稱 | 格式 | 大小 | 功能說明 |
|----------|------|------|---------|
| [BU5_HW_970A12.pdf](file:///C:/公司/電子元件/ITE/IT97x/HDK/BU5_HW_970A12.pdf) | PDF | 105 KB | IT97x 硬體設計應用筆記 |
| Chinese_IT97x_APSoC_HDK_V1.1.7.7z | 7Z | 100 MB | IT97x 系列完整硬體設計套件壓縮檔 V1.1.7，含原理圖、PCB Layout、AP Note 等（需解壓縮） |

---

## 4. IT983x — 測試板設計檔

**路徑**：`ITE\IT983x\`
**晶片簡介**：IT983x 系列是 ITE 較早期的 SoC，以下為其 Test Board / EVB 的設計檔案。

### 4.1 CON2 SD-CARD/WiFi 切換測試板

| 檔案名稱 | 格式 | 大小 | 功能說明 |
|----------|------|------|---------|
| [IT983X_CON2_SDCARD-WIFI SELECT20211001.DSN](file:///C:/公司/電子元件/ITE/IT983x/IT983X_CON2_SDCARD-WIFI SELECT20211001.DSN) | DSN | 967 KB | CON2 SD/WiFi 切換板原理圖 (OrCAD) |
| [IT983X_CON2_SDCARD-WIFI SELECT20211001.asc](file:///C:/公司/電子元件/ITE/IT983x/IT983X_CON2_SDCARD-WIFI SELECT20211001.asc) | ASC | 420 KB | CON2 板 Netlist 文字檔 |
| [IT983X_CON2_SDCARD-WIFI SELECT20211001.pcb](file:///C:/公司/電子元件/ITE/IT983x/IT983X_CON2_SDCARD-WIFI SELECT20211001.pcb) | PCB | 446 KB | CON2 板 PCB Layout 檔 (Allegro/PADS) |
| [IT983X_CON2_SDCARD-WIFI SELECT20211001.pdf](file:///C:/公司/電子元件/ITE/IT983x/IT983X_CON2_SDCARD-WIFI SELECT20211001.pdf) | PDF | 125 KB | CON2 板原理圖 PDF 輸出 |
| [IT983X_CON2_SDCARD-WIFI Test Board Guide.pdf](file:///C:/公司/電子元件/ITE/IT983x/IT983X_CON2_SDCARD-WIFI Test Board Guide.pdf) | PDF | 635 KB | CON2 SD/WiFi 切換測試板使用指南 |
| [IT983X_CON2_SDCARD-WIFI Test Board Guide.pptx](file:///C:/公司/電子元件/ITE/IT983x/IT983X_CON2_SDCARD-WIFI Test Board Guide.pptx) | PPTX | 993 KB | CON2 測試板使用指南 PPT 版 |

### 4.2 CON3 SDIO WiFi Combo 測試板

| 檔案名稱 | 格式 | 大小 | 功能說明 |
|----------|------|------|---------|
| [IT983X_CON3_SDIO WIFI_COMBO Guide.pdf](file:///C:/公司/電子元件/ITE/IT983x/IT983X_CON3_SDIO WIFI_COMBO Guide.pdf) | PDF | 558 KB | CON3 SDIO WiFi Combo 板使用指南 |
| [IT983X_CON3_SDIO WIFI_COMBO Guide.pptx](file:///C:/公司/電子元件/ITE/IT983x/IT983X_CON3_SDIO WIFI_COMBO Guide.pptx) | PPTX | 3.7 MB | CON3 SDIO WiFi Combo 板使用指南 PPT 版 |
| [IT983X_CON3_SDIO WIFI_COMBO20211001.DSN](file:///C:/公司/電子元件/ITE/IT983x/IT983X_CON3_SDIO WIFI_COMBO20211001.DSN) | DSN | 978 KB | CON3 SDIO WiFi Combo 板原理圖 |
| [IT983X_CON3_SDIO WIFI_COMBO20211001.asc](file:///C:/公司/電子元件/ITE/IT983x/IT983X_CON3_SDIO WIFI_COMBO20211001.asc) | ASC | 430 KB | CON3 板 Netlist 文字檔 |
| [IT983X_CON3_SDIO WIFI_COMBO20211001.pcb](file:///C:/公司/電子元件/ITE/IT983x/IT983X_CON3_SDIO WIFI_COMBO20211001.pcb) | PCB | 490 KB | CON3 板 PCB Layout 檔 |
| [IT983X_CON3_SDIO WIFI_COMBO20211001.pdf](file:///C:/公司/電子元件/ITE/IT983x/IT983X_CON3_SDIO WIFI_COMBO20211001.pdf) | PDF | 169 KB | CON3 板原理圖 PDF 輸出 |

### 4.3 General EVB

| 檔案名稱 | 格式 | 大小 | 功能說明 |
|----------|------|------|---------|
| [IT983X_GENERAL EVB-V1.1.DSN](file:///C:/公司/電子元件/ITE/IT983x/IT983X_GENERAL EVB-V1.1.DSN) | DSN | 2.8 MB | IT983X 通用 EVB V1.1 原理圖 |
| [IT983X_General EVB-V1.1.asc](file:///C:/公司/電子元件/ITE/IT983x/IT983X_General EVB-V1.1.asc) | ASC | 2.6 MB | 通用 EVB V1.1 Netlist |
| [IT983X_General EVB-V1.1.pcb](file:///C:/公司/電子元件/ITE/IT983x/IT983X_General EVB-V1.1.pcb) | PCB | 2.4 MB | 通用 EVB V1.1 PCB Layout |
| [IT983X_General EVB-V1.1.pdf](file:///C:/公司/電子元件/ITE/IT983x/IT983X_General EVB-V1.1.pdf) | PDF | 457 KB | 通用 EVB V1.1 原理圖 PDF 輸出 |

---

## 5. IT986x — 主力開發平台 (完整資料)

**路徑**：`ITE\IT986x\`
**晶片簡介**：IT986x 系列（含 IT9860/IT9862/IT9866/IT9868）是 ITE 主力 ARM Cortex 架構 AP SoC，支援 DDR2/DDR3 記憶體、LVDS/MIPI/RGB 顯示輸出、Video Capture、CAN-FD、USB、Ethernet、SD/eMMC、SPI NOR/NAND 等豐富周邊。車用版 IT986x-AT 通過 AEC-Q100 車規認證。

### 5.1 模組暫存器規格書 (`IT986x\Datasheet\`)

> [!TIP]
> 這些是最重要的底層暫存器規格文件，用於驅動開發和除錯時查閱各硬體模組的暫存器位址、位元定義和時序要求。

#### 5.1.1 處理器與系統

| 檔案名稱 | 功能說明 |
|----------|---------|
| [IT986x - ARM.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - ARM.pdf) | ARM 核心規格，含 Cache、MMU、中斷等系統級暫存器 |
| [IT986x - AMBA - DMA (AXI).pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - AMBA - DMA (AXI).pdf) | AMBA AXI 總線 DMA 控制器暫存器規格 |
| [IT986x - Memory Controller.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - Memory Controller.pdf) | DDR2/DDR3 記憶體控制器暫存器規格 (5.1 MB) |
| [IT986x - Command Queue.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - Command Queue.pdf) | Command Queue 引擎暫存器規格 |
| [IT986x - Interrupt Controller.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - Interrupt Controller.pdf) | 中斷控制器暫存器規格 |
| [IT986x - General Register Spec.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - General Register Spec.pdf) | 通用暫存器規格總覽 (4.4 MB)，含 Clock、Reset、Power 等系統暫存器 |

#### 5.1.2 顯示與影像

| 檔案名稱 | 功能說明 |
|----------|---------|
| [IT986x - LCD Controller.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - LCD Controller.pdf) | LCD 控制器暫存器規格，含 RGB/LVDS 輸出控制 |
| [IT986x - MIPI D-PHY.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - MIPI D-PHY.pdf) | MIPI D-PHY 物理層暫存器規格 |
| [IT986x - MIPI DSI Host Controller.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - MIPI DSI Host Controller.pdf) | MIPI DSI 主控端暫存器規格 |
| [IT986x - DPU.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - DPU.pdf) | 顯示處理單元 (Display Processing Unit) 暫存器規格 |
| [IT986x - Video Capture.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - Video Capture.pdf) | 視訊擷取模組暫存器規格 (支援 CVBS/BT.656/BT.1120 等) |
| [IT986x - Vidoe Processor.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - Vidoe Processor.pdf) | 視訊處理器暫存器規格 (色彩空間轉換、縮放等) |
| [IT986x - JPEG Engine.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - JPEG Engine.pdf) | 硬體 JPEG 編解碼引擎暫存器規格 |

#### 5.1.3 通訊介面

| 檔案名稱 | 功能說明 |
|----------|---------|
| [IT986x - UART.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - UART.pdf) | UART 序列埠暫存器規格 |
| [IT986x - SPI.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - SPI.pdf) | SPI 介面暫存器規格 |
| [IT986x - IIC.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - IIC.pdf) | I²C 介面暫存器規格 |
| [IT986x - IIS Controller.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - IIS Controller.pdf) | I²S 音訊介面控制器暫存器規格 |
| [IT986x - CANBUS_FD.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - CANBUS_FD.pdf) | CAN / CAN-FD 控制器暫存器規格 (3.9 MB) |
| [IT986x - USB.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - USB.pdf) | USB Host/Device 控制器暫存器規格 (5.1 MB) |
| [IT986x - SD Controller.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - SD Controller.pdf) | SD/SDIO/eMMC 控制器暫存器規格 (4.5 MB) |

#### 5.1.4 外設與計時器

| 檔案名稱 | 功能說明 |
|----------|---------|
| [IT986x - GPIO.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - GPIO.pdf) | GPIO 通用輸入輸出暫存器規格 |
| [IT986x - Timer.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - Timer.pdf) | 計時器暫存器規格 |
| [IT986x - Watch Dog Timer.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - Watch Dog Timer.pdf) | 看門狗計時器暫存器規格 |
| [IT986x - Real Time Clock.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - Real Time Clock.pdf) | RTC 實時時鐘暫存器規格 |
| [IT986x - SARADC Controller.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - SARADC Controller.pdf) | SAR ADC 類比數位轉換器暫存器規格 |
| [IT986x - Remote Controller.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - Remote Controller.pdf) | 紅外線遙控器接收暫存器規格 |

#### 5.1.5 儲存控制

| 檔案名稱 | 功能說明 |
|----------|---------|
| [IT986x - AXI SPI Flash Controller.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x - AXI SPI Flash Controller.pdf) | AXI SPI NOR Flash 控制器暫存器規格 |

#### 5.1.6 Datasheet 總覽

| 檔案名稱 | 功能說明 |
|----------|---------|
| [IT986x_Datasheet.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x_Datasheet.pdf) | IT986x 消費/工控級 SoC 總體 Datasheet |
| [IT986x-AT_Datasheet.pdf](file:///C:/公司/電子元件/ITE/IT986x/Datasheet/IT986x-AT_Datasheet.pdf) | IT986x-AT 車規級 SoC 總體 Datasheet |

---

### 5.2 HDK 硬體設計套件 (`IT986x\HDK\`)

#### 5.2.1 根目錄問題排除快速指南

> [!IMPORTANT]
> 以下 PDF 文件是 **常見硬體問題的快速排除指南**，遇到硬體異常時優先查閱。

| 檔案名稱 | 功能說明 |
|----------|---------|
| [IO端似有干擾導致外接設備會誤動作.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/IO端似有干擾導致外接設備會誤動作.pdf) | GPIO 干擾導致外接設備誤動作問題排查 |
| [PCB_Layout出圖前基本自檢流程.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/PCB_Layout出圖前基本自檢流程.pdf) | PCB Layout 出圖前的基本自檢 Checklist |
| [如何透過SPI_Tool查詢PCB的開機模式.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/如何透過SPI_Tool查詢PCB的開機模式.pdf) | 使用 SPI Tool 查詢 PCB Boot Mode |
| [如何透過SPI_Tool查詢開機失敗的趨向.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/如何透過SPI_Tool查詢開機失敗的趨向.pdf) | 使用 SPI Tool 分析開機失敗原因 |
| [如何透過SPI_Tool測試memory運行狀態.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/如何透過SPI_Tool測試memory運行狀態.pdf) | 使用 SPI Tool 測試 DDR 記憶體運行狀態 |
| [晶片動作正常_但是晶片本體異常發燙.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/晶片動作正常_但是晶片本體異常發燙.pdf) | 晶片過熱問題排查指南 |
| [燒錄韌體後_系統開機無反應也無信息.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/燒錄韌體後_系統開機無反應也無信息.pdf) | 韌體燒錄後無法開機問題排查 |
| [系統正常運行_但開啟其他用電設備時引起當機或重啟.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/系統正常運行_但開啟其他用電設備時引起當機或重啟.pdf) | 電源耦合導致系統不穩問題排查 |
| [系統無法開機_需卸除部分用電零組件方可開機.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/系統無法開機_需卸除部分用電零組件方可開機.pdf) | 電源負載過重導致無法開機排查 |
| [系統運行不穩_莫名當機,開機成功率偏低.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/系統運行不穩_莫名當機,開機成功率偏低.pdf) | 系統不穩定、隨機當機問題排查 |
| [芯片工作溫度與公版相比明顯偏高.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/芯片工作溫度與公版相比明顯偏高.pdf) | 散熱異常排查 |
| [透過X-Ray圖片簡易判別SoC_EPAD焊接狀態.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/透過X-Ray圖片簡易判別SoC_EPAD焊接狀態.pdf) | 透過 X-Ray 檢查 SoC EPAD 焊接品質 |
| [透過開發工具燒錄NOR_NAND會異常失敗.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/透過開發工具燒錄NOR_NAND會異常失敗.pdf) | Flash 燒錄異常失敗排查 |
| [開機狀態時好時壞_成功率偏低.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/開機狀態時好時壞_成功率偏低.pdf) | 開機成功率不穩定問題排查 |
| [開發工具無法抓取SoC的Chip_ID.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/開發工具無法抓取SoC的Chip_ID.pdf) | SPI Tool 無法讀取 Chip ID 問題排查 |
| [開發工具無法正確抓取NOR_NAND的ID及相關資料.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/開發工具無法正確抓取NOR_NAND的ID及相關資料.pdf) | Flash ID 讀取失敗排查 |

---

#### 5.2.2 HDK V1.1.7 完整套件 (`Chinese_IT986x_SoC_HDK_V1.1.7\`)

##### 設計指南 (Design Guide)

| 檔案名稱 | 功能說明 |
|----------|---------|
| [IT986x_硬件開發指南_V110.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/IT986x_硬件開發指南_V110.pdf) | IT986x **硬體開發指南 V1.10**（消費/工控級），含電源設計、時脈、DDR Layout、周邊電路要點 (13 MB) |
| [IT986x-AT車用芯片_硬件開發指南_V111.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/IT986x-AT車用芯片_硬件開發指南_V111.pdf) | IT986x-AT **車用級硬體開發指南 V1.11**，額外包含 MCU 備援、車規電源、低功耗待機等 (13 MB) |
| [IT986X SoC Schematic Design Guide.ppsx](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/IT986X SoC Schematic Design Guide.ppsx) | 原理圖設計指南投影片，含原理圖重點標注 |
| [IT986X SoC PCB Layout Design Guide.ppsx](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/IT986X SoC PCB Layout Design Guide.ppsx) | PCB Layout 設計指南投影片，含走線、疊層、阻抗等設計規範 |
| [IT986x_GPIO_Assignment_20230322.xls](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/IT986x_GPIO_Assignment_20230322.xls) | GPIO 腳位分配表 (2023/03/22 版) |
| [ITE原理圖協助檢視需求表.docx](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/ITE原理圖協助檢視需求表.docx) | 送 ITE 原廠審閱原理圖前須填寫的需求表 |
| [Release Note.txt](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/Release Note.txt) | HDK 各版本更新記錄 (V1.01 ~ V1.1.7) |
| [Training Video.txt](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/Training Video.txt) | 訓練影片入口：https://soc.ite.com.tw |

---

##### AP Note 應用筆記 (`AP Note\`)

> [!NOTE]
> 應用筆記涵蓋 SMT 注意事項、ADC 校正、SD 卡設計、USB 測試、RTC 電路、看門狗設計等硬體設計實務。

| 檔案名稱 | 功能說明 |
|----------|---------|
| [BU5_HW_9860A00_SMT注意事項.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/AP Note/BU5_HW_9860A00_SMT注意事項.pdf) | SoC **SMT 上件注意事項**：焊接溫度曲線、EPAD 處理、鋼網設計 |
| [BU5_HW_9860A01_ADC校正線路.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/AP Note/BU5_HW_9860A01_ADC校正線路.pdf) | **SAR ADC 校正電路**設計指南 |
| [BU5_HW_9860A05_SDCARD 線路設計.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/AP Note/BU5_HW_9860A05_SDCARD 線路設計.pdf) | **SD Card 介面電路**設計指南 |
| [BU5_HW_9860A06_USB Test JK.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/AP Note/BU5_HW_9860A06_USB Test JK.pdf) | **USB Test J/K 訊號**測試方法 |
| [BU5_HW_9860A07_RTC_模塊運用與電源處理.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/AP Note/BU5_HW_9860A07_RTC_模塊運用與電源處理.pdf) | **RTC 模塊電路**與電源處理設計 |
| [BU5_HW_9860A08_RTC 電源過低斷電處理.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/AP Note/BU5_HW_9860A08_RTC 電源過低斷電處理.pdf) | RTC 低電壓斷電保護處理 |
| [BU5_HW_9860A11_RTC 漏電處理.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/AP Note/BU5_HW_9860A11_RTC 漏電處理.pdf) | RTC 漏電問題排查 |
| [BU5-HW-9860A12_Add External Watchdog Circuit.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/AP Note/BU5-HW-9860A12_Add External Watchdog Circuit.pdf) | 外部硬體**看門狗電路**設計指南 |
| [BU5-HW-9860A13_SMT上件流程與ESD關係.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/AP Note/BU5-HW-9860A13_SMT上件流程與ESD關係.pdf) | SMT 製程與 ESD 防護的關聯 |
| [IT986x AP Note List.xlsx](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/AP Note/IT986x AP Note List.xlsx) | AP Note 索引清單 |
| `IT9860_testJ_testK_script\` | USB Test J/K 測試腳本目錄（含 DDR 參數設定腳本） |

---

##### General EVB 通用開發板 (`IT986x_General_EVB-20230425\`)

> [!TIP]
> 這是最新一代通用 EVB (2023/04/25 版)，支援 IT9862/IT9866 (DDR2) 和 IT9868 (DDR3)。

| 檔案名稱 | 功能說明 |
|----------|---------|
| [IT986x GENERAL EVB GUIDE_V3.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/IT986x_General_EVB-20230425/IT986x GENERAL EVB GUIDE_V3.pdf) | 通用 EVB V3 **使用手冊** (5 MB) |
| [IT9862_IT9866_DDR2_GENERAL EVB-20250321.DSN](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/IT986x_General_EVB-20230425/IT9862_IT9866_DDR2_GENERAL EVB-20250321.DSN) | IT9862/IT9866 (DDR2) 通用 EVB 原理圖 |
| [IT9862_IT9866_DDR2_GENERAL EVB-20250321.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/IT986x_General_EVB-20230425/IT9862_IT9866_DDR2_GENERAL EVB-20250321.pdf) | IT9862/IT9866 (DDR2) 通用 EVB 原理圖 PDF |
| [IT9868_DDR3_GENERAL EVB-20250321.DSN](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/IT986x_General_EVB-20230425/IT9868_DDR3_GENERAL EVB-20250321.DSN) | IT9868 (DDR3) 通用 EVB 原理圖 |
| [IT9868_DDR3_GENERAL EVB-20250321.pdf](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/IT986x_General_EVB-20230425/IT9868_DDR3_GENERAL EVB-20250321.pdf) | IT9868 (DDR3) 通用 EVB 原理圖 PDF |
| [IT9860_GENERAL-EVB_BOM-20230428.xlsx](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/IT986x_General_EVB-20230425/IT9860_GENERAL-EVB_BOM-20230428.xlsx) | 通用 EVB **BOM 表** |
| IT986X__GENERAL_EVB-Final-20230425.asc/pcb | 通用 EVB PCB Layout & Netlist 設計檔 |
| `Daughter board\` | **13 種子板設計**：WiFi (AP6256)、CAN-FD (ATA6570/TIJ10443/TJA1043)、Ethernet (IP101GR)、RS232、RS485、Sensor、Video (RN6752MV1)、Debug Tool V6/V7 等 |

---

##### 周邊電路參考設計 (`IT986x_Peripheral circuit\`)

| 子目錄 | 內容說明 |
|--------|---------|
| `Analog Video Interface\` | 類比視訊輸入介面參考電路，含 **ADV7180、PR2000、TP2825、TP9950、TVP5150** 等 Video Decoder 晶片的連接設計 |
| `Ethernet\` | 乙太網 PHY 參考電路，含 **DM9162、LAN8710A、LAN8720A、RTL8201F、RTL8304M、RTL8363** 等晶片 |
| `Panel Interface\` | 面板介面轉換參考電路：**LVDS→2CH LVDS**、**LVDS→RGB**、**MIPI→DPI**、**MIPI→RGB** |
| `SDIO_WiFi_CON3\` | SDIO WiFi 模組 (CON3 接口) 參考電路 |
| `Serdes Board\` | SerDes 串列器/解串器參考電路：**TI DS90UB925/926/941** 及 **Maxim MAX9275/9276/9279/9280/96706** 共 8 種組合 |

---

##### Legacy EVB 與 Demo Board (`Legacy EVB and Demo Board\`)

| 子目錄 | 內容說明 |
|--------|---------|
| `IT986x_Standard_EVB-V3\` | IT986x Standard EVB V3：原理圖 (DSN/PDF)、Layout、GPIO 分配、User Guide、SDIO WiFi Guide、子板設計 |
| `IT9868-AT_Standard_EVB-V3\` | IT9868-AT 車規 Standard EVB V3：原理圖 (DSN/PDF)、User Guide、應用模組清單、子板設計 |
| `IT986x_AT_General_EVB-V1\` | IT986x-AT 車規通用 EVB V1：原理圖 (DSN/PDF)、子板設計 |
| `IT986x-AT_DASHBOARD_V002-20241111\` | IT986x-AT **儀表板專案板** V002：原理圖 (DSN/PDF)、Dashboard Guide、子板設計 |
| `Demo Board Circuit and PCB\` | 3 款 Demo Board 完整設計：**HMI Demo**、**Indoor Demo**、**Smart Thermostat Demo** |

---

##### Q&A 常見問題集 (`Q&A\`)

> [!WARNING]
> 總計 **29 份 Q&A 文件**，依編號分類，涵蓋從 SMT、開機、燒錄到 ESD/EMI 的完整問題集。

| 編號範圍 | 主題分類 | 包含的 Q&A 文件 |
|----------|---------|----------------|
| QA01xxx | **SoC 基礎問題** | 開發工具無法抓取 Chip ID、開機成功率偏低、未使用管腳上下拉處理 |
| QA02xxx | **PCB Layout** | PCB Layout 出圖前基本自檢流程 |
| QA03xxx | **Flash 燒錄** | NOR/NAND ID 讀取失敗、燒錄異常失敗、Flash ID 抓取失敗 |
| QA04xxx | **SPI Tool 診斷** | 查詢開機模式、查詢開機失敗趨向、測試 Memory 狀態、快速開關機測試失敗 |
| QA05xxx | **電源問題** | 系統無法開機(需卸載零件)、其他設備引起當機、晶片異常發燙 |
| QA06xxx | **環境測試** | 室溫正常但高低溫異常 |
| QA07xxx | **韌體/IO** | 韌體燒錄後無反應、IO 干擾、系統不穩當機 |
| QA08xxx | **溫度** | 芯片工作溫度偏高 |
| QA09xxx | **散熱** | SoC Board 散熱規劃 |
| QA10xxx | **SMT 品質** | X-Ray 判別 EPAD 焊接、SMT 良率改善、X-Ray 進階判別焊接優劣 |
| QA11xxx | **EMC/可靠性** | ESD 預防排查、EMI 預防排查、高溫劣化、ESD Rework Guide、EMI Rework Guide |

完整清單位於：[IT9860_SOC_QA List_Chinese_Version.xlsx](file:///C:/公司/電子元件/ITE/IT986x/HDK/Chinese_IT986x_SoC_HDK_V1.1.7/Q&A/IT9860_SOC_QA List_Chinese_Version.xlsx)

---

### 5.3 SDK 軟體開發套件 (`IT986x\SDK\`)

#### 5.3.1 SDK 設計指南文件

| 檔案名稱 | 功能說明 |
|----------|---------|
| [ITE_2D_GUI_Framework_Porting_Guide.pdf](file:///C:/公司/電子元件/ITE/IT986x/SDK/ITE_2D_GUI_Framework_Porting_Guide.pdf) | ITE 2D GUI 框架移植指南 |
| [ITE_ITU_Graphic_API.pdf](file:///C:/公司/電子元件/ITE/IT986x/SDK/ITE_ITU_Graphic_API.pdf) | ITU 圖形 API 參考手冊 |
| [ITE_SDK_File_System_Selection_Guide.pdf](file:///C:/公司/電子元件/ITE/IT986x/SDK/ITE_SDK_File_System_Selection_Guide.pdf) | **檔案系統選擇指南**：比較 FAT/LFS/Yaffs2/FileX 各自適用場景 |
| [ITE_SDK_Library_Porting_Guide.pdf](file:///C:/公司/電子元件/ITE/IT986x/SDK/ITE_SDK_Library_Porting_Guide.pdf) | SDK Library 移植指南 |
| [ITE_SoC_Screen_Mirroring_App_Design_Guide.pdf](file:///C:/公司/電子元件/ITE/IT986x/SDK/ITE_SoC_Screen_Mirroring_App_Design_Guide.pdf) | **手機投屏 (Screen Mirroring) 應用**設計指南 |
| [ITE_SoC_Wireless_Module_Design_Guide.pdf](file:///C:/公司/電子元件/ITE/IT986x/SDK/ITE_SoC_Wireless_Module_Design_Guide.pdf) | **WiFi/BT 無線模組**設計指南 (2.9 MB) |
| [ITE_Speech_Recognition_Design_Guide.pdf](file:///C:/公司/電子元件/ITE/IT986x/SDK/ITE_Speech_Recognition_Design_Guide.pdf) | **語音辨識功能**設計指南 |
| [Panel_Script_Generator_378.exe](file:///C:/公司/電子元件/ITE/IT986x/SDK/Panel_Script_Generator_378.exe) | **面板參數腳本產生工具** (Panel Script Generator V378) |

#### 5.3.2 ITEGCC 交叉編譯工具鏈 (`ITEGCC\`)

ITE 專用的 GCC 交叉編譯工具鏈，含以下目標架構：

| 目標 | 說明 |
|------|------|
| `arm-none-eabi` | ARM Cortex 交叉編譯工具 |
| `sm32-elf` | ITE SM32 架構交叉編譯工具 |
| `bin\` | 編譯器可執行檔 (gcc, g++, ld, objcopy 等) |
| `include\` / `lib\` | 標準函式庫頭文件與靜態庫 |

#### 5.3.3 SDK V2.4.9.1 完整開發套件

**路徑**：`SDK\SDK_V2.4.9.1\`

##### 文件 (`doc\`)

| 路徑 | 功能說明 |
|------|---------|
| `ITE SDK DrawRocker GUI Designer System Development Guide\` | **DrawRocker GUI 設計工具**開發指南 (13.7 MB PDF)，含 GUI 設計器使用方法、Widget 開發 |
| `ITE SoC IT97x_IT986x Panel Tool Guide\` | **面板工具指南** (4 份 PDF)：CPU Panel、LVDS Panel、MIPI Panel、RGB/SPI Panel 的設定方法 |
| `ITE SoC IT986x Series Software Development Guide\` | **軟體開發指南** (78 MB PDF)，SDK 完整使用手冊，含環境建置、專案設定、驅動 API 等 |

##### SDK 原始碼 (`ite_sdk\`)

| 目錄 | 功能說明 |
|------|---------|
| `build\` | CMake 建構系統輸出目錄 |
| `cmake\` | CMake 工具鏈與模組定義 |
| `data\` | 資源檔案 (字型、圖片等) |
| `openrtos\` | **OpenRTOS 核心**及板級設定檔 |
| `sdk\` | **驅動與中介軟體**原始碼（見下方詳細列表） |
| `project\` | **應用專案範例**（見下方詳細列表） |
| `tool\` | 建構輔助工具 |
| `win32\` | Win32 模擬器環境 |
| `wizard.ps1` / `wizard.py` | 專案建立精靈腳本 |

##### SDK 驅動與中介軟體模組 (`sdk\`)

| 模組 | 說明 |
|------|------|
| `driver\` | 硬體驅動程式：GPIO、UART、SPI、I²C、LCD、CAN-FD、USB、SD、NOR/NAND Flash、ADC、Timer、WDT、RTC 等 |
| `codec\` | 音視訊編解碼器 |
| `include\` | 公用頭文件 API 定義 |
| `itu\` | ITU GUI 框架原始碼 |
| `share\` | 共享函式庫（freetype、libpng、zlib、curl、xml2 等） |
| `target\` | 目標板硬體抽象層 |
| `alt_cpu\` | 雙核/輔助 CPU 程式碼 |
| `arm_lite_dev\` | ARM Lite 裝置驅動 |
| `Kconfig` | 核心配置選項定義 (408 KB)，定義所有可配置參數 |

##### SDK 範例專案 (`project\`)

| 專案名稱 | 類型 | 功能說明 |
|----------|------|---------|
| `ref_base_general` | 參考專案 | 基礎通用參考專案 |
| `ref_base_cluster` | 參考專案 | 儀表板 (Cluster) 參考專案 |
| `ref_ext_mirror` | 參考專案 | 手機投屏 (Screen Mirroring) 參考專案 |
| `ref_ext_mirror_headset` | 參考專案 | 投屏 + 藍牙耳機參考專案 |
| `ref_ext_btclassic` | 參考專案 | 經典藍牙參考專案 |
| `ref_ext_hud` | 參考專案 | HUD 抬頭顯示器參考專案 |
| `automotive_canbus` | 車用專案 | 車用 CAN-FD 通訊專案 |
| `demo_aircon` | Demo | 空調控制面板 Demo |
| `demo_auto` | Demo | 車用自動化 Demo |
| `demo_fitness` | Demo | 健身器材面板 Demo |
| `demo_home` | Demo | 智慧家庭 Demo |
| `hmi_mcu` | Demo | HMI + MCU 協作 Demo |
| `lv_demos` / `lv_examples` | LVGL | LVGL GUI 框架 Demo 與範例 |
| `bootloader` | 系統 | Bootloader 原始碼 |
| `alt_cpu` | 系統 | 輔助 CPU 程式碼 |
| `arm_lite_codec` / `arm_lite_dev` | 系統 | ARM Lite 編解碼與裝置 |
| `test_*` (32 個) | 測試 | 各模組單元測試：GPIO、UART、SPI、I²C、CAN、LCD、Audio、USB、SD、NOR、RTC、ADC、Timer、Touch、Ethernet、WiFi、Bluetooth、Video、HUD 等 |

#### 5.3.4 Toolkit 工具集 (`Toolkit\`)

| 工具 | 功能說明 |
|------|---------|
| `20151013_Runtime\` | VC++ Runtime 安裝程式 (VC2008 SP1 / VC2010 SP1 / VC2012 / VC2013)，SDK 工具執行前置需求 |
| `CDM20828_Setup.exe` | **FTDI USB 驅動安裝程式**（SPI Tool / Debug Tool 使用的 USB-Serial 驅動） |
| `Video Convert Tools\` | 影片轉檔工具：含 **FFmpeg** 與 **Mencoder** |
| `ucl_tool_for_win32_emulation_IT983x.7z` | IT983x Win32 模擬器工具 |
| `ucl_tool_for_win32_emulation_IT986x_2491.7z` | IT986x Win32 模擬器工具 (SDK 2491 版) |

---

## 6. 快速查找指南 (Quick Lookup)

### 按需求查找

| 我需要... | 去哪裡找 |
|-----------|---------|
| 晶片總覽規格（Pinout、功能方塊圖） | `IT986x\Datasheet\IT986x_Datasheet.pdf` 或 `-AT` 版 |
| 特定模組暫存器位址 | `IT986x\Datasheet\IT986x - [模組名].pdf` |
| GPIO 腳位功能表 | `IT986x_GPIO_Assignment_*.xls` |
| 原理圖設計指引 | `HDK\Chinese_*\IT986x_硬件開發指南_V110.pdf` |
| PCB Layout 設計指引 | `HDK\Chinese_*\IT986X SoC PCB Layout Design Guide.ppsx` |
| EVB 使用手冊 | `HDK\Chinese_*\IT986x_General_EVB-20230425\IT986x GENERAL EVB GUIDE_V3.pdf` |
| 硬體問題排除 | `HDK\` 根目錄或 `Q&A\Chinese_Version\` 的中文問答 PDF |
| 軟體 SDK 完整指南 | `SDK\SDK_V2.4.9.1\doc\ITE SoC IT986x Series Software Development Guide\` |
| GUI 開發 (DrawRocker) | `SDK\SDK_V2.4.9.1\doc\ITE SDK DrawRocker GUI Designer System Development Guide\` |
| 面板設定 (Panel Tool) | `SDK\SDK_V2.4.9.1\doc\ITE SoC IT97x_IT986x Panel Tool Guide\` |
| 檔案系統選擇 | `SDK\ITE_SDK_File_System_Selection_Guide.pdf` |
| WiFi/BT 模組設計 | `SDK\ITE_SoC_Wireless_Module_Design_Guide.pdf` |
| 手機投屏功能 | `SDK\ITE_SoC_Screen_Mirroring_App_Design_Guide.pdf` |
| CAN-FD 車用通訊 | Datasheet: `IT986x - CANBUS_FD.pdf`; 範例: `project\test_canbus` / `automotive_canbus` |
| USB Host/Device | Datasheet: `IT986x - USB.pdf`; AP Note: `BU5_HW_9860A06_USB Test JK.pdf` |
| RTC 設計 | AP Note: `BU5_HW_9860A07~A08~A11` (電源處理/斷電/漏電) |
| ESD/EMI 處理 | Q&A: `BU5_QA11001~11005` (預防/排查/Rework Guide) |
| 散熱設計 | Q&A: `BU5_QA09001_SoC Board散熱規劃議題.pdf` |
| SMT 製程規範 | AP Note: `BU5_HW_9860A00` + `BU5-HW-9860A13` |
| 看門狗設計 | AP Note: `BU5-HW-9860A12`; Datasheet: `IT986x - Watch Dog Timer.pdf` |
| 周邊晶片參考電路 | `HDK\Chinese_*\IT986x_Peripheral circuit\` |
| IT6122 HDMI 轉換器 | `IT6122\` 目錄 |
| IT97x 新一代 SoC | `IT97x\` 目錄 |

### 按晶片型號查找

| 晶片 | 主要差異 | DDR | 定位 |
|------|---------|-----|------|
| IT9860 | 基礎型 | DDR2 | 消費/工控 |
| IT9862 | 中階型 | DDR2 | 消費/工控 |
| IT9866 | 含 MIPI | DDR2 | 消費/工控 |
| IT9868 | 高階型 | DDR3 | 消費/工控 |
| IT986x-AT | 車規版 | DDR2/3 | 車用 (AEC-Q100) |
| IT972 | 新一代 | - | 下一代平台 |
| IT976/IT978 | 新一代 | - | 下一代平台 |
| IT6122 | HDMI IC | - | HDMI 接收/轉換 |

---

## 7. 檔案格式對照表

| 副檔名 | 軟體 | 說明 |
|--------|------|------|
| `.DSN` | OrCAD Capture | 原理圖設計檔 (可匯出 PDF) |
| `.pcb` | Allegro PCB / PADS | PCB Layout 設計檔 |
| `.asc` | - | Netlist 文字檔 |
| `.ppsx` | Microsoft PowerPoint | 投影片 (唯讀模式) |
| `.xls/.xlsx` | Microsoft Excel | 電子表格 (GPIO 表/BOM 表) |
| `.7z` | 7-Zip | 壓縮檔 |
| `.exe` | - | Windows 執行檔 / 安裝程式 |

---

> [!NOTE]
> **此索引產生時間**：2026-07-11。若有新增或更新檔案，請重新掃描 `C:\公司\電子元件\ITE` 以更新本文件。
> **ITE SoC 官方線上資源**：https://soc.ite.com.tw
