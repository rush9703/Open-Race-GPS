# User Manual (ESP32 Standalone Logger)
# 使用说明书 (ESP32 独立记录版)

[English](#english) | [中文](#chinese)

---

## <a id="english"></a>🇬🇧 English

This guide explains how to operate the Standalone Logger (ESP32) version of the hardware.

### 1. Button Operations

The device uses a single button (BOOT button on ESP32) to control all functions.

| Action | Function | Description |
| :--- | :--- | :--- |
| **Short Press** | **Switch Logging Mode** | Cycle through: **BLE Mode** → **WiFi Mode** → **SD Mode**. |
| **Long Press (2s)** | **Sync Mode** | Toggle **Sync Mode** (for downloading files). Press and hold for 2 seconds to Enter or Exit. |

### 2. Operating Modes & LED Status

| Mode | LED Indicator | Function | Connection Details |
| :--- | :--- | :--- | :--- |
| **BLE Mode** (Default) | **Slow Blink** (Standby)<br>**Fast Blink** (Connected) | Real-time data via Bluetooth. Best for daily use. | **Name:** `CarPilot_Pro`<br>**Type:** BLE (Nordic UART) |
| **WiFi Mode** | **Very Fast Blink** | Real-time data via WiFi (Low Latency). | **SSID:** `CarPilot_Live`<br>**Pass:** `12345678`<br>**IP:** `192.168.4.1`<br>**Port:** `8080` |
| **SD Mode** | **Double Blink** | Logs data directly to SD card (`.vbo` format). No radio transmission. | Save to `/` root directory.<br>File: `LOG_001.VBO` |
| **Sync Mode** | **Solid On** | **File Download Mode**. Creates a WiFi hotspot to download logs from SD card. | **SSID:** `CarPilot_Sync`<br>**Pass:** `12345678`<br>**IP:** `192.168.4.1` |

### 3. Usage Tips

*   **Boot Up**: The device starts in **BLE Mode** by default.
*   **Recording**:
    *   **BLE/WiFi Mode**: Data is sent to the app. The App handles recording.
    *   **SD Mode**: Recording starts automatically when GPS fix is acquired.
*   **Downloading Files**:
    1.  Long press button to enter **Sync Mode** (LED Solid On).
    2.  Connect your phone/PC to WiFi: `CarPilot_Sync`.
    3.  Use the CarPilot App (or a browser/Telnet) to browse and download files.
    4.  Long press again to exit and return to recording mode.

---

## <a id="chinese"></a>🇨🇳 中文

本指南介绍如何操作独立记录版 (ESP32) 硬件。

### 1. 按键操作

设备仅需一个按键 (ESP32 上的 BOOT 键) 即可完成所有控制。

| 操作 | 功能 | 说明 |
| :--- | :--- | :--- |
| **短按** | **切换记录模式** | 循环切换: **BLE 蓝牙模式** → **WiFi 模式** → **SD 卡模式**。 |
| **长按 (2秒)** | **同步模式** | 进入或退出 **同步模式** (用于下载数据)。长按 2 秒触发。 |

### 2. 运行模式与 LED 状态

| 模式 | LED 指示灯 | 功能 | 连接详情 |
| :--- | :--- | :--- | :--- |
| **BLE 蓝牙模式** (默认) | **慢闪** (待机)<br>**快闪** (已连接) | 通过蓝牙传输实时数据。日常使用首选。 | **名称:** `CarPilot_Pro`<br>**类型:** BLE (Nordic UART) |
| **WiFi 模式** | **极快闪烁** | 通过 WiFi 传输实时数据 (低延迟)。 | **WIFI名:** `CarPilot_Live`<br>**密码:** `12345678`<br>**IP:** `192.168.4.1`<br>**端口:** `8080` |
| **SD 卡模式** | **双闪** (哒哒-停) | 数据直接存入 SD 卡 (`.vbo` 格式)。不发射无线信号。 | 存入根目录 `/`。<br>文件: `LOG_001.VBO` |
| **Sync 同步模式** | **常亮** | **文件下载模式**。建立 WiFi 热点供从 SD 卡下载文件。 | **WIFI名:** `CarPilot_Sync`<br>**密码:** `12345678`<br>**IP:** `192.168.4.1` |

### 3. 使用技巧

*   **开机**: 设备默认启动进入 **BLE 蓝牙模式**。
*   **记录**:
    *   **BLE/WiFi 模式**: 数据发送给 App，由 App 进行记录。
    *   **SD 卡模式**: 获取到 GPS 定位后自动开始记录。
*   **下载文件**:
    1.  长按按键进入 **Sync 同步模式** (LED 常亮)。
    2.  手机/电脑连接 WiFi: `CarPilot_Sync`。
    3.  使用 CarPilot App (或浏览器/Telnet) 浏览并下载文件。
    4.  再次长按退出，返回记录模式。
