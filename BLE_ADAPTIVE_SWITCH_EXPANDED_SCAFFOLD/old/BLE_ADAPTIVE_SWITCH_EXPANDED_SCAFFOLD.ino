// =====================================================================
// BLE ADAPTIVE SWITCH NODE — EXPANDED SCAFFOLD  (v2, board-verified)
// Board : ESP32-S3 SuperMini  (ESP32S3FH4R2 — in-package flash+PSRAM)
// Target: NimBLE HID keyboard over BLE → iPadOS Switch Control
//
// Pin layout — verified against board's actual header positions:
//
//   LEFT HEADER  (top→bottom)
//     GPIO  1, 2, 4, 5, 6, 7 → TTP223 capacitive touch (6 sensors)
//       ⚠  GPIO 3 is a strapping pin. TTP223 rests LOW when untouched,
//          which is the safe boot-time state. Do NOT touch the pad
//          connected to GPIO 3 while powering on or pressing RESET.
//
//   RIGHT HEADER (top→bottom from 3V3 pin)
//     GPIO  8 (SDA)  → I2C proximity sensor
//     GPIO  9 (SCL)  → I2C proximity sensor
//     GPIO 10, 11, 12, 13 → mono-jack mechanical switches
//       ✓  ESP32S3FH4R2 has in-package flash; FSPI (GPIO 9–14) is
//          NOT wired to any external chip. These pads are free GPIO.
//
//   GPIO 48 → NeoPixel RGB LED (onboard, shared with red power LED)
//
// 12 unique HID keycodes: F1–F12, no modifier byte needed.
// All inputs fire on activation EDGE only (debounced, 35 ms).
// LED fades to off 250 ms after last event.
//
// ── PROXIMITY SENSOR WIRING ─────────────────────────────────────────
//   VCC → 3V3   GND → GND   SDA → GPIO 8   SCL → GPIO 9
//
// ── PROXIMITY SENSOR LIBRARY ─────────────────────────────────────────
// 1. Install your sensor's library in Arduino Library Manager.
// 2. Uncomment the matching #include and object lines below.
// 3. Adjust PROX_I2C_ADDR and PROX_THRESHOLD to suit your sensor.
//
// Tested sensor families:
//   VCNL4040  — Adafruit_VCNL4040   addr 0x60  lib "Adafruit VCNL4040"
//   VL53L0X   — Adafruit_VL53L0X    addr 0x29  lib "Adafruit VL53L0X"
//   APDS-9930/9960 — SparkFun_APDS9960 addr 0x39  lib "SparkFun APDS9960"
// =====================================================================

// --- SELECT PROXIMITY SENSOR (Uncomment ONLY ONE) ---
#define USE_APDS9930  // <-- Enable APDS9930
// #define USE_VCNL4040
// #define USE_VL53L0X
// #define USE_APDS9960

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <HIDKeyboardTypes.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>

// --- Proximity Sensor Library ---
#if defined(USE_APDS9930) || defined(USE_APDS9960)
  #include <SparkFun_APDS9960.h>
  SparkFun_APDS9960 proxSensor;
#elif defined(USE_VCNL4040)
  #include <Adafruit_VCNL4040.h>
  Adafruit_VCNL4040 proxSensor;
#elif defined(USE_VL53L0X)
  #include <Adafruit_VL53L0X.h>
  Adafruit_VL53L0X proxSensor;
#endif

#include "types_expanded.h"

// =====================================================================
// HARDWARE — PIN MAP
// =====================================================================

// ── TTP223 capacitive touch (INPUT, active HIGH, no pull needed) ──────
static const uint8_t TOUCH_PINS[] = { 1, 2, 4, 5, 6, 7 };  // Removed GPIO 3 (strapping pin)
static constexpr uint8_t NUM_TOUCH = 6;  // Updated to 6 sensors

// ── I2C proximity sensor ──────────────────────────────────────────────
#define I2C_SDA        8
#define I2C_SCL        9
#if defined(USE_APDS9930) || defined(USE_APDS9960)
  #define PROX_I2C_ADDR  0x39  // APDS9930/9960 default
  #define PROX_THRESHOLD 200   // 0-255 (8-bit)
#elif defined(USE_VCNL4040)
  #define PROX_I2C_ADDR  0x60
  #define PROX_THRESHOLD 500
#elif defined(USE_VL53L0X)
  #define PROX_I2C_ADDR  0x29
  #define PROX_THRESHOLD 200
#else
  #define PROX_I2C_ADDR  0x39
  #define PROX_THRESHOLD 200
#endif

// ── Mono-jack switches (INPUT_PULLUP, active LOW) ─────────────────────
static const uint8_t SWITCH_PINS[] = { 10, 11, 12, 13 };
static constexpr uint8_t NUM_SWITCH = 4;

// ── NeoPixel ─────────────────────────────────────────────────────────
#define LED_PIN    48
#define NUM_PIXELS 1

// =====================================================================
// HID KEYCODE LOOKUP TABLES  (F1–F12, all unique, no modifier needed)
// =====================================================================
//  id  GPIO  Source   Keycode  Key   LED Color
//   0     1  TOUCH    0x3A     F1    RED
//   1     2  TOUCH    0x3B     F2    RED
//   2     4  TOUCH    0x3C     F3    RED
//   3     5  TOUCH    0x3D     F4    RED
//   4     6  TOUCH    0x3E     F5    RED
//   5     7  TOUCH    0x3F     F6    RED
//   —     —  PROX     0x41     F8    GREEN
//   0    10  SWITCH   0x42     F9    BLUE
//   1    11  SWITCH   0x43     F10   BLUE
//   2    12  SWITCH   0x44     F11   BLUE
//   3    13  SWITCH   0x45     F12   BLUE

static const uint8_t TOUCH_KEYS[]    = { 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F };
static const uint8_t SWITCH_KEYS[]   = { 0x42, 0x43, 0x44, 0x45 };
static const Color   TOUCH_COLORS[]  = { RED, RED, RED, RED, RED, RED };
static const Color   SWITCH_COLORS[] = { BLUE, BLUE, BLUE, BLUE };

// =====================================================================
// GLOBALS
// =====================================================================
Adafruit_NeoPixel pixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

Color   ledCurrent = {0, 0, 0};
Color   ledTarget  = {0, 0, 0};
UIState ui;

const uint32_t DEBOUNCE_MS = 35;

DebouncedBtn t[NUM_TOUCH];
DebouncedBtn s[NUM_SWITCH];
DebouncedBtn prox;

NimBLEHIDDevice*      hid;
NimBLECharacteristic* input;
bool connected = false;

// =====================================================================
// BLE SERVER CALLBACKS
// =====================================================================
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    connected = true;
    Serial.println("=== CONNECTED ===");
  }
  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    connected = false;
    Serial.println("=== DISCONNECTED, restarting advertising ===");
    NimBLEDevice::startAdvertising();
  }
};

// =====================================================================
// LED FADE ENGINE
// =====================================================================
void fadeStep() {
  auto step = [](uint8_t &c, uint8_t tgt) {
    if (c < tgt) c++;
    else if (c > tgt) c--;
  };
  step(ledCurrent.r, ledTarget.r);
  step(ledCurrent.g, ledTarget.g);
  step(ledCurrent.b, ledTarget.b);
  pixel.setPixelColor(0, pixel.Color(ledCurrent.r, ledCurrent.g, ledCurrent.b));
  pixel.show();
}

// =====================================================================
// DEBOUNCE — returns true on stable-state change only
// =====================================================================
bool updateBtn(DebouncedBtn &b, bool raw) {
  uint32_t now = millis();
  if (raw != b.last) { b.last = raw; b.t = now; }
  if ((now - b.t) > DEBOUNCE_MS && b.stable != raw) {
    b.stable = raw;
    return true;
  }
  return false;
}

// =====================================================================
// HID KEY SEND  (press + release)
// =====================================================================
void sendKey(uint8_t modifier, uint8_t keycode) {
  if (!connected) { Serial.println("  (not connected, skipping)"); return; }

  uint8_t press[9]   = { 0x01, modifier, 0x00, keycode, 0x00, 0x00, 0x00, 0x00, 0x00 };
  uint8_t release[9] = { 0x01, 0x00,     0x00, 0x00,    0x00, 0x00, 0x00, 0x00, 0x00 };

  input->setValue(press, sizeof(press));
  input->notify();
  delay(30);
  input->setValue(release, sizeof(release));
  input->notify();
  delay(20);
}

// =====================================================================
// PROXIMITY RAW READ
// =====================================================================
uint16_t readProximityRaw() {
  #if defined(USE_APDS9930) || defined(USE_APDS9960)
    uint8_t val;
    if (proxSensor.readProximity(val)) {
      return val;
    }
    return 0;

  #elif defined(USE_VCNL4040)
    return proxSensor.getProximity();

  #elif defined(USE_VL53L0X)
    VL53L0X_RangingMeasurementData_t m;
    proxSensor.rangingTest(&m, false);
    return (m.RangeStatus != 4) ? m.RangeMilliMeter : 0;

  #else
    Wire.beginTransmission(PROX_I2C_ADDR);
    Wire.write(0x09);
    byte error = Wire.endTransmission(false);
    if (error != 0) {
      Serial.printf("  I2C Error: %d (addr=0x%02X)\n", error, PROX_I2C_ADDR);
      return 0;
    }
    if (Wire.requestFrom((uint8_t)PROX_I2C_ADDR, (uint8_t)1) != 1) {
      Serial.println("  I2C Request failed");
      return 0;
    }
    return Wire.read();
  #endif
}

// =====================================================================
// EVENT HANDLERS
// =====================================================================
void handleTouch(uint8_t id) {
  ui.lastEvent   = millis();
  ui.targetColor = TOUCH_COLORS[id];
  Serial.printf("TOUCH  %d  GPIO%-2d -> F%d (0x%02X)\n",
                id, TOUCH_PINS[id], id + 1, TOUCH_KEYS[id]);
  sendKey(0x00, TOUCH_KEYS[id]);
}

void handleSwitch(uint8_t id) {
  ui.lastEvent   = millis();
  ui.targetColor = SWITCH_COLORS[id];
  Serial.printf("SWITCH %d  GPIO%-2d -> F%d (0x%02X)\n",
                id, SWITCH_PINS[id], id + 9, SWITCH_KEYS[id]);
  sendKey(0x00, SWITCH_KEYS[id]);
}

void handleProximity() {
  if (!prox.stable) return;
  ui.lastEvent   = millis();
  ui.targetColor = WHITE;
  Serial.println("PROX   near   -> F8 (0x41)");
  sendKey(0x00, 0x41);
}

// =====================================================================
// BLE SETUP
// =====================================================================
void setupBLE() {
  Serial.println("=== BLE INIT START ===");

  NimBLEDevice::init("BT Switch");
  NimBLEDevice::setMTU(247);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setSecurityAuth(true, false, false);

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  hid = new NimBLEHIDDevice(server);
  hid->setManufacturer("ESP32 Switch");
  hid->setPnp(0x01, 0xFFFF, 0x0001, 0x0100);

  input = hid->getInputReport(1);

  static const uint8_t reportMap[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    // Modifier keys (8 bits)
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0xE0,        //   Usage Minimum (Left Ctrl)
    0x29, 0xE7,        //   Usage Maximum (Right GUI)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1 bit)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)
    // Reserved byte
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8 bits)
    0x81, 0x03,        //   Input (Constant)
    // Key array (6 key slots)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8 bits)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x65,        //   Logical Maximum (101)
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0x00,        //   Usage Minimum (0)
    0x29, 0x65,        //   Usage Maximum (101)
    0x81, 0x00,        //   Input (Data, Array)
    // LED output (5 LEDs + 3 padding bits)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x91, 0x02,        //   Output (Data, Variable, Absolute)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x03,        //   Output (Constant)
    0xC0               // End Collection
  };

  hid->setReportMap((uint8_t*)reportMap, sizeof(reportMap));
  hid->setBatteryLevel(100);

  // --- Battery Service (Required for Android/Nokia) ---
  NimBLEService* battSvc = server->createService(BLEUUID((uint16_t)0x180F));
  NimBLECharacteristic* battChr = battSvc->createCharacteristic(
      BLEUUID((uint16_t)0x2A19),
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  uint8_t battLevel = 100;
  battChr->setValue(&battLevel, 1);

  // --- Advertising ---
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  NimBLEAdvertisementData advData;
  advData.setFlags(0x06);  // General Discovery + LE-only
  advData.setName("BT Switch");
  advData.addServiceUUID(NimBLEUUID("1812"));  // Standard HID UUID
  advData.addServiceUUID(hid->getHidService()->getUUID());
  advData.addServiceUUID(battSvc->getUUID());
  adv->setAdvertisementData(advData);
  adv->setMinInterval(32);  // 20ms (0x20 * 0.625ms)
  adv->setMaxInterval(48);  // 30ms (0x30 * 0.625ms)
  adv->start();

  Serial.println("=== BLE READY, ADVERTISING ===");
}

// =====================================================================
// SETUP
// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("=== SETUP START ===");

  // ── Touch sensors (TTP223 drives output directly, no pull needed) ──
  for (int i = 0; i < NUM_TOUCH; i++) {
    pinMode(TOUCH_PINS[i], INPUT);
    Serial.printf("  Touch  GPIO%2d -> INPUT\n", TOUCH_PINS[i]);
  }

  // ── I2C proximity sensor ──────────────────────────────────────────────
  Wire.begin(I2C_SDA, I2C_SCL);

  #if defined(USE_APDS9930) || defined(USE_APDS9960)
    if (!proxSensor.init()) {
      Serial.println("  ERROR: APDS9930/9960 init failed — check wiring!");
    } else {
      proxSensor.enableProximitySensor(true);  // Enable proximity (no gesture)
      Serial.println("  Prox   APDS9930/9960 initialized (addr=0x39)");
    }
  #elif defined(USE_VCNL4040)
    if (!proxSensor.begin()) {
      Serial.println("  ERROR: VCNL4040 not found — check wiring!");
    } else {
      Serial.println("  Prox   VCNL4040 initialized (addr=0x60)");
    }
  #elif defined(USE_VL53L0X)
    if (!proxSensor.begin()) {
      Serial.println("  ERROR: VL53L0X not found — check wiring!");
    } else {
      Serial.println("  Prox   VL53L0X initialized (addr=0x29)");
    }
  #else
    Serial.printf("  Prox   Generic I2C (addr=0x%02X, threshold=%d)\n",
                  PROX_I2C_ADDR, PROX_THRESHOLD);
  #endif

  // ── Mechanical switches (INPUT_PULLUP; switch shorts pin to GND) ───
  for (int i = 0; i < NUM_SWITCH; i++) {
    pinMode(SWITCH_PINS[i], INPUT_PULLUP);
    Serial.printf("  Switch GPIO%2d -> INPUT_PULLUP\n", SWITCH_PINS[i]);
  }

  // ── NeoPixel ──────────────────────────────────────────────────────
  pixel.begin();
  pixel.setBrightness(80);
  pixel.clear();
  pixel.show();
  Serial.println("=== NEOPIXEL OK ===");

  ui.targetColor = COLOR_OFF;  // Use COLOR_OFF instead of OFF
  setupBLE();
  Serial.println("=== SETUP COMPLETE ===");
}

// =====================================================================
// MAIN LOOP
// =====================================================================
void loop() {
  // ── Touch sensors (active HIGH, read directly) ────────────────────
  for (int i = 0; i < NUM_TOUCH; i++) {
    bool raw = (bool)digitalRead(TOUCH_PINS[i]);
    if (updateBtn(t[i], raw) && t[i].stable) {
      handleTouch(i);
    }
  }

  // ── Proximity sensor (threshold converts to debounced bool) ───────
  uint16_t proxRaw = readProximityRaw();
  bool proxNear = (proxRaw > PROX_THRESHOLD);
  if (updateBtn(prox, proxNear) && prox.stable) {
    handleProximity();
  }

  // ── Mechanical switches (active LOW — invert the digital read) ────
  for (int i = 0; i < NUM_SWITCH; i++) {
    bool raw = !digitalRead(SWITCH_PINS[i]);
    if (updateBtn(s[i], raw) && s[i].stable) {
      handleSwitch(i);
    }
  }

  // ── LED timeout — fade to off 250 ms after last event ─────────────
  if (millis() - ui.lastEvent > 250) {
    ui.targetColor = COLOR_OFF;  // Use COLOR_OFF instead of OFF
  }
  ledTarget = ui.targetColor;
  fadeStep();
  delay(3);
}