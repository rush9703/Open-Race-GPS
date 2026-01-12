# Open Race GPS (DIY High-Refresh GNSS)

[English](#english) | [中文](#chinese)

<a name="english"></a>
## 🇬🇧 English

### Introduction
Open Race GPS is a high-performance, low-cost DIY hardware project for motorsports telemetry. This repository provides two versions with different capabilities:

1.  **Direct Bluetooth Model (M9 + DBX36)**: 🌟 **Highly Recommended**. Simple structure, extremely reliable BLE connection. Requires phone connection.
2.  **Standalone Logger Model (ESP32 + M10 + SD)**: The "All-in-One". Supports **Real-time BLE/WiFi connection** to phone, AND **Standalone VBO logging** to SD card without phone.

Both versions achieve **25Hz** refresh rates and are fully compatible with **CarPilot** (iOS).

---

### Version 1: Direct Bluetooth (M9 + DBX36) - 🌟 Highly Recommended
*Pure hardware passthrough. The Bluetooth module broadcasts GNSS data directly to your phone.*

**Why we recommend this:**
- **Reliability**: Direct hardware UART to BLE is extremely stable. Connection almost never drops.
- **Simplicity**: Safest choice for most users. No coding required.
- **Performance**: M9/Tenet modules often have onboard Flash and handle high frequencies effortlessly.

#### Bill of Materials (BOM)
| Component | Recommendation | Estimated Price | Notes |
|-----------|----------------|-----------------|-------|
| **GNSS Module** | U-blox M9N | ~$20 | |
| **Bluetooth** | DX-BT36 (BLE+SPP) | ~$3 | **Critical**: Must support BLE for iOS |
| **Battery** | 18650 Li-ion + 1S BMS | ~$3 | |

#### Wiring
- **GNSS TX**  -> **Bluetooth RX**
- **GNSS RX**  -> **Bluetooth TX**
- **VCC/GND**  -> **Battery +/-**

---

### Version 2: Standalone Logger (ESP32 + M10 + SD)
*A versatile logger that works both online and offline.*

**Key Features:**
- **Double Mode**: Works as a **Real-time GPS Receiver** (via BLE/WiFi) OR a **Standalone Logger**.
- **Offline Recording**: Records data to SD card in `.vbo` format when phone is not connected.
- **WiFi Download**: Supports downloading VBO files via WiFi.

#### Bill of Materials (BOM)
| Component | Recommendation | Estimated Price | Notes |
|-----------|----------------|-----------------|-------|
| **GNSS Module** | Max M10s | ~$15 | 10Hz Capable, no flash， need esp32 write config to GPS after power off |
| **Controller**| ESP32-C3 / ESP32 DevKit | ~$5 | |
| **SD Card**   | MicroSD Module + SPI | ~$2 | |
| **Battery**   | 18650 / LiPo | ~$5 | |

#### Wiring
- **GNSS TX** -> **ESP32 RX (GPIO x)**
- **GNSS RX** -> **ESP32 TX (GPIO y)**
- **SD CS**   -> **ESP32 GPIO z**
- *(See `Standalone_Logger_ESP32_M10_SD/firmware/` folder for pin definitions)*

---

<a name="chinese"></a>
## 🇨🇳 中文

### 简介
Open Race GPS 是一个高性能、低成本的开源赛车数据硬件方案。本项目提供两种不同功能版本的硬件：

1.  **蓝牙直连版 (Bluetooth Direct)**: 🌟 **强烈推荐**。结构简单，BLE 连接极其稳定可靠。使用时需连接手机 App。
2.  **脱机记录版 (ESP32 + M10 + SD)**: “全能型”选手。既支持 **BLE/WiFi 实时连接手机**，也支持 **不带手机脱机记录** VBO 数据到 SD 卡。

两个版本均支持 **25Hz** 刷新率（基于 U-blox M10/M9），完美支持 **CarPilot** (iOS)。

---

### 版本 1: 蓝牙直连版 (M9 + DBX36) - 🌟 强烈推荐
*纯硬件直通方案。蓝牙模块直接把 GPS 数据广播给手机。*

**我们为什么更推荐这个：**
- **极其可靠**：硬件直连比软件桥接更稳，BLE 工作非常可靠，几乎不掉线。
- **结构简单**：不需要写代码，甚至不需要单片机，接线即用。
- **高性能**：推荐使用 M9 (如天那特/北天) 模块，自带 Flash 可保存高频配置，轻松跑满 25Hz。

#### 零件清单 (BOM)
| 组件 | 推荐型号 | 预估价格 | 备注 |
|---|---|---|---|
| **GNSS 模块** | U-blox M9N | ~80元 | |
| **蓝牙模块** | DX-BT36 (双模) | ~15元 | **注意**: 必须支持 BLE 以连接 iPhone |
| **电池** | 18650 锂电池 + 保护板 | ~10元 | |

#### 接线
- **GPS TX**  -> **蓝牙 RX**
- **GPS RX**  -> **蓝牙 TX**
- **VCC/GND** -> **电池 +/-**

---

### 版本 2: 脱机记录版 (ESP32 + M10 + SD)
*一个支持在线/离线双模式的通用记录仪。*

**核心功能：**
- **双模工作**：支持作为实时 GPS 接收器 (BLE/WiFi) 连接手机，**或者** 作为独立记录仪工作。
- **脱机记录**：手机不在时，数据会自动以 **VBO** 格式保存到 SD 卡。
- **WiFi 下载**：支持通过 WiFi 无线下载 SD 卡中的文件。

#### 零件清单 (BOM)
| 组件 | 推荐型号 | 预估价格 | 备注 |
|---|---|---|---|
| **GNSS 模块** | Max-M10s | ~40元 | 支持 10Hz,没有flash，断电需要重新写入配置，必须ESP32 |
| **主控** | ESP32-C3 / ESP32 开发板 | ~25元 | |
| **SD卡模块** | MicroSD SPI 模块 | ~5元 | |
| **电池** | 18650 / 聚合物锂电池 | ~15元 | |

#### 接线与固件
请查看 `Standalone_Logger_ESP32_M10_SD/firmware/` 文件夹获取详细的 ESP32 接线图和固件代码。
