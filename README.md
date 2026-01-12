# Open Race GPS (DIY High-Refresh GNSS)

[English](#english) | [中文](#chinese)

<a name="english"></a>
## 🇬🇧 English

### Introduction
A high-performance, low-cost DIY GPS solution for motorsports telemetry.
By combining a **U-blox M10 (e.g., M181)** or **M9** chip with a **Bluetooth module (e.g., DX-BT36)**, you can achieve **25Hz** (or 10Hz stable) refresh rates and extremely low latency for under **$40**.

Compared to commercial devices like RaceBox Mini (~$199) or Dragy (~$150), this DIY solution offers comparable performance at a fraction of the cost. It is perfectly compatible with **CarPilot** (iOS) and many other lap timer apps.

### Bill of Materials (BOM)
| Component | Recommendation | Estimated Price | Notes |
|-----------|----------------|-----------------|-------|
| **GNSS Module** | Walksnail WS-M181 (M10) / Beitian MI-M10 | ~$15 - $20 | Must support U-blox protocol |
| **Bluetooth** | DX-BT36 (BLE+SPP) | ~$3 - $5 | Supports iOS BLE connection |
| **Battery** | 18650 Li-ion + 1S BMS | ~$3 | Or any 3.7V Lipo |
| **Case** | 3D Printed / Generic Box | ~$2 | STL files included (TBD) |

### Wiring Guide
In the simplest "Pass-through" mode, you don't even need a microcontroller (ESP32/Arduino). The Bluetooth module directly broadcasts the GNSS data.

**Connection:**
- **GNSS TX**  -> **Bluetooth RX**
- **GNSS RX**  -> **Bluetooth TX**
- **GNSS VCC** -> **Battery + (3.3V-5V)**
- **GNSS GND** -> **Battery -**
- **BT VCC**   -> **Battery +**
- **BT GND**   -> **Battery -**

> **Note:** Ensure your GNSS module configuration (Baud Rate) matches your Bluetooth module's default baud rate (usually 9600 or 115200). It is highly recommended to configure both to **115200** for high-frequency data.

---

<a name="chinese"></a>
## 🇨🇳 中文

### 简介
一个高性能、低成本的赛车数据记录 GPS 硬件方案。
通过将 **U-blox M10 (如 M181)** 或 **M9** 定位芯片与 **蓝牙模块 (如 DX-BT36)** 组合，你可以以低于 **300元人民币** 的成本，获得媲美专业设备（如 RaceBox Mini, Dragy）的 **25Hz** 刷新率和极低延迟。

本硬件完美支持 **CarPilot** (iOS) 以及其他支持外接 BLE GPS 的圈速软件。

### 零件清单 (BOM)
| 组件 | 推荐型号 | 预估价格 | 备注 |
|---|---|---|---|
| **GNSS 定位模块** | 蜗牛 Walksnail WS-M181 (M10) / 北天 MI-M10 | ~100元 | 核心部件，必须支持 U-blox |
| **蓝牙模块** | DX-BT36 (双模) | ~15元 | 必须支持 BLE 以连接 iOS |
| **电池** | 18650 锂电池 + 保护板 | ~10元 | 或任意 3.7V 聚合物锂电池 |
| **外壳** | 3D打印 / 通用塑料盒 | ~5元 | 后续将提供 STL 文件 |

### 接线指南 (小白版)
在最简单的“透传模式”下，你甚至不需要单片机 (ESP32/Arduino)。蓝牙模块会直接把 GPS 数据广播给手机。

**接线方式：**
- **GPS 的 TX**  -> 接 -> **蓝牙的 RX**
- **GPS 的 RX**  -> 接 -> **蓝牙的 TX**
- **GPS 的 VCC** -> 接 -> **电池正极 (+)**
- **GPS 的 GND** -> 接 -> **电池负极 (-)**
- **蓝牙的 VCC** -> 接 -> **电池正极 (+)**
- **蓝牙的 GND** -> 接 -> **电池负极 (-)**

> **注意**：请务必确保 GPS 模块的波特率与蓝牙模块一致。推荐使用 **115200** 波特率以支持 10Hz/25Hz 的高频数据传输。
