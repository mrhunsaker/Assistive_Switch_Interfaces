#pragma once

// =====================================================================
// GPIO / KEYCODE / LED COLOR REFERENCE
// Board: ESP32-S3 SuperMini  (chip: ESP32S3FH4R2)
//        4 MB in-package flash, 2 MB in-package PSRAM
//
// This is the touch-sensor parallel of the BLE 3-Switch Interface.
// There are NO mechanical switches in this build — only 3 TTP223
// capacitive touch sensors, physically mounted left / middle / right
// on top of an enclosure.
//
// GPIO  Array   Physical Position   pinMode   Key          LED Color
// ----  ------  ------------------  --------  -----------  ---------
//    1  t[0]    LEFT  (lowest GPIO) INPUT     LEFT_ARROW   BLUE
//    2  t[1]    MIDDLE              INPUT     ENTER        GREEN
//    4  t[2]    RIGHT (highest GPIO)INPUT     RIGHT_ARROW  RED
//
// Mapping rule (per request, identical to the 3-switch interface): the
// lowest-numbered GPIO is wired to the physically LEFT sensor and sends
// LEFT_ARROW; the middle GPIO is wired to the physically MIDDLE sensor
// and sends ENTER; the highest-numbered GPIO is wired to the physically
// RIGHT sensor and sends RIGHT_ARROW.
//
// TTP223 modules drive their SIG output HIGH on touch, so these pins
// use plain INPUT (no pull-up) and are read directly, with NO inversion
// — identical to how touch inputs are handled in the parent 11-input
// scaffold.
//
// GPIO 3 is intentionally avoided (reset-time strapping pin), consistent
// with the parent project's GPIO 3 policy, even though it sits between
// two of the pins used here.
//
// PINS TRULY OFF-LIMITS on this board:
//   GPIO 0  — BOOT button / boot-mode strapping (HIGH = SPI boot)
//   GPIO 3  — JTAG source strapping pin (avoided by policy)
//   GPIO 19 — USB D-
//   GPIO 20 — USB D+
//   GPIO 45 — VDD_SPI strapping (flash voltage)
//   GPIO 46 — Boot mode strapping (input-only)
//   GPIO 48 — NeoPixel RGB LED (in use)
//
// =====================================================================
// COLOR SYSTEM — one color per sensor (only 3 inputs total here, so
// per-sensor color is unambiguous and still easy to memorize)
// =====================================================================
//   BLUE  = LEFT sensor    -> LEFT_ARROW
//   GREEN = MIDDLE sensor  -> ENTER
//   RED   = RIGHT sensor   -> RIGHT_ARROW
//   OFF   = idle / no recent activation
// =====================================================================
struct Color { uint8_t r, g, b; };

static Color COLOR_OFF = {0,   0,   0  };
static Color RED       = {255, 0,   0  };
static Color GREEN     = {0,   255, 0  };
static Color BLUE      = {0,   0,   255};

// =====================================================================
// DEBOUNCED BUTTON
// Each touch sensor gets its own independent instance of this struct.
// Because every sensor's stable/last/t fields are private to its own
// array element, no sensor's timing or state can influence another
// sensor's debounce/edge-trigger decision — this is what guarantees no
// crosstalk.
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
