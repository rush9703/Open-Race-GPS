# Open Race GPS (DIY High-Refresh GNSS)

[English](#english) | [中文](#chinese)

<a name="english"></a>
## 🇬🇧 English

### Introduction
Open Race GPS is a high-performance, low-cost DIY hardware project for motorsports telemetry. This repository provides two versions of the hardware:

1.  **Lite Version (Beginner)**: Direct Bluetooth pass-through. Easiest to build, ultra-low cost (~$40).
2.  **Pro Version (Advanced)**: ESP32 + SD Card. Standalone logging (no phone needed) and WiFi telemetry support (~$50).

Both versions achieve **25Hz** refresh rates with U-blox M10/M9 chips and are fully compatible with **CarPilot** (iOS).

---

### Version 1: Lite (Pass-through) - 🌟 Highly Recommended
*Simple, direct connection. The Bluetooth module broadcasts GNSS data directly to your phone.*

**Why we recommend this:**
- **Simplicity**: No coding, no microcontroller, just wire it up.
- **Reliability**: Direct hardware UART to BLE is often more stable than software bridging.
- **Performance**: U-blox M9/Tenet modules often have onboard Flash (save settings permanently) and handle 25Hz effortlessly alongside other constellations.

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

### Version 2: Pro (ESP32 + SD Logging)
*Smart logger. Records data to SD card even without a phone connection. Supports WiFi AP mode for data download.*

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
- *(See `firmware/` folder for pin definitions)*

---

<a name="chinese"></a>
## 🇨🇳 中文

### 简介
Open Race GPS 是一个高性能、低成本的开源赛车数据硬件方案。本项目提供两个版本：

1.  **Lite 简易版**: 蓝牙透传。制作最简单，无需编程，成本极低（< 300元）。
2.  **Pro以此类推专业版**: ESP32 + SD卡。支持脱机记录（无需手机也能记录数据），支持 WiFi 下载数据。

两个版本均支持 **25Hz** 刷新率（基于 U-blox M10/M9），完美支持 **CarPilot** (iOS)。

---

### 版本 1: Lite (简易透传版) - 🌟 强烈推荐
*最简单的方案。蓝牙模块直接把 GPS 数据转发给手机。*

**推荐理由：**
- **结构简单**：不需要单片机也不用写代码，连几根线就能用。
- **更加可靠**：硬件直连通常比软件桥接更稳定，BLE 连接不断连。
- **性能优势**：推荐使用 **M9 (如北天/天那特)** 模块，它们通常自带 Flash (掉电保存配置)，且处理 25Hz 高频数据更轻松。

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

### 版本 2: Pro (ESP32 脱机记录版)
*智能记录仪。即使不带手机上车，插入 SD 卡即可自动记录数据。支持 WiFi 热点下载。*

#### 零件清单 (BOM)
| 组件 | 推荐型号 | 预估价格 | 备注 |
|---|---|---|---|
| **GNSS 模块** | 蜗牛 Walksnail WS-M181 (M10) | ~100元 | 支持 25Hz |
| **主控** | ESP32-C3 / ESP32 开发板 | ~25元 | |
| **SD卡模块** | MicroSD SPI 模块 | ~5元 | |
| **电池** | 18650 / 聚合物锂电池 | ~15元 | |

#### 接线与固件
请查看 `firmware/` 文件夹获取详细的 ESP32 接线图和固件代码。
