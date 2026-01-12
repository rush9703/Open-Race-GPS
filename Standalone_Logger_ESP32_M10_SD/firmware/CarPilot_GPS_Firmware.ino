/*
 * CarPilot GPS Firmware v2.0
 *
 * ESP32 GPS Firmware optimized for CarPilot iOS App
 * - BLE: Nordic UART Service (NUS) Transparent NMEA
 * - WiFi: TCP Server Transparent NMEA
 * - SD: Local NMEA Logging
 *
 * Hardware: ESP32 + ATGM336H (or other NMEA GPS modules)
 */

#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <HardwareSerial.h>
#include <Preferences.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <esp_wifi.h> // For WiFi power management

// ================= 1. Hardware Pin Configuration =================
#define GPS_RX_PIN 16  // GPS TX -> ESP32 GPIO16
#define GPS_TX_PIN 17  // GPS RX -> ESP32 GPIO17
#define SD_CS_PIN 5    // SD Card Chip Select
#define LED_PIN 2      // Status LED
#define BUTTON_PIN 0   // Mode Switch Button (BOOT)
#define GPS_BAUD 38400 // GPS Module Baud Rate (Default)

// ================= 2. Operation Mode Definition =================
enum AppMode {
  MODE_BLE_LOG = 0,  // BLE Transparent (No SD Logging)
  MODE_WIFI_LOG = 1, // WiFi Transparent (No SD Logging)
  MODE_SD_ONLY = 2,  // SD Logging Only (No Communication)
  MODE_SYNC = 3      // WiFi File Management
};

AppMode currentMode = MODE_BLE_LOG;
Preferences prefs;

// ================= 3. Nordic UART Service UUIDs =================
// Must match UUIDs in iOS App RaceBoxManager.swift
#define NUS_SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_TX_CHAR_UUID                                                       \
  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // Data sent to phone
#define NUS_RX_CHAR_UUID                                                       \
  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // Data received from phone

// ================= 4. Global Objects =================
#define GPS_BUFFER_SIZE 512
#define NMEA_LINE_MAX 128

// GPS
HardwareSerial gpsSerial(2);
char nmeaBuffer[NMEA_LINE_MAX];
int nmeaIndex = 0;

// SD Card
File logFile;
bool isSdReady = false;
String currentLogFileName = "";

// WiFi
WiFiServer tcpServer(8080);
WiFiClient tcpClient;
WiFiClient apiClient;

// BLE
BLEServer *pBleServer = NULL;
BLECharacteristic *pBleTxChar = NULL;
bool bleConnected = false;
bool bleDeviceConnected = false;

// Status
unsigned long lastLedTime = 0;
unsigned long lastFlushTime = 0;
bool ledState = false;

// VBO Output Parsing State
double vboLat = 0;         // Latitude (Decimal Degrees)
double vboLon = 0;         // Longitude (Decimal Degrees)
double vboVelocity = 0;    // Speed (km/h)
double vboHeading = 0;     // Heading (Degrees)
double vboAltitude = 0;    // Altitude (Meters)
int vboSats = 0;           // Satellite Count
int vboFixQuality = 0;     // Fix Quality (0-6)
char vboTimeStr[16] = "";  // GPS Time (HHMMSS.SS)
char vboDateStr[16] = "";  // GPS Date (DDMMYY)
bool vboDataReady = false; // Data Ready Flag

// Non-blocking File Transfer State
bool isTransferring = false;     // Is transferring file
File transferFile;               // Current transfer file
size_t transferFileSize = 0;     // Total file size
size_t transferBytesSent = 0;    // Bytes sent
#define TRANSFER_CHUNK_SIZE 1024 // Send 1KB per chunk

// Non-blocking Clear State
bool isClearing = false;   // Is clearing SD card
File clearRootDir;         // Root dir handle for traversal
int clearDeletedCount = 0; // Deleted count

// GPS Boot State Machine
enum GpsState {
  GPS_SEARCHING,   // Searching Baud Rate/Ping
  GPS_CONFIGURING, // Configuring
  GPS_RUNNING      // Running Normally
};
GpsState gpsState = GPS_SEARCHING;
const long gpsSearchBauds[] = {38400}; // Only need 38400
int gpsBaudIdx = 0;
unsigned long lastGpsStateChange = 0;

// GPS Watchdog (Detects GPS disconnection and auto-recovers)
unsigned long lastGpsDataTime = 0;  // Last GPS data time
int consecutiveZeroDataPeriods = 0; // Consecutive zero data periods
int consecutiveNoValidPacketPeriods =
    0;                    // Periods with bytes but no valid packets
int validPacketCount = 0; // Valid packet count per period
#define GPS_WATCHDOG_TIMEOUT_PERIODS                                           \
  2 // Restart after 2 consecutive periods (10s) of no data/valid packets
#define GPS_WATCHDOG_CHECK_INTERVAL 5000 // Check every 5 seconds

const uint8_t UBX_POLL_VER[] = {0xB5, 0x62, 0x0A, 0x04,
                                0x00, 0x00, 0x0E, 0x34}; // UBX-MON-VER Poll

// ================= 5. BLE Callbacks =================
class BleServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override {
    bleDeviceConnected = true;
    bleConnected = true;
    Serial.println("[BLE] Client connected");
  }

  void onDisconnect(BLEServer *pServer) override {
    bleDeviceConnected = false;
    bleConnected = false;
    Serial.println("[BLE] Client disconnected");
    // Auto restart advertising
    pServer->startAdvertising();
  }
};

// ================= 6. LED Status Indication =================
void updateLED() {
  unsigned long now = millis();
  int interval = 1000;

  switch (currentMode) {
  case MODE_BLE_LOG:
    // BLE Mode: Slow blink (800ms)
    interval = bleConnected ? 200 : 800;
    break;
  case MODE_WIFI_LOG:
    // WiFi Mode: Fast blink (150ms)
    interval = tcpClient.connected() ? 100 : 150;
    break;
  case MODE_SD_ONLY:
    // SD Mode: Double blink
    if (now - lastLedTime > 2000)
      lastLedTime = now;
    if (now - lastLedTime < 100)
      digitalWrite(LED_PIN, HIGH);
    else if (now - lastLedTime < 200)
      digitalWrite(LED_PIN, LOW);
    else if (now - lastLedTime < 300)
      digitalWrite(LED_PIN, HIGH);
    else
      digitalWrite(LED_PIN, LOW);
    return;
  case MODE_SYNC:
    // Sync Mode: Solid on
    digitalWrite(LED_PIN, HIGH);
    return;
  }

  if (now - lastLedTime > interval) {
    lastLedTime = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }
}

// ================= 7. Initialize GPS =================
// UBX Binary Commands (u-blox M8N/M10)

// UBX-CFG-RATE: Set update rate to 10Hz (100ms)
const uint8_t UBX_10HZ[] = {
    0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64, 0x00, // measRate = 100ms
    0x01, 0x00,                                     // navRate = 1
    0x01, 0x00,                                     // timeRef = UTC
    0x7A, 0x12                                      // checksum
};

// UBX-CFG-PRT: Set UART1 Baud Rate to 115200
const uint8_t UBX_BAUD_115200[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01,
                                   0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00,
                                   0x00, 0xC2, 0x01, 0x00, 0x07, 0x00, 0x03,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x7E};

// UBX-CFG-CFG: Save Configuration to Flash
const uint8_t UBX_SAVE_CFG[] = {0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x03, 0x1D, 0xAB};

// UBX-CFG-MSG: Disable NMEA-GSV (Satellite Details) - on all ports
// Class=0xF0 (NMEA), ID=0x03 (GSV), Rate=0 for all ports
const uint8_t UBX_DISABLE_GSV[] = {
    0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, // NMEA-GSV
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             // disable on all ports
    0x02, 0x38                                      // checksum
};

// UBX-CFG-MSG: Disable NMEA-GSA (Satellite Dilution of Precision)
const uint8_t UBX_DISABLE_GSA[] = {
    0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x02, // NMEA-GSA
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x31  // checksum
};

// UBX-CFG-MSG: Disable NMEA-GLL (Geographic Position - Latitude/Longitude)
const uint8_t UBX_DISABLE_GLL[] = {
    0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, // NMEA-GLL
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A  // checksum
};

// UBX-CFG-MSG: Enable NMEA-RMC (Recommended Minimum Specific GPS/Transit Data)
const uint8_t UBX_ENABLE_RMC[] = {
    0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x04, // NMEA-RMC
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00,             // enable on UART1 only
    0x04, 0x46                                      // checksum
};

// UBX-CFG-MSG: Enable NMEA-VTG (Course Over Ground and Ground Speed)
const uint8_t UBX_ENABLE_VTG[] = {
    0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, // NMEA-VTG
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x05, 0x4F  // checksum
};

// UBX-CFG-MSG: Enable NMEA-GGA (Global Positioning System Fix Data)
const uint8_t UBX_ENABLE_GGA[] = {
    0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x00, // NMEA-GGA
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23  // checksum
};

void sendGPSCommand(const char *cmd) {
  gpsSerial.print(cmd);
  gpsSerial.print("\r\n");
  Serial.print("[GPS] Sent: ");
  Serial.println(cmd);
  delay(100);
}

// Helper: Send UBX Binary Command (Auto calculate checksum)
void sendUBX(uint8_t cls, uint8_t id, const uint8_t *payload, uint16_t len) {
  gpsSerial.write(0xB5);
  gpsSerial.write(0x62);
  gpsSerial.write(cls);
  gpsSerial.write(id);
  gpsSerial.write(len & 0xFF);
  gpsSerial.write((len >> 8) & 0xFF);

  uint8_t ck_a = 0, ck_b = 0;
  auto updateChecksum = [&](uint8_t b) {
    ck_a += b;
    ck_b += ck_a;
  };

  updateChecksum(cls);
  updateChecksum(id);
  updateChecksum(len & 0xFF);
  updateChecksum((len >> 8) & 0xFF);

  for (uint16_t i = 0; i < len; i++) {
    gpsSerial.write(payload[i]);
    updateChecksum(payload[i]);
  }

  gpsSerial.write(ck_a);
  gpsSerial.write(ck_b);
}

// Helper: Wait for UBX ACK (Cls=0x05, ID=0x01)
// Simple blocking wait (Only used during initialization)
bool waitForUBXAck(uint8_t cls, uint8_t id, unsigned long timeoutMs = 1000) {
  unsigned long start = millis();
  uint8_t step = 0;
  uint8_t ackCls = 0, ackId = 0;

  while (millis() - start < timeoutMs) {
    if (gpsSerial.available()) {
      uint8_t c = gpsSerial.read();
      switch (step) {
      case 0:
        if (c == 0xB5)
          step = 1;
        break;
      case 1:
        if (c == 0x62)
          step = 2;
        else
          step = 0;
        break;
      case 2:
        if (c == 0x05)
          step = 3;
        else
          step = 0;
        break; // ACK Class
      case 3:
        if (c == 0x01)
          step = 4;
        else
          step = 0;
        break; // ACK-ACK ID
      case 4:
        step = 5;
        break; // Len L
      case 5:
        step = 6;
        break; // Len H
      case 6:
        ackCls = c;
        step = 7;
        break; // Payload: Msg Cls
      case 7:
        ackId = c;
        step = 8;
        break; // Payload: Msg ID
      case 8:  // End
        if (ackCls == cls && ackId == id)
          return true;
        step = 0;
        break;
      }
    }
  }
  return false;
}

// Global UBX Parsing Variables (For VBO Generation)
uint8_t ubx_state = 0;
uint16_t ubx_len = 0;
uint16_t ubx_idx = 0;
uint8_t ubx_payload[120];
uint8_t ubx_cls, ubx_id;
uint8_t ubx_ck_a, ubx_ck_b;
volatile bool ubx_packet_ready =
    false; // Packet ready flag, trigger immediate send

// Helper: Send message to App (BLE/WiFi)
void notifyApp(String msg) {
  if (!msg.endsWith("\n"))
    msg += "\r\n";
  sendNMEA(msg.c_str(), msg.length());
  Serial.print("[AppNotify] ");
  Serial.println(msg);
}

// Boot Phase: Non-blocking GPS Initialization
void handleGpsBoot() {
  // Listen for GPS data
  if (gpsSerial.available()) {
    // Receive any data means baud rate is correct and GPS is alive
    if (gpsState == GPS_SEARCHING) {
      long currentBaud = gpsSearchBauds[gpsBaudIdx];
      Serial.printf("[GPS Boot] Data received at %ld! GPS Alive. Switching to "
                    "High Speed...\n",
                    currentBaud);
      gpsState = GPS_CONFIGURING;
      lastGpsStateChange = millis();

      notifyApp("$DEBUG,GPS Detected,Configuring 230400...*00");
    }
  }

  // State Machine
  if (gpsState == GPS_SEARCHING) {
    if (millis() - lastGpsStateChange > 1500) {
      // Search loop logic (Simplified for single baud, but keep retry)
      lastGpsStateChange = millis();
      gpsSerial.write(UBX_POLL_VER, sizeof(UBX_POLL_VER));
      notifyApp("$DEBUG,Waiting for GPS (38400)...*00");
    }
  } else if (gpsState == GPS_CONFIGURING) {
    // 1. Send command to change baud rate to 230400 (CFG-UART1-BAUDRATE =
    // 230400) Key: 0x40520001 Value: 230400 = 0x00038400. LE: 00 84 03 00
    uint8_t baudPayload[] = {0x01, 0x01, 0x00, 0x00, 0x01, 0x00,
                             0x52, 0x40, 0x00, 0x84, 0x03, 0x00};
    sendUBX(0x06, 0x8A, baudPayload, sizeof(baudPayload));

    Serial.println("[GPS Boot] Baud switch sent. Re-opening at 230400...");
    delay(200);
    gpsSerial.flush();
    gpsSerial.end();
    delay(100);

    // 2. Re-initialize Serial to 230400 with larger buffer
    gpsSerial.setRxBufferSize(
        1024); // Increase from default 256 to prevent overflow
    gpsSerial.begin(230400, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    delay(200);

    // 3. Send User Config and Wait for ACK (Retry 3 times)
    auto sendAndConfirm = [&](uint8_t *cmd, size_t size, const char *name) {
      for (int i = 0; i < 3; i++) {
        sendUBX(0x06, 0x8A, cmd, size);
        // VALSET returns ACK-ACK
        if (waitForUBXAck(0x06, 0x8A)) {
          Serial.printf("[GPS Boot] %s: OK\n", name);
          return true;
        }
        Serial.printf("[GPS Boot] %s: Timeout, Retry %d\n", name, i + 1);
      }
      Serial.printf("[GPS Boot] %s: FAILED\n", name);
      return false;
    };

    // Cmd 1
    uint8_t userCmd1[] = {0x01, 0x01, 0x00, 0x00, 0x24, 0x00, 0x31, 0x10, 0x00};
    sendAndConfirm(userCmd1, sizeof(userCmd1), "Signal");

    uint8_t userCmd2[] = {0x01, 0x01, 0x00, 0x00, 0x25, 0x00, 0x31, 0x10, 0x01};
    sendAndConfirm(userCmd2, sizeof(userCmd2), "Cmd2");

    uint8_t userCmd3[] = {0x01, 0x01, 0x00, 0x00, 0x18, 0x00, 0x31, 0x10, 0x01};
    sendAndConfirm(userCmd3, sizeof(userCmd3), "Cmd3");

    uint8_t userCmd4[] = {0x01, 0x01, 0x00, 0x00, 0x21, 0x00, 0x11, 0x20, 0x04};
    sendAndConfirm(userCmd4, sizeof(userCmd4), "Automotive");

    uint8_t userCmd5[] = {0x01, 0x01, 0x00, 0x00, 0x05, 0x00, 0x22, 0x20, 0x03};
    sendAndConfirm(userCmd5, sizeof(userCmd5), "Cmd5");

    uint8_t userCmd6[] = {0x01, 0x01, 0x00, 0x00, 0x01,
                          0x00, 0x21, 0x30, 0x64, 0x00};
    sendAndConfirm(userCmd6, sizeof(userCmd6), "10Hz");

    uint8_t userCmd7[] = {0x01, 0x01, 0x00, 0x00, 0x07, 0x00, 0x91, 0x20, 0x01};
    sendAndConfirm(userCmd7, sizeof(userCmd7), "EnablePVT");

    // Static Hold Threshold: 0.5 km/h = 14 cm/s
    // Key: CFG-MOT-GNSSSPEED_THRS (0x20250038), Type: U1, Value: 14
    uint8_t staticHoldCmd[] = {0x01, 0x01, 0x00, 0x00, 0x38,
                               0x00, 0x25, 0x20, 0x00};
    sendAndConfirm(staticHoldCmd, sizeof(staticHoldCmd), "StaticHold 0.5kph");

    // 4. Disable NMEA (No need to ACK strict check, just send)
    // RMC, GGA, GSA, GSV on UART1
    uint8_t disableNMEA_RMC[] = {0x01, 0x01, 0x00, 0x00, 0xAC,
                                 0x00, 0x91, 0x20, 0x00};
    sendUBX(0x06, 0x8A, disableNMEA_RMC, 9);
    delay(20);
    uint8_t disableNMEA_GGA[] = {0x01, 0x01, 0x00, 0x00, 0xBB,
                                 0x00, 0x91, 0x20, 0x00};
    sendUBX(0x06, 0x8A, disableNMEA_GGA, 9);
    delay(20);
    uint8_t disableNMEA_GSA[] = {0x01, 0x01, 0x00, 0x00, 0xC0,
                                 0x00, 0x91, 0x20, 0x00};
    sendUBX(0x06, 0x8A, disableNMEA_GSA, 9);
    delay(20);
    uint8_t disableNMEA_GSV[] = {0x01, 0x01, 0x00, 0x00, 0xC5,
                                 0x00, 0x91, 0x20, 0x00};
    sendUBX(0x06, 0x8A, disableNMEA_GSV, 9);
    delay(20);

    // 5. Save Config
    uint8_t saveCfg[] = {0xFF, 0xFF, 0x00, 0x00};
    sendUBX(0x06, 0x09, saveCfg, 4);
    waitForUBXAck(0x06, 0x09); // Wait for save ack

    Serial.println("[GPS Boot] Config Written. GPS Ready at 230400.");
    notifyApp("$DEBUG,GPS Config Params Written (230400)*00");
    gpsState = GPS_RUNNING;

    // Clear buffer
    while (gpsSerial.available())
      gpsSerial.read();
  }
}

void initGPS() {
  gpsSerial.setRxBufferSize(GPS_BUFFER_SIZE);
  // Initial try first baud rate
  gpsBaudIdx = 0;
  Serial.printf("[GPS Boot] Start Search at %ld\n", gpsSearchBauds[0]);
  gpsSerial.begin(gpsSearchBauds[0], SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  gpsState = GPS_SEARCHING;
  lastGpsStateChange = millis();
}

// ================= 8. Initialize BLE =================
void initBLE() {
  if (pBleServer != NULL) {
    // Already initialized, just restart advertising
    BLEDevice::startAdvertising();
    Serial.println("[BLE] Restarted advertising");
    return;
  }

  BLEDevice::init("CarPilot_Pro"); // Device Name

  // Set TX power to Max (9dBm)
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL0, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);

  pBleServer = BLEDevice::createServer();
  pBleServer->setCallbacks(new BleServerCallbacks());

  // Create Nordic UART Service
  BLEService *pService = pBleServer->createService(NUS_SERVICE_UUID);

  // TX Characteristic (ESP32 -> Phone)
  pBleTxChar = pService->createCharacteristic(
      NUS_TX_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  pBleTxChar->addDescriptor(new BLE2902());

  // RX Characteristic (Phone -> ESP32) - Not used yet
  BLECharacteristic *pRxChar = pService->createCharacteristic(
      NUS_RX_CHAR_UUID,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);

  pService->start();

  // Start Advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(NUS_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("[BLE] NUS Service started, advertising...");
}

void stopBLE() {
  if (pBleServer) {
    pBleServer->getAdvertising()->stop();
    // Fully reset BLE state, force re-init next time
    BLEDevice::deinit(false); // false = don't release memory (faster restart)
    pBleServer = NULL;
    pBleTxChar = NULL;
    bleConnected = false;
    bleDeviceConnected = false;
    Serial.println("[BLE] Stopped and reset");
  }
}

// ================= 9. Send NMEA Data =================
// WiFi TX statistics
static unsigned long wifiTxBytes = 0;
static unsigned long wifiTxPackets = 0;
static unsigned long lastWifiDebug = 0;

void sendNMEA(const char *sentence, int len) {
  // Debug: Print status every 5 seconds
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 5000) {
    lastDebug = millis();
    Serial.printf("[Debug] Mode=%d, BLE=%d, TxChar=%s\n", currentMode,
                  bleConnected, pBleTxChar ? "OK" : "NULL");
  }

  // A. BLE Mode
  if (currentMode == MODE_BLE_LOG && bleConnected && pBleTxChar) {
    // BLE MTU Limit, send in chunks (Max 20 bytes per chunk)
    int offset = 0;
    while (offset < len) {
      int chunkSize = min(20, len - offset);
      pBleTxChar->setValue((uint8_t *)(sentence + offset), chunkSize);
      pBleTxChar->notify();
      offset += chunkSize;
      // Small delay to prevent packet loss
      delayMicroseconds(500);
    }
  }

  // B. WiFi Mode
  else if (currentMode == MODE_WIFI_LOG) {
    // Check if client is connected
    if (tcpClient && tcpClient.connected()) {
      // Write directly, let underlying layer handle buffering
      size_t written = tcpClient.write((const uint8_t *)sentence, len);
      if (written > 0) {
        wifiTxBytes += written;
        wifiTxPackets++;
      }

      // Print WiFi TX stats every 5 seconds
      if (millis() - lastWifiDebug > 5000) {
        lastWifiDebug = millis();
        Serial.printf("[WiFi TX] Sent %lu bytes, %lu packets\n", wifiTxBytes,
                      wifiTxPackets);
        wifiTxBytes = 0;
        wifiTxPackets = 0;
      }
    }
  }
}

// ================= 10. Process GPS Data =================

// Parse NMEA Coordinate (DDMM.MMMM format) to Decimal Degrees
double parseNmeaCoord(const char *str, char dir) {
  if (str[0] == '\0')
    return 0;
  double val = atof(str);
  int deg = (int)(val / 100);
  double min = val - deg * 100;
  double result = deg + min / 60.0;
  if (dir == 'S' || dir == 'W')
    result = -result;
  return result;
}

// Parse NMEA Sentence and Update VBO State
void parseNMEAForVBO(char *sentence) {
  // Copy sentence for parsing (strtok modifies original string)
  char buf[NMEA_LINE_MAX];
  strncpy(buf, sentence, NMEA_LINE_MAX - 1);
  buf[NMEA_LINE_MAX - 1] = '\0';

  // Split fields
  char *fields[20];
  int fieldCount = 0;
  char *token = strtok(buf, ",*");
  while (token && fieldCount < 20) {
    fields[fieldCount++] = token;
    token = strtok(NULL, ",*");
  }

  if (fieldCount < 2)
    return;

  // $GNRMC or $GPRMC - Time, Position, Speed, Heading
  if (strstr(fields[0], "RMC") && fieldCount >= 10) {
    // Field 1: Time HHMMSS.SS
    if (strlen(fields[1]) >= 6) {
      strncpy(vboTimeStr, fields[1], 15);
      vboTimeStr[15] = '\0';
    }
    // Field 2: Status (A=Valid)
    if (fields[2][0] != 'A')
      return;

    // Field 3,4: Latitude
    if (strlen(fields[3]) > 0) {
      vboLat = parseNmeaCoord(fields[3], fields[4][0]);
    }
    // Field 5,6: Longitude
    if (strlen(fields[5]) > 0) {
      vboLon = parseNmeaCoord(fields[5], fields[6][0]);
    }
    // Field 7: Speed (Knots)
    if (strlen(fields[7]) > 0) {
      vboVelocity = atof(fields[7]) * 1.852; // Convert to km/h
    }
    // Field 8: Heading
    if (strlen(fields[8]) > 0) {
      vboHeading = atof(fields[8]);
    }

    // Field 9: Date (DDMMYY)
    if (strlen(fields[9]) >= 6) {
      strncpy(vboDateStr, fields[9], 15);
      vboDateStr[15] = '\0';
    }

    vboDataReady = true;
    validPacketCount++; // Watchdog: Count valid packets

    // Write VBO data line ONLY in SD mode (WiFi mode doesn't write SD to avoid
    // blocking)
    if (currentMode == MODE_SD_ONLY) {
      writeVBODataLine();

      // In VBO mode, flushing here replaces flushing in sendNMEA
      if (currentMode == MODE_SD_ONLY && isSdReady && logFile) {
        if (millis() - lastFlushTime > 1000) {
          logFile.flush();
          lastFlushTime = millis();
        }
      }
    }
  }

  // $GNGGA or $GPGGA - Satellites, Altitude, Fix Quality
  else if (strstr(fields[0], "GGA") && fieldCount >= 10) {
    // Field 6: Fix Quality (0=Invalid, 1=GPS, 2=DGPS, ...)
    if (strlen(fields[6]) > 0) {
      vboFixQuality = atoi(fields[6]);
    }
    // Field 7: Satellite Count
    if (strlen(fields[7]) > 0) {
      vboSats = atoi(fields[7]);
    }
    // Field 9: Altitude
    if (strlen(fields[9]) > 0) {
      vboAltitude = atof(fields[9]);
    }
  }

  // $GNVTG or $GPVTG - Speed (More precise)
  else if (strstr(fields[0], "VTG") && fieldCount >= 8) {
    // Field 1: Heading
    if (strlen(fields[1]) > 0) {
      vboHeading = atof(fields[1]);
    }
    // Field 7: Speed km/h
    if (strlen(fields[7]) > 0) {
      vboVelocity = atof(fields[7]);
    }
  }
}

void processUBXForVBO(uint8_t c) {
  switch (ubx_state) {
  case 0:
    if (c == 0xB5)
      ubx_state = 1;
    break;
  case 1:
    if (c == 0x62)
      ubx_state = 2;
    else
      ubx_state = 0;
    break;
  case 2:
    ubx_cls = c;
    ubx_state = 3;
    break;
  case 3:
    ubx_id = c;
    ubx_state = 4;
    break;
  case 4:
    ubx_len = c;
    ubx_state = 5;
    break;
  case 5:
    ubx_len |= (c << 8);
    if (ubx_len > 110)
      ubx_state = 0; // Limit buffer
    else {
      ubx_idx = 0;
      ubx_state = 6;
    }
    break;
  case 6:
    ubx_payload[ubx_idx++] = c;
    if (ubx_idx == ubx_len)
      ubx_state = 7;
    break;
  case 7:
    ubx_ck_a = c;
    ubx_state = 8;
    break;
  case 8:
    ubx_ck_b = c;
    // NAV-PVT (0x01 0x07) Length 92
    if (ubx_cls == 0x01 && ubx_id == 0x07 && ubx_len == 92) {
      // Offset mapping (M8/M10 PVT)
      // 20: fixType (1)
      // 23: numSV (1)
      // 24: Lon (4), 28: Lat (4)
      // 36: hMSL (4)
      // 60: gSpeed (4)
      // 0: iTOW, 4: Year(2), 6: Month(1), 7: Day, 8: Hour, 9: Min,
      // 10: Sec

      uint16_t year = ubx_payload[4] | (ubx_payload[5] << 8);
      uint8_t month = ubx_payload[6];
      uint8_t day = ubx_payload[7];
      uint8_t hour = ubx_payload[8];
      uint8_t min = ubx_payload[9];
      uint8_t sec = ubx_payload[10];
      // Nanoseconds at offset 16-19 (signed 32-bit)
      int32_t nano = (int32_t)((uint32_t)ubx_payload[16] |
                               ((uint32_t)ubx_payload[17] << 8) |
                               ((uint32_t)ubx_payload[18] << 16) |
                               ((uint32_t)ubx_payload[19] << 24));
      // Convert nanoseconds to milliseconds (0-999)
      int32_t millis_val = nano / 1000000;

      if (millis_val < 0)
        millis_val = 0;
      if (millis_val > 999)
        millis_val = 999;

      long lon = (long)ubx_payload[24] | ((long)ubx_payload[25] << 8) |
                 ((long)ubx_payload[26] << 16) | ((long)ubx_payload[27] << 24);
      long lat = (long)ubx_payload[28] | ((long)ubx_payload[29] << 8) |
                 ((long)ubx_payload[30] << 16) | ((long)ubx_payload[31] << 24);
      long hMSL = (long)ubx_payload[36] | ((long)ubx_payload[37] << 8) |
                  ((long)ubx_payload[38] << 16) | ((long)ubx_payload[39] << 24);
      long gSpd = (long)ubx_payload[60] | ((long)ubx_payload[61] << 8) |
                  ((long)ubx_payload[62] << 16) | ((long)ubx_payload[63] << 24);
      long head = (long)ubx_payload[64] | ((long)ubx_payload[65] << 8) |
                  ((long)ubx_payload[66] << 16) | ((long)ubx_payload[67] << 24);

      vboLon = lon * 1e-7;
      vboLat = lat * 1e-7;
      vboAltitude = hMSL / 1000.0;
      vboVelocity = gSpd / 1000.0 * 3.6; // m/s to km/h
      vboHeading = head * 1e-5;
      vboSats = ubx_payload[23];
      vboFixQuality = (ubx_payload[20] >= 3) ? 1 : 0; // 3D Fix

      sprintf(vboDateStr, "%02d%02d%02d", day, month, year % 100);
      // Include milliseconds in time string for proper VBO timestamp
      sprintf(vboTimeStr, "%02d%02d%02d.%03d", hour, min, sec, (int)millis_val);
      vboDataReady = true;
      validPacketCount++; // Watchdog: Count valid packets

      if (currentMode == MODE_SD_ONLY) {
        writeVBODataLine();
        // Regular Flush
        static unsigned long lastFlush = 0;
        if (millis() - lastFlush > 1000) {
          logFile.flush();
          lastFlush = millis();
        }
      }
    }
    ubx_state = 0;
    ubx_packet_ready = true; // Mark packet ready, trigger immediate send
    break;
  }
}

void processGPS() {
  static unsigned long lastGpsDebug = 0;
  static int gpsCount = 0;

  // Timing debug
  static unsigned long maxReadTime = 0;
  static unsigned long maxWriteTime = 0;
  // Buffer: Batch forward data (Efficiency)
  static uint8_t txBuffer[256]; // Increased buffer
  static uint8_t txIndex = 0;
  bool needFlush = false;

  // 1. Fast read all available data to buffer (Non-blocking)
  while (gpsSerial.available()) {
    uint8_t c = gpsSerial.read();
    gpsCount++;

    // Parse UBX-NAV-PVT for SD Card VBO logging
    processUBXForVBO(c);

    // Buffer data for forwarding (WiFi/BLE)
    if (currentMode == MODE_WIFI_LOG || currentMode == MODE_BLE_LOG) {
      if (txIndex < sizeof(txBuffer)) {
        txBuffer[txIndex++] = c;
      }
      // Mark flush needed (Buffer full or packet ready)
      if (txIndex >= 200 || ubx_packet_ready) {
        needFlush = true;
      }
    }
  }

  // 2. Send data outside loop (With timeout protection)
  if (needFlush || ubx_packet_ready) {
    ubx_packet_ready = false;
    if (currentMode == MODE_WIFI_LOG && tcpClient && tcpClient.connected()) {
      // Set write timeout 10ms (Avoid long block)
      tcpClient.setTimeout(10);
      unsigned long t0 = micros();
      size_t written = tcpClient.write(txBuffer, txIndex);
      unsigned long wt = micros() - t0;
      if (wt > maxWriteTime)
        maxWriteTime = wt;
      // If write > 50ms, log warning
      if (wt > 50000) {
        Serial.printf("[WiFi] Write blocked: %lums\n", wt / 1000);
      }
    } else if (currentMode == MODE_BLE_LOG && bleConnected && pBleTxChar) {
      for (int i = 0; i < txIndex; i += 20) {
        int chunkSize = min(20, (int)(txIndex - i));
        pBleTxChar->setValue(txBuffer + i, chunkSize);
        pBleTxChar->notify();
        delayMicroseconds(500);
      }
    }
    txIndex = 0;
  }

  if (millis() - lastGpsDebug > 5000) {
    Serial.printf("[GPS] Bytes:%d Packets:%d MaxWrite:%luus\n", gpsCount,
                  validPacketCount, maxWriteTime);

    // GPS Watchdog Logic
    if (gpsState == GPS_RUNNING) {
      bool needRestart = false;

      // Case 1: No bytes at all (Hardware disconnected)
      if (gpsCount == 0) {
        consecutiveZeroDataPeriods++;
        consecutiveNoValidPacketPeriods = 0; // Reset other counter
        Serial.printf("[GPS] Warning: No bytes for %d periods\n",
                      consecutiveZeroDataPeriods);

        if (consecutiveZeroDataPeriods >= GPS_WATCHDOG_TIMEOUT_PERIODS) {
          Serial.println("[GPS] WATCHDOG: No data - hardware disconnected!");
          needRestart = true;
        }
      }
      // Case 2: Bytes received but 0 valid packets (Parser desync)
      else if (validPacketCount == 0) {
        consecutiveNoValidPacketPeriods++;
        consecutiveZeroDataPeriods = 0; // Reset other counter
        Serial.printf("[GPS] Warning: %d bytes but 0 valid packets for %d "
                      "periods (parser desync?)\n",
                      gpsCount, consecutiveNoValidPacketPeriods);

        if (consecutiveNoValidPacketPeriods >= GPS_WATCHDOG_TIMEOUT_PERIODS) {
          Serial.println("[GPS] WATCHDOG: Parser desync detected!");
          needRestart = true;
        }
      }
      // Case 3: Normal - Data and Valid Packets
      else {
        consecutiveZeroDataPeriods = 0;
        consecutiveNoValidPacketPeriods = 0;
        lastGpsDataTime = millis();
      }

      // Execute Restart
      if (needRestart) {
        notifyApp("$DEBUG,GPS Watchdog Restart*00");

        // Clear buffer and parser state
        while (gpsSerial.available())
          gpsSerial.read();
        nmeaIndex = 0; // Reset NMEA parser

        // Restart GPS State Machine
        gpsSerial.end();
        delay(100);
        initGPS();

        consecutiveZeroDataPeriods = 0;
        consecutiveNoValidPacketPeriods = 0;
      }
    }

    maxWriteTime = 0;
    gpsCount = 0;
    validPacketCount = 0; // Reset valid packet count
    lastGpsDebug = millis();
  }
}

// ================= 11. SD File Management =================
String getNextLogFileName() {
  int i = 1;
  while (true) {
    String fname = "/Track_" + String(i) + ".vbo";
    if (!SD.exists(fname))
      return fname;
    i++;
  }
}

void openLogFile() {
  if (isSdReady && currentMode != MODE_SYNC) {

    // Try to use GPS Time for filename: /20YYMMDD_HHMMSS.vbo
    // vboDateStr: DDMMYY, vboTimeStr: HHMMSS.SS
    if (strlen(vboDateStr) == 6 && strlen(vboTimeStr) >= 6) {
      char fname[32];
      // DDMMYY -> 20YYMMDD
      // Date: 0=D,1=D,2=M,3=M,4=Y,5=Y
      // Time: 0=H,1=H,2=M,3=M,4=S,5=S
      sprintf(fname, "/20%c%c%c%c%c%c_%c%c%c%c%c%c.vbo", vboDateStr[4],
              vboDateStr[5],                // YY
              vboDateStr[2], vboDateStr[3], // MM
              vboDateStr[0], vboDateStr[1], // DD
              vboTimeStr[0], vboTimeStr[1], // HH
              vboTimeStr[2], vboTimeStr[3], // MM
              vboTimeStr[4], vboTimeStr[5]  // SS
      );
      currentLogFileName = String(fname);
    } else {
      // Fallback
      currentLogFileName = getNextLogFileName();
    }

    logFile = SD.open(currentLogFileName, FILE_WRITE);
    if (logFile) {
      Serial.println("[SD] Logging to: " + currentLogFileName);

      // Write VBO header with correct time
      // RMC Date: DDMMYY. VBO Header: DD/MM/YYYY
      if (strlen(vboDateStr) == 6 && strlen(vboTimeStr) >= 6) {
        logFile.printf("File created on %c%c/%c%c/20%c%c @ %c%c:%c%c:%c%c\n",
                       vboDateStr[0], vboDateStr[1], // DD
                       vboDateStr[2], vboDateStr[3], // MM
                       vboDateStr[4], vboDateStr[5], // YY
                       vboTimeStr[0], vboTimeStr[1], // HH
                       vboTimeStr[2], vboTimeStr[3], // MM
                       vboTimeStr[4], vboTimeStr[5]  // SS
        );
      } else {
        logFile.println("File created on 01/01/2024 @ 00:00:00");
      }

      logFile.println();
      logFile.println("[header]");
      logFile.println();
      logFile.println("[comments]");
      logFile.println("Type : CarPilot GPS Box");
      logFile.printf("Serial : ESP32-%08X\n",
                     (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF));
      logFile.println("Firmware : 2.0");
      logFile.println("Log rate (Hz) : 10");
      logFile.println();
      logFile.println("[column names]");
      logFile.println("sats time lat long velocity heading height vert-vel "
                      "longacc latacc solution_type");
      logFile.println();
      logFile.println("[laptiming]");
      logFile.println();
      logFile.println("[data]");
      logFile.flush();
    }
  }
}

void closeLogFile() {
  if (logFile) {
    logFile.flush();
    logFile.close();
    Serial.println("[SD] File closed");
  }
}

// Write VBO Data Line
void writeVBODataLine() {
  if (!logFile || vboLat == 0 || vboLon == 0)
    return;

  // Convert Decimal Degrees to DMM Format (Standard VBO: DDMM.mmmm)
  // DMM = Degrees * 100 + Minutes
  double latAbs = fabs(vboLat);
  int latDeg = (int)latAbs;
  double latMin = (latAbs - latDeg) * 60.0;
  double latDMM = (latDeg * 100.0) + latMin;

  if (vboLat < 0)
    latDMM = -latDMM; // Negative for South Latitude

  // VBO Longitude Sign Convention: East is Negative, West is Positive
  double lonAbs = fabs(vboLon);
  int lonDeg = (int)lonAbs;
  double lonMin = (lonAbs - lonDeg) * 60.0;
  double lonDMM = (lonDeg * 100.0) + lonMin;

  if (vboLon > 0)
    lonDMM = -lonDMM; // East Longitude is Negative in VBO

  // Format: sats time lat long velocity heading height vert-vel longacc latacc
  // solution_type
  logFile.printf("%d %s %.5f %.5f %.2f %.1f %.1f 0.00 0.00 0.00 %d\n", vboSats,
                 vboTimeStr, latDMM, lonDMM, vboVelocity, vboHeading,
                 vboAltitude, vboFixQuality > 0 ? vboFixQuality : 1);
}

// ================= 12. File Management API (SYNC Mode) =================
String getJsonValue(String json, String key) {
  int keyIdx = json.indexOf("\"" + key + "\"");
  if (keyIdx == -1)
    return "";
  int valStart = json.indexOf(":", keyIdx) + 1;
  while (json[valStart] == ' ' || json[valStart] == '"')
    valStart++;
  int valEnd = valStart;
  while (valEnd < json.length() && json[valEnd] != '"' && json[valEnd] != ',' &&
         json[valEnd] != '}')
    valEnd++;
  return json.substring(valStart, valEnd);
}

void handleSyncAPI() {
  if (tcpServer.hasClient()) {
    if (apiClient)
      apiClient.stop();
    apiClient = tcpServer.available();
  }

  if (apiClient && apiClient.connected() && apiClient.available()) {
    String req = apiClient.readStringUntil('\n');
    req.trim();
    String cmd = getJsonValue(req, "cmd");

    if (cmd == "list") {
      String jsonResponse = "{\"files\":[";
      File root = SD.open("/");
      File file = root.openNextFile();
      bool first = true;
      while (file) {
        if (!file.isDirectory()) {
          String fname = String(file.name());
          if (fname.endsWith(".nmea") || fname.endsWith(".NMEA") ||
              fname.endsWith(".vbo") || fname.endsWith(".VBO")) {
            if (!first)
              jsonResponse += ",";
            jsonResponse += "{\"name\":\"" + fname +
                            "\",\"size\":" + String(file.size()) + "}";
            first = false;
          }
        }
        file = root.openNextFile();
      }
      jsonResponse += "]}";

      // Debug: Print Sent JSON
      Serial.printf("[SYNC] Sending list JSON (%d bytes): %s\n",
                    jsonResponse.length(), jsonResponse.c_str());

      apiClient.println(jsonResponse);
    } else if (cmd == "del") {
      // Reject delete if busy
      if (isTransferring || isClearing) {
        Serial.println("[SYNC] Del rejected: BUSY");
        apiClient.println("{\"status\":\"busy\"}");
        return;
      }

      String path = getJsonValue(req, "path");
      if (!path.startsWith("/"))
        path = "/" + path;
      Serial.printf("[SYNC] Deleting file: %s ... ", path.c_str());
      if (SD.exists(path)) {
        SD.remove(path);
        Serial.println("OK");
        apiClient.println("{\"status\":\"ok\"}");
      } else {
        Serial.println("Not Found");
        apiClient.println("{\"status\":\"err\"}");
      }
    } else if (cmd == "get") {
      // Reject new get if busy
      if (isTransferring || isClearing) {
        Serial.println("[SYNC] Get rejected: BUSY");
        apiClient.println("ERR:BUSY");
        return;
      }

      String path = getJsonValue(req, "path");
      if (!path.startsWith("/"))
        path = "/" + path;
      Serial.printf("[SYNC] Request download: %s\n", path.c_str());

      if (SD.exists(path)) {
        // Non-blocking Transfer: Open file and send SIZE, then stream in loop()
        transferFile = SD.open(path, FILE_READ);
        if (transferFile) {
          transferFileSize = transferFile.size();
          transferBytesSent = 0;
          isTransferring = true;

          // Send SIZE Header
          apiClient.printf("SIZE:%lu\n", transferFileSize);
          apiClient.setNoDelay(true); // Disable Nagle
          Serial.printf(
              "[SYNC] Sending SIZE header. Starting transfer: %s (%lu bytes)\n",
              path.c_str(), transferFileSize);
        } else {
          Serial.printf("[SYNC] Error opening: %s\n", path.c_str());
          apiClient.println("ERR:OPEN");
        }
      } else {
        Serial.printf("[SYNC] File not found: %s\n", path.c_str());
        apiClient.println("ERR:404");
      }
    } else if (cmd == "clear_all") {
      // Clear All: Non-blocking, delete files one by one in loop()
      if (isTransferring || isClearing) {
        Serial.println("[SYNC] ClearAll rejected: BUSY");
        apiClient.println("{\"status\":\"busy\"}");
        return;
      }

      // Reply Processing Started
      apiClient.println(
          "{\"status\":\"processing\",\"msg\":\"Starting clear...\"}");

      // Prepare
      clearRootDir = SD.open("/");
      isClearing = true;
      clearDeletedCount = 0;
      Serial.println("[SYNC] Start clearing all files (Task Started)...");
    }
  }
}

// Non-blocking File Transfer Process (Send one chunk at a time)
void processFileTransfer() {
  if (!isTransferring || !transferFile || !apiClient.connected()) {
    // Transfer Ended or Client Disconnected
    if (isTransferring) {
      isTransferring = false;
      if (transferFile) {
        transferFile.close();
      }
      Serial.println("[SYNC] Transfer aborted (client disconnected)");
    }
    return;
  }

  // Check if data available
  if (transferFile.available()) {
    uint8_t buf[TRANSFER_CHUNK_SIZE];
    int toRead = min((int)TRANSFER_CHUNK_SIZE, transferFile.available());
    int bytesRead = transferFile.read(buf, toRead);

    if (bytesRead > 0) {
      size_t written = apiClient.write(buf, bytesRead);
      transferBytesSent += written;

      // LED Feedback (Fast Blink)
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
  }

  // Check if transfer complete
  if (!transferFile.available() || transferBytesSent >= transferFileSize) {
    transferFile.close();
    isTransferring = false;
    digitalWrite(LED_PIN, LOW);
    Serial.printf("[SYNC] Transfer complete: %lu bytes sent\n",
                  transferBytesSent);
  }
}

// Non-blocking Clear Process (Delete one file at a time)
void processClearTask() {
  if (!isClearing)
    return;

  File file = clearRootDir.openNextFile();

  if (file) {
    if (!file.isDirectory()) {
      String path = String("/") + String(file.name());
      file.close(); // Close before removing

      // Only delete .vbo and .nmea files, protect system files
      if (path.endsWith(".vbo") || path.endsWith(".VBO") ||
          path.endsWith(".nmea") || path.endsWith(".NMEA")) {
        SD.remove(path);
        clearDeletedCount++;

        // LED Feedback (Fast Blink)
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        Serial.printf("[Clear] Deleted: %s\n", path.c_str());
      }
    } else {
      file.close();
    }
  } else {
    // Traversal Done
    isClearing = false;
    clearRootDir.close();
    digitalWrite(LED_PIN, HIGH);
    Serial.printf("[Clear] Done. Removed %d files.\n", clearDeletedCount);
  }
}

// ================= 13. Mode Switching =================
void switchMode(AppMode newMode) {
  Serial.printf("[Mode] Switching to mode %d\n", newMode);

  // Close existing resources
  closeLogFile();
  WiFi.softAPdisconnect(true);
  stopBLE();
  if (tcpClient)
    tcpClient.stop();

  currentMode = newMode;
  prefs.putInt("mode", (int)currentMode);

  switch (currentMode) {
  case MODE_BLE_LOG:
    initBLE();
    // No SD Logging, reduce latency
    Serial.println("[Mode] BLE Only (No SD)");
    break;

  case MODE_WIFI_LOG:
    // User fixed channel to avoid interference (1, 6, 11 are non-overlapping)
    WiFi.softAP("CarPilot_Live", "12345678", 1); // Channel 1
    WiFi.setTxPower(WIFI_POWER_19_5dBm);         // Max TX Power

    // Disable WiFi Power Save, improve latency
    esp_wifi_set_ps(WIFI_PS_NONE);

    tcpServer.begin();
    tcpServer.setNoDelay(true); // Disable Nagle at TCP Server level

    // No SD Logging, reduce latency
    Serial.printf("[Mode] WiFi Only (Ch1, No Sleep), IP: %s\n",
                  WiFi.softAPIP().toString().c_str());
    break;

  case MODE_SD_ONLY:
    WiFi.mode(WIFI_OFF);
    openLogFile();
    Serial.println("[Mode] SD Recording Only");
    break;

  case MODE_SYNC:
    WiFi.softAP("CarPilot_Sync", "12345678");
    tcpServer.begin();
    Serial.printf("[Mode] Sync Mode, IP: %s\n",
                  WiFi.softAPIP().toString().c_str());
    break;
  }
}

// ================= 14. Button Handling =================
void handleButton() {
  static unsigned long btnStart = 0;
  static bool btnHandled = false;

  if (digitalRead(BUTTON_PIN) == LOW) {
    if (btnStart == 0)
      btnStart = millis();

    // Long Press 2s: Enter/Exit SYNC Mode (Blocked during transfer)
    if (millis() - btnStart > 2000 && !btnHandled) {
      if (isTransferring) {
        // Block mode switch during transfer
        Serial.println("[Button] Mode switch blocked (transfer in progress)");
        btnHandled = true;
        return;
      }
      btnHandled = true;
      if (currentMode == MODE_SYNC) {
        switchMode(MODE_BLE_LOG);
      } else {
        switchMode(MODE_SYNC);
      }
      // Blink Feedback
      for (int i = 0; i < 5; i++) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(50);
      }
    }
  } else {
    // Short Press: Switch between BLE/WiFi/SD modes
    if (btnStart > 0 && !btnHandled) {
      if (currentMode != MODE_SYNC) {
        int next = (currentMode + 1) % 3;
        switchMode((AppMode)next);
      }
    }
    btnStart = 0;
    btnHandled = false;
  }
}

// ================= 15. Setup =================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== CarPilot GPS v2.0 ===");

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Read saved mode
  prefs.begin("carpilot", false);
  int savedMode = prefs.getInt("mode", 0);
  if (savedMode == MODE_SYNC)
    savedMode = MODE_BLE_LOG; // Don't auto-enter SYNC

  // Initialize SD Card
  SPI.begin(18, 19, 23, SD_CS_PIN);
  if (SD.begin(SD_CS_PIN)) {
    isSdReady = true;
    Serial.println("[SD] Card initialized");
  } else {
    Serial.println("[SD] Card failed or not present");
  }

  // Initialize GPS (Non-blocking Search)
  initGPS();

  // Switch to saved mode
  // switchMode((AppMode)savedMode);
  switchMode(MODE_WIFI_LOG); // Force WiFi mode for testing
}

// ================= 16. Loop =================
void loop() {
  handleButton();
  updateLED();

  // Check if WiFi client disconnected (Log Mode)
  if (currentMode == MODE_WIFI_LOG && tcpClient && !tcpClient.connected()) {
    Serial.println("[WiFi] Client disconnected");
    tcpClient.stop();
    tcpClient = WiFiClient(); // Reset to empty client
  }

  // Handle New WiFi Client Connection (WiFi Mode Only)
  if ((currentMode == MODE_WIFI_LOG || currentMode == MODE_SYNC) &&
      tcpServer.hasClient()) {
    WiFiClient newClient = tcpServer.available();
    newClient.setNoDelay(true); // Disable Nagle

    if (currentMode == MODE_WIFI_LOG) {
      // Transparent Mode: Only supports single client
      if (tcpClient && tcpClient.connected()) {
        Serial.println("[WiFi] New client rejected (Already connected)");
        newClient.stop();
      } else {
        tcpClient = newClient;
        Serial.println("[WiFi] New client connected (Log Mode)");
      }
    } else {
      // Sync Mode
      if (apiClient)
        apiClient.stop();
      apiClient = newClient;
    }
  }

  if (currentMode == MODE_SYNC) {
    handleSyncAPI();
    // Non-blocking File Transfer
    if (isTransferring) {
      processFileTransfer();
    }
    // Non-blocking Clear Task
    if (isClearing) {
      processClearTask();
    }
  } else {
    // GPS State Machine Logic
    if (gpsState == GPS_RUNNING) {
      processGPS();
    } else {
      handleGpsBoot();
    }
  }
}
