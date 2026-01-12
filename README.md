# Open Race GPS (DIY High-Refresh GNSS)

[English](#english) | [中文](#chinese)

<a name="english"></a>
## 🇬🇧 English

### Introduction
Open Race GPS is a high-performance, low-cost DIY hardware project for motorsports telemetry. This repository provides two versions with different capabilities:

1.  **Direct Bluetooth Model (M9 + DBX36)**: 🌟 **Highly Recommended**. Simple structure, extremely reliable BLE connection. Requires phone connection.
2.  **Standalone Logger Model (ESP32 + SD)**: Supports recording **VBO files** to SD card without a phone. Supports WiFi data download.

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
| **GNSS Module** | U-blox M9 or M10 (e.g., Beitian BN-220) | ~$15 | |
| **Bluetooth** | DX-BT36 (BLE+SPP) | ~$3 | **Critical**: Must support BLE for iOS |
| **Battery** | 18650 Li-ion + 1S BMS | ~$3 | |

#### Wiring
- **GNSS TX**  -> **Bluetooth RX**
- **GNSS RX**  -> **Bluetooth TX**
- **VCC/GND**  -> **Battery +/-**

---

### Version 2: Standalone Logger (ESP32 + M10 + SD)
*A standalone VBO logger. Records session data to SD card in `.vbo` format even without a phone connection.*

**Key Features:**
- **Standalone**: Can record independently.
- **VBO Format**: Saves data directly in professional VBO format.
- **WiFi Support**: Acts as an AP for downloading files wirelessly.

#### Bill of Materials (BOM)
| Component | Recommendation | Estimated Price | Notes |
|-----------|----------------|-----------------|-------|
| **GNSS Module** | Walksnail WS-M181 (M10) | ~$20 | 25Hz Capable |
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
2.  **脱机记录版 (Standalone Logger)**: ESP32 + SD卡。支持脱离手机独立记录，生成 **VBO 格式**文件，支持 WiFi 数据下载。

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
| **GNSS 模块** | U-blox M9 或 M10 (如北天 BN-220) | ~80元 | |
| **蓝牙模块** | DX-BT36 (双模) | ~15元 | **注意**: 必须支持 BLE 以连接 iPhone |
| **电池** | 18650 锂电池 + 保护板 | ~10元 | |

#### 接线
- **GPS TX**  -> **蓝牙 RX**
- **GPS RX**  -> **蓝牙 TX**
- **VCC/GND** -> **电池 +/-**

---

### 版本 2: 脱机记录版 (ESP32 + M10 + SD)
*智能记录仪。支持不插手机，直接将数据以 **VBO** 专业格式记录到 SD 卡中。支持 WiFi 热点下载。*

**核心功能：**
- **独立工作**：不需要带手机上车也能记录。
- **VBO 格式**：生成的日志文件可直接用于专业软件分析或 CarPilot 导入。
- **WiFi 下载**：通过 HTTP 服务器无线下载数据。

#### 零件清单 (BOM)
| 组件 | 推荐型号 | 预估价格 | 备注 |
|---|---|---|---|
| **GNSS 模块** | 蜗牛 Walksnail WS-M181 (M10) | ~100元 | 支持 25Hz |
| **主控** | ESP32-C3 / ESP32 开发板 | ~25元 | |
| **SD卡模块** | MicroSD SPI 模块 | ~5元 | |
| **电池** | 18650 / 聚合物锂电池 | ~15元 | |

#### 接线与固件
请查看 `Standalone_Logger_ESP32_M10_SD/firmware/` 文件夹获取详细的 ESP32 接线图和固件代码。
