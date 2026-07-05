#pragma once

// =====================================================================
// GPIO / KEYCODE / LED COLOR REFERENCE
// Board: ESP32-S3 SuperMini  (chip: ESP32S3FH4R2)
//        4 MB in-package flash, 2 MB in-package PSRAM
//
// In-package flash/PSRAM → FSPI (GPIO 9–14) is NOT routed to any
// external chip on this board. GPIO 10–13 are free general-purpose I/O.
//
// Main header pin layout:
//   LEFT SIDE  (top→bottom): TX, RX, GPIO 1,2,3,4,5,6,7
//   RIGHT SIDE (top→bottom): 5V, GND, 3V3, GPIO 13,12,11,10,9,8
//
// GPIO  Array    Source      pinMode        Key   LED Color
// ----  -------  ----------  -------------  ----  ---------
//    1  t[0]     SRC_TOUCH   INPUT          F1    GREEN
//    2  t[1]     SRC_TOUCH   INPUT          F2    GREEN
//    4  t[2]     SRC_TOUCH   INPUT          F3    GREEN
//    5  t[3]     SRC_TOUCH   INPUT          F4    GREEN
//    6  t[4]     SRC_TOUCH   INPUT          F5    GREEN
//    7  t[5]     SRC_TOUCH   INPUT          F6    GREEN
//   --  prox     SRC_PROX    I2C (SDA=8)    F8    RED
//   10  s[0]     SRC_SWITCH  INPUT_PULLUP   F9    BLUE
//   11  s[1]     SRC_SWITCH  INPUT_PULLUP   F10   BLUE
//   12  s[2]     SRC_SWITCH  INPUT_PULLUP   F11   BLUE
//   13  s[3]     SRC_SWITCH  INPUT_PULLUP   F12   BLUE
//
// NOTE: This board uses a simplified 3-color feedback scheme by design.
// The LED tells you WHICH KIND of input fired, not which individual
// sensor fired. This is intentional and makes the device easier to
// teach and troubleshoot in a classroom setting:
//   GREEN = a touch sensor was activated     (F1-F6)
//   RED   = the proximity sensor was activated (F8)
//   BLUE  = a mechanical switch / mono-jack input was activated (F9-F12)
//   OFF   = idle / no recent activation
//
// GPIO 3 is not used on this board at all (see policy below), so there
// is no t[6]/F7 entry — F7 is intentionally unused.
//
// * GPIO 3 strapping note (for reference only — pin is unused here):
//   Sampled at reset to select JTAG source (LOW = USB JTAG, HIGH = pins).
//   TTP223 rests LOW when untouched → would be a safe default at boot,
//   but this design avoids GPIO 3 entirely to eliminate the risk.
//
// PINS TRULY OFF-LIMITS on this board:
//   GPIO 0  — BOOT button / boot-mode strapping (HIGH = SPI boot)
//   GPIO 19 — USB D-
//   GPIO 20 — USB D+
//   GPIO 26–32 — Primary SPI flash interface (internal)
//   GPIO 45 — VDD_SPI strapping (flash voltage)
//   GPIO 46 — Boot mode strapping (input-only)
//   GPIO 48 — NeoPixel RGB LED (in use)
//
// I2C default: SDA = GPIO 8,  SCL = GPIO 9
// =====================================================================

// =====================================================================
// COLOR SYSTEM  (simplified — 3 colors + OFF)
// =====================================================================
// This board intentionally uses only 3 indicator colors, one per INPUT
// TYPE (not one per individual sensor). This keeps the visual feedback
// simple enough for classroom staff to learn in a few seconds:
//   GREEN = touch sensor          BLUE = mechanical switch
//   RED   = proximity sensor      OFF  = idle
//
// If you later want per-sensor colors again, add new Color constants
// here and update TOUCH_COLORS / SWITCH_COLORS in the .ino file.
// =====================================================================
struct Color { uint8_t r, g, b; };

static Color COLOR_OFF = {0,   0,   0  };
static Color RED       = {255, 0,   0  };
static Color GREEN     = {0,   255, 0  };
static Color BLUE      = {0,   0,   255};

// =====================================================================
// INPUT SOURCE
// =====================================================================
enum InputSource { SRC_TOUCH, SRC_SWITCH, SRC_PROX };

// =====================================================================
// DEBOUNCED BUTTON
// =====================================================================
struct DebouncedBtn {
  bool     stable;
  bool     last;
  uint32_t t;
  DebouncedBtn() : stable(false), last(false), t(0) {}
};

// =====================================================================
// UI STATE
// =====================================================================
struct UIState {
  Color    targetColor;
  uint32_t lastEvent;
  UIState() : lastEvent(0) { targetColor = {0, 0, 0}; }
};
