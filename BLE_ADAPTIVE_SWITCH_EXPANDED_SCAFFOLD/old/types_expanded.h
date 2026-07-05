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
//    1  t[0]     SRC_TOUCH   INPUT          F1    RED
//    2  t[1]     SRC_TOUCH   INPUT          F2    ORANGE
//    3  t[2]     SRC_TOUCH   INPUT          F3    YELLOW   ← strapping pin*
//    4  t[3]     SRC_TOUCH   INPUT          F4    GREEN
//    5  t[4]     SRC_TOUCH   INPUT          F5    CYAN
//    6  t[5]     SRC_TOUCH   INPUT          F6    BLUE
//    7  t[6]     SRC_TOUCH   INPUT          F7    PURPLE
//   --  prox     SRC_PROX    I2C (SDA=8)    F8    WHITE
//   10  s[0]     SRC_SWITCH  INPUT_PULLUP   F9    PINK
//   11  s[1]     SRC_SWITCH  INPUT_PULLUP   F10   LIME
//   12  s[2]     SRC_SWITCH  INPUT_PULLUP   F11   TEAL
//   13  s[3]     SRC_SWITCH  INPUT_PULLUP   F12   MAGENTA
//
// * GPIO 3 strapping note:
//   Sampled at reset to select JTAG source (LOW = USB JTAG, HIGH = pins).
//   TTP223 rests LOW when untouched → safe default at boot.
//   Rule: ensure TTP223 on GPIO 3 is NOT pressed during power-on/reset.
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
// COLOR SYSTEM  (12 unique colors + OFF)
// =====================================================================
struct Color { uint8_t r, g, b; };

static Color COLOR_OFF     = {0,   0,   0  };
static Color RED     = {255, 0,   0  };
static Color ORANGE  = {255, 80,  0  };
static Color YELLOW  = {255, 180, 0  };
static Color GREEN   = {0,   255, 0  };
static Color CYAN    = {0,   200, 200};
static Color BLUE    = {0,   0,   255};
static Color PURPLE  = {180, 0,   255};
static Color WHITE   = {180, 180, 180}; // dimmed — avoids glare
static Color PINK    = {255, 50,  150};
static Color LIME    = {80,  255, 0  };
static Color TEAL    = {0,   180, 150};
static Color MAGENTA = {255, 0,   180};

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
