# ESP32 Firmware Flashing Guide (ESP32-WROOM-32)
# ESP32 固件烧录指南 (ESP32-WROOM-32)

[English](#english) | [中文](#chinese)

---

## <a id="english"></a>🇬🇧 English

This guide explains how to flash the `CarPilot_GPS_Firmware.ino` onto your ESP32-WROOM-32 development board using a USB cable.

### 1. Install USB Drivers (CH340 / CP210x)
Most ESP32 development boards use the **CH340** or **CP2102** USB-to-Serial chip. If you plug in your ESP32 and don't see a COM port (Windows) or `/dev/cu.usbserial...` (Mac), you likely need to install the drivers.

*   **CH340 Drivers (Common for cheaper boards):**
    *   [Download Link (WCH Official)](http://www.wch-ic.com/downloads/CH341SER_ZIP.html)
    *   Download, unzip, and run the installer.
*   **CP210x Drivers (Common for NodeMCU/DevKitC):**
    *   [Download Link (Silicon Labs)](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)

### 2. Setup Arduino IDE

1.  **Download Arduino IDE**: Install the latest version from [arduino.cc](https://www.arduino.cc/en/software).
2.  **Add ESP32 Board Manager**:
    *   Open **File -> Preferences** (or Arduino IDE -> Settings on Mac).
    *   In "Additional Boards Manager URLs", paste:
        `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
    *   Click OK.
3.  **Install ESP32 Platform**:
    *   Open **Tools -> Board -> Boards Manager**.
    *   Search for **"esp32"** (by Espressif Systems).
    *   Click **Install** (Version 2.0.x or 3.0.x are both fine).
4.  **Select Board**:
    *   Go to **Tools -> Board -> esp32**.
    *   Select **"ESP32 Dev Module"** (This is the most compatible option for WROOM-32 boards).
5.  **Select Port**:
    *   Go to **Tools -> Port**.
    *   Select the port that corresponds to your ESP32 (e.g., `COM3` on Windows or `/dev/cu.usbserial-xxx` on Mac).

### 3. Flash the Firmware

1.  **Open Firmware**: Double-click `firmware/CarPilot_GPS_Firmware.ino` to open it in Arduino IDE.
2.  **Verify Settings**:
    *   **Upload Speed**: 921600 (Faster) or 115200 (More reliable).
    *   **Flash Frequency**: 80MHz.
    *   **Flash Mode**: QIO (default) or DIO (if QIO fails).
3.  **Upload**:
    *   Click the **Arrow Icon (➜)** or press `Cmd+U` / `Ctrl+U` to Compile and Upload.
4.  **Troubleshooting (Connecting...)**:
    *   If you see `Connecting........_____.....` and it fails:
    *   **Press and hold the BOOT button** on the ESP32 board when "Connecting..." appears.
    *   Once uploading starts, release the BOOT button.

---

## <a id="chinese"></a>🇨🇳 中文

本指南介绍了如何使用 USB 线将 `CarPilot_GPS_Firmware.ino` 固件刷入 ESP32-WROOM-32 开发板。

### 1. 安装 USB 驱动 (CH340 / CP210x)
大多数 ESP32 开发板使用 **CH340** 或 **CP2102** USB 转串口芯片。如果你插入 ESP32 后没有看到 COM 端口 (Windows) 或 `/dev/cu.usbserial...` (Mac)，则说明需要安装驱动。

*   **CH340 驱动 (常见于普通开发板):**
    *   [下载链接 (WCH 沁恒官网)](http://www.wch.cn/downloads/CH341SER_EXE.html) (Windows)
    *   [下载链接 (WCH 沁恒官网)](http://www.wch.cn/downloads/CH341SER_MAC_ZIP.html) (Mac)
    *   下载后解压并运行安装程序。
*   **CP210x 驱动 (常见于 NodeMCU/DevKitC):**
    *   [下载链接 (Silicon Labs)](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)

### 2. 设置 Arduino IDE

1.  **下载 Arduino IDE**: 从 [arduino.cc](https://www.arduino.cc/en/software) 安装最新版本。
2.  **添加 ESP32 开发板管理器**:
    *   打开 **文件 (File) -> 首选项 (Preferences)** (Mac 上是 Arduino IDE -> Settings)。
    *   在 "附加开发板管理器网址 (Additional Boards Manager URLs)" 中粘贴:
        `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
    *   点击确定。
3.  **安装 ESP32 平台**:
    *   打开 **工具 (Tools) -> 开发板 (Board) -> 开发板管理器 (Boards Manager)**。
    *   搜索 **"esp32"** (作者是 Espressif Systems)。
    *   点击 **安装 (Install)**。
4.  **选择开发板**:
    *   前往 **工具 (Tools) -> 开发板 (Board) -> esp32**。
    *   选择 **"ESP32 Dev Module"** (这是最通用的 WROOM-32 选项)。
5.  **选择端口**:
    *   前往 **工具 (Tools) -> 端口 (Port)**。
    *   选择对应 ESP32 的端口 (例如 Windows 上的 `COM3` 或 Mac 上的 `/dev/cu.usbserial-xxx`)。

### 3. 烧录固件

1.  **打开固件**: 双击 `firmware/CarPilot_GPS_Firmware.ino` 在 Arduino IDE 中打开。
2.  **检查设置**:
    *   **Upload Speed**: 推荐 921600 (更快) 或 115200 (更稳)。
    *   **Flash Frequency**: 80MHz。
    *   **Flash Mode**: QIO (默认) 或 DIO (如果 QIO 失败)。
3.  **上传 (Upload)**:
    *   点击界面上的 **向右箭头图标 (➜)**，或按 `Cmd+U` / `Ctrl+U` 开始编译并上传。
4.  **故障排除 (Connecting...)**:
    *   如果在底部看到 `Connecting........_____.....` 并且最终失败：
    *   当出现 "Connecting..." 时，**按住 ESP32 板子上的 BOOT 按钮**。
    *   一旦开始上传进度条，松开 BOOT 按钮。
