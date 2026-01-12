# GNSS Configuration Guide (U-blox M9/M10)

[English](#english) | [中文](#chinese)

<a name="english"></a>
## 🇬🇧 English

To achieve the best performace (25Hz refresh rate) and low latency for CarPilot, we need to configure the U-blox M9/M10 module using u-center.

### 1. Hardware Preparation
You need a **USB to TTL (Serial) Converter** (e.g., CP2102, CH340).

**Connection:**
- **Module VCC** <-> **USB 5V/3.3V**
- **Module GND** <-> **USB GND**
- **Module TX**  <-> **USB RX**
- **Module RX**  <-> **USB TX**

### 2. Software
Download and install **u-center 2** (or u-center classic) from the U-blox website.

### 3. Configuration Steps
1.  **Connect**: Open u-center, select the COM port of your USB adapter, and set baudrate (default is usually 9600 or 38400).
2.  **Open Configuration View** (View -> Configuration View).
3.  **Set Baud Rate**:
    - Go to `PRT (Ports)` -> `UART1`.
    - Set **Baudrate** to `115200`.
    - Click `Send`. (You may need to reconnect u-center at 115200 now).
4.  **Set Update Rate**:
    - Go to `RATE (Rates)`.
    - Set **Measurement Period** to `40 ms` (which equals **25Hz**).
    - Click `Send`.
5.  **Disable NMEA & Enable PVT**:
    - Go to `MSG (Messages)`.
    - **Disable**: Select `F0-00 NMEA GxGGA`, uncheck UART1. Repeat for `GxGLL`, `GxGSA`, `GxGSV`, `GxRMC`, `GxVTG`.
    - **Enable**: Select `01-07 UBX NAX-PVT`, check **UART1 On**.
    - Click `Send`.
6.  **Set Dynamic Model**:
    - Go to `NAV5 (Navigation 5)`.
    - Set **Dynamic Model** to `Automotive`.
    - Click `Send`.
7.  **Save Configuration (Important!)**:
    - Go to `CFG (Configuration)`.
    - Select **Save current configuration**.
    - Highlight `BBR` and `FLASH`.
    - Click `Send`.

---

<a name="chinese"></a>
## 🇨🇳 中文

为了让 CarPilot 获得最佳性能（25Hz 刷新率、低延迟），我们需要使用 u-center 软件对 U-blox M9/M10 模块进行配置。

### 1. 硬件准备
你需要一个 **USB 转 TTL (串口) 模块** (例如 CP2102, CH340, FT232)。

**接线：**
- **模块 VCC** <-> **USB 5V/3.3V**
- **模块 GND** <-> **USB GND**
- **模块 TX**  <-> **USB RX**
- **模块 RX**  <-> **USB TX**

### 2. 软件准备
去 U-blox 官网下载并安装 **u-center 2** (或 u-center classic)。

### 3. 配置步骤
1.  **连接硬件**：打开 u-center，选择 USB 模块对应的 COM 口。波特率选默认（通常是 9600 或 38400），连接成功后右下角会闪烁绿色。
2.  **打开配置窗口**：点击菜单栏 `View` -> `Configuration View`。
3.  **修改波特率 (Baud Rate)**：
    - 左侧找到 `PRT (Ports)`。目标选 `UART1`。
    - 将 **Baudrate** 改为 `115200`。
    - 点击左下角 `Send`。*(此时连接会断开，你需要将 u-center 上方的波特率也改为 115200 重新连接)*。
4.  **修改刷新率 (Rate)**：
    - 左侧找到 `RATE (Rates)`。
    - 将 **Measurement Period** 改为 `40 ms` (即 **25Hz**)。
    - 点击 `Send`。
5.  **开启 PVT / 关闭 NMEA**:
    - 左侧找到 `MSG (Messages)`。
    - **关闭 NMEA**：下拉找到 `F0-00 NMEA GxGGA`，取消勾选 `UART1`。同理取消 `GLL`, `GSA`, `GSV`, `RMC`, `VTG` 等所有 F0 开头的 NMEA 消息。
    - **开启 PVT**：下拉找到 `01-07 UBX NAX-PVT`，勾选 `UART1 On`。
    - 点击 `Send`。
6.  **设置模式**：
    - 左侧找到 `NAV5 (Navigation 5)`。
    - 将 **Dynamic Model** 设为 `Automotive` (车载模式)。
    - 点击 `Send`。
7.  **保存配置 (关键步骤)**：
    - 左侧找到 `CFG (Configuration)`。
    - 选择 **Save current configuration** (保存当前配置)。
    - 确保 `Devices` 里的 `BBR` 和 `FLASH` (如果有) 被选中。
    - 点击 `Send`。**掉电重启模块，确认配置是否生效。**
