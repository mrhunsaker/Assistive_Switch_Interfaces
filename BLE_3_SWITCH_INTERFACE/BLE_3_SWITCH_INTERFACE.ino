// =====================================================================
// BLE 3-SWITCH ARROW/ENTER NODE
// Board : ESP32-S3 SuperMini  (ESP32S3FH4R2 — in-package flash+PSRAM)
// Target: NimBLE HID keyboard over BLE → iPadOS/iOS Switch Control,
//         Android Switch Access, and any standard BLE keyboard host
//         (Windows / macOS / Linux).
//
// This is a stripped-down variant of the BLE Adaptive Switch Node
// scaffold. There are NO touch sensors and NO proximity sensor here —
// only 3 mechanical switches, mounted left / middle / right on top of
// an enclosure.
//
//   Lowest-numbered  GPIO -> physically LEFT switch   -> LEFT_ARROW
//   Middle-numbered  GPIO -> physically MIDDLE switch -> ENTER
//   Highest-numbered GPIO -> physically RIGHT switch  -> RIGHT_ARROW
//
// Each switch uses INPUT_PULLUP (switch closure pulls the pin LOW),
// exactly as in the parent scaffold's mechanical-switch handling.
//
// NO CROSSTALK: every switch is read, debounced, and edge-triggered
// completely independently. Each switch has its own DebouncedBtn
// struct instance (its own stable/last/t fields), its own raw digitalRead
// call, and its own handleSwitch() invocation. Nothing about one
// switch's state, timing, or HID key is shared with, derived from, or
// capable of blocking another switch's activation. A held switch does
// not repeat (edge-triggered) and does not stop the other two switches
// from firing on the very next loop iteration.
//
// 12-byte→9-byte HID report format, report map, Battery Service, and
// automatic-reconnect BLE server callbacks are carried over unchanged
// from the parent scaffold, since that combination was already verified
// against iPadOS, Android (Nokia G100 / Android 12 / Switch Access),
// Windows, macOS, and Linux.
// =====================================================================

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <Adafruit_NeoPixel.h>

// NOTE: unlike the parent scaffold, this build never references any
// named constant from HIDKeyboardTypes.h (all keycodes below are sent
// as raw hex values), so that #include is intentionally omitted here.
// If you copy code from the parent project, do NOT re-add that include
// unless you actually need a named constant from it.

#include "types_3switch.h"

// =====================================================================
// HARDWARE — PIN MAP
// =====================================================================
// Mono-jack / momentary switches (INPUT_PULLUP, active LOW).
// Index 0 = LEFT (lowest GPIO), 1 = MIDDLE, 2 = RIGHT (highest GPIO).
static const uint8_t  SWITCH_PINS[] = { 4, 5, 6 };
static constexpr uint8_t NUM_SWITCH = 3;

// ── NeoPixel ─────────────────────────────────────────────────────────
#define LED_PIN    48
#define NUM_PIXELS 1

// =====================================================================
// HID KEYCODE LOOKUP TABLE
// =====================================================================
//  id  GPIO  Position  Keycode  Key           LED Color
//   0     4  LEFT      0x50     LEFT_ARROW    BLUE
//   1     5  MIDDLE    0x28     ENTER         GREEN
//   2     6  RIGHT     0x4F     RIGHT_ARROW   RED
static const uint8_t SWITCH_KEYS[]   = { 0x50, 0x28, 0x4F };
static const Color   SWITCH_COLORS[] = { BLUE, GREEN, RED };
static const char*   SWITCH_NAMES[]  = { "LEFT_ARROW", "ENTER", "RIGHT_ARROW" };

// =====================================================================
// GLOBALS
// =====================================================================
Adafruit_NeoPixel pixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

Color   ledCurrent = {0, 0, 0};
Color   ledTarget  = {0, 0, 0};
UIState ui;

const uint32_t DEBOUNCE_MS = 35;

// One independent debounce instance per switch — see "NO CROSSTALK" note above.
DebouncedBtn s[NUM_SWITCH];

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
// DEBOUNCE — returns true on stable-state change only.
// Called once per switch, per loop, with that switch's OWN DebouncedBtn
// instance — no shared/global debounce state exists anywhere.
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
// EVENT HANDLER
// Fully self-contained per call: only touches array element `id`.
// =====================================================================
void handleSwitch(uint8_t id) {
  ui.lastEvent   = millis();
  ui.targetColor = SWITCH_COLORS[id];
  Serial.printf("SWITCH %d  GPIO%-2d -> %s (0x%02X)\n",
                id, SWITCH_PINS[id], SWITCH_NAMES[id], SWITCH_KEYS[id]);
  sendKey(0x00, SWITCH_KEYS[id]);
}

// =====================================================================
// BLE SETUP
// =====================================================================
void setupBLE() {
  Serial.println("=== BLE INIT START ===");

  NimBLEDevice::init("BT Switch 3");
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

  // --- Battery Service (Required for reliable pairing on some Android
  //     devices, verified in the parent project against a Nokia G100) ---
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
  advData.setName("BT Switch 3");
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

  // ── Mechanical switches (INPUT_PULLUP; switch shorts pin to GND) ───
  for (int i = 0; i < NUM_SWITCH; i++) {
    pinMode(SWITCH_PINS[i], INPUT_PULLUP);
    Serial.printf("  Switch GPIO%2d -> INPUT_PULLUP (%s)\n",
                  SWITCH_PINS[i], SWITCH_NAMES[i]);
  }

  // ── NeoPixel ──────────────────────────────────────────────────────
  pixel.begin();
  pixel.setBrightness(80);
  pixel.clear();
  pixel.show();
  Serial.println("=== NEOPIXEL OK ===");

  ui.targetColor = COLOR_OFF;
  setupBLE();
  Serial.println("=== SETUP COMPLETE ===");
}

// =====================================================================
// MAIN LOOP
// =====================================================================
void loop() {
  // ── Mechanical switches (active LOW — invert the digital read) ────
  // Each switch is read, debounced, and dispatched independently, in
  // its own loop iteration pass, using only its own array element.
  // No shared state exists between iterations of this loop, so one
  // switch's press/release timing can never suppress, delay, or
  // trigger another switch's event (no crosstalk).
  for (int i = 0; i < NUM_SWITCH; i++) {
    bool raw = !digitalRead(SWITCH_PINS[i]);
    if (updateBtn(s[i], raw) && s[i].stable) {
      handleSwitch(i);
    }
  }

  // ── LED timeout — fade to off 250 ms after last event ─────────────
  if (millis() - ui.lastEvent > 250) {
    ui.targetColor = COLOR_OFF;
  }
  ledTarget = ui.targetColor;
  fadeStep();
  delay(3);
}
