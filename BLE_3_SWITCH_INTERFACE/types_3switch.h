#pragma once

// =====================================================================
// GPIO / KEYCODE / LED COLOR REFERENCE
// Board: ESP32-S3 SuperMini  (chip: ESP32S3FH4R2)
//        4 MB in-package flash, 2 MB in-package PSRAM
//
// This is a simplified 3-input variant of the BLE Adaptive Switch Node.
// There are NO touch sensors and NO proximity sensor in this build —
// only 3 mechanical switches, physically mounted left / middle / right
// on top of an enclosure.
//
// GPIO  Array   Physical Position   pinMode        Key          LED Color
// ----  ------  ------------------  -------------  -----------  ---------
//    4  s[0]    LEFT  (lowest GPIO) INPUT_PULLUP   LEFT_ARROW   BLUE
//    5  s[1]    MIDDLE              INPUT_PULLUP   ENTER        GREEN
//    6  s[2]    RIGHT (highest GPIO)INPUT_PULLUP   RIGHT_ARROW  RED
//
// Mapping rule (per request): the lowest-numbered GPIO is wired to the
// physically LEFT switch and sends LEFT_ARROW; the middle GPIO is wired
// to the physically MIDDLE switch and sends ENTER; the highest-numbered
// GPIO is wired to the physically RIGHT switch and sends RIGHT_ARROW.
//
// GPIO 3 is intentionally avoided (reset-time strapping pin), consistent
// with the parent project's GPIO 3 policy, even though it is not adjacent
// to this pin selection.
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
// COLOR SYSTEM — one color per switch (only 3 inputs total here, so
// per-switch color is unambiguous and still easy to memorize)
// =====================================================================
//   BLUE  = LEFT switch    -> LEFT_ARROW
//   GREEN = MIDDLE switch  -> ENTER
//   RED   = RIGHT switch   -> RIGHT_ARROW
//   OFF   = idle / no recent activation
// =====================================================================
struct Color { uint8_t r, g, b; };

static Color COLOR_OFF = {0,   0,   0  };
static Color RED       = {255, 0,   0  };
static Color GREEN     = {0,   255, 0  };
static Color BLUE      = {0,   0,   255};

// =====================================================================
// DEBOUNCED BUTTON
// Each switch gets its own independent instance of this struct. Because
// every switch's stable/last/t fields are private to its own array
// element, no switch's timing or state can influence another switch's
// debounce/edge-trigger decision — this is what guarantees no crosstalk.
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
