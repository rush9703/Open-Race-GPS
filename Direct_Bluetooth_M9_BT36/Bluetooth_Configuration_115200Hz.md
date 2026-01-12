# Bluetooth Configuration Guide (DX-BT36)

[English](#english) | [中文](#chinese)

<a name="english"></a>
## 🇬🇧 English

The Bluetooth module (DX-BT36, JDY-31, or compatible SPP+BLE module) must be configured to match the baud rate of the GNSS module (**115200 bps**). If they don't match, the phone will receive garbage data or nothing at all.

### 1. Hardware Preparation
You need a **USB to TTL (Serial) Converter**.

**Connection for Configuration:**
- **BT VCC** <-> **USB 5V/3.3V** (Check your module voltage!)
- **BT GND** <-> **USB GND**
- **BT TX**  <-> **USB RX**
- **BT RX**  <-> **USB TX**

### 2. Software
Use any Serial Port Utility (e.g., **SSCOM**, **PuTTY**, **CoolTerm**, or Web Serial).

### 3. Configuration Steps
1.  **Connect**: Open the Serial Tool, select the COM port.
    - **Baudrate**: Try `9600` (Default for most modules) or `38400`.
2.  **Test Connection**:
    - Send `AT` (no newline usually, or try with `\r\n`).
    - If you receive `OK`, you are connected.
3.  **Change Baud Rate to 115200**:
    - For **DX-BT36** (BK3231 chip): Send `AT+BAUD8`.
      - *Note*: `4`=9600, `5`=19200, `6`=38400, `7`=57600, `8`=115200.
    - You should receive `+BAUD=115200` or `OK+BAUD8`.
4.  **Change Name (Optional)**:
    - Send `AT+NAME=CarPilot_GPS`.
    - You should receive `+NAME=CarPilot_GPS`.
5.  **Re-verify**:
    - Change your Serial Tool baudrate to `115200`.
    - Send `AT`.
    - If you get `OK`, configuration is complete!

---

<a name="chinese"></a>
## 🇨🇳 中文

蓝牙模块 (如 DX-BT36, JDY-31) 的波特率必须与 GNSS 模块完全一致 (**115200 bps**)，否则手机端无法收到正确数据。

### 1. 硬件准备
需要一个 **USB 转 TTL (串口) 模块**。

**配置时的接线：**
- **蓝牙 VCC** <-> **USB 5V/3.3V** (注意查看模块电压要求)
- **蓝牙 GND** <-> **USB GND**
- **蓝牙 TX**  <-> **USB RX**
- **蓝牙 RX**  <-> **USB TX**

### 2. 软件准备
任意串口调试助手 (如 **SSCOM**, **XCOM**, 或网页版串口工具)。

### 3. 配置步骤
1.  **连接**：打开串口工具，选择 COM 口。
    - **波特率**：尝试 `9600` (出厂默认) 或 `38400`。
2.  **测试连接**：
    - 发送 `AT` (注意：有些模块不需要回车换行，有些需要)。
    - 如果收到 `OK`，说明连接成功。
3.  **修改波特率到 115200**:
    - 对于 **DX-BT36** (BK3231芯片)：发送 `AT+BAUD8`。
      - *代码对照*: `4`=9600, `5`=19200, `6`=38400, `7`=57600, `8`=115200。
    - 应回复 `+BAUD=115200` 或 `OK+BAUD8`。
4.  **修改蓝牙名 (可选)**:
    - 发送 `AT+NAME=CarPilot_GPS`。
    - 应回复 `+NAME=CarPilot_GPS`。
5.  **验证**：
    - 将串口工具的波特率改为 `115200`。
    - 再次发送 `AT`。
    - 如果收到 `OK`，恭喜配置完成！
