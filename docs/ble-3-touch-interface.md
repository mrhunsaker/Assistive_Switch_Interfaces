---
layout: default
title: BLE 3-Touch Arrow/Enter Node
---

[← Back to project overview](index.html)

# BLE 3-Touch Arrow/Enter Node

**ESP32-S3 SuperMini · BLE HID Keyboard · 3-Touch-Sensor Directional Controller**

The touch-sensor parallel of the BLE 3-Switch Interface. This build has **no mechanical switches** — only 3 TTP223 capacitive touch sensors mounted on top of an enclosure (left / middle / right), each sending a unique HID keystroke: `LEFT_ARROW`, `ENTER`, and `RIGHT_ARROW`.

> 📄 Source: [`BLE_3_TOUCH_INTERFACE/`](https://github.com/mrhunsaker/Assistive_Switch_Interfaces/tree/main/BLE_3_TOUCH_INTERFACE) in the repository

---

## Pin / Key / Position Map

| Position | GPIO | pinMode | HID Key | LED Color |
|----------|------|---------|-------------|-----------|
| Left (lowest GPIO) | 1 | `INPUT` | LEFT_ARROW (`0x50`) | Blue |
| Middle | 2 | `INPUT` | ENTER (`0x28`) | Green |
| Right (highest GPIO) | 4 | `INPUT` | RIGHT_ARROW (`0x4F`) | Red |

Same rule as the 3-switch interface: the lowest-numbered GPIO is the physically left sensor and sends `LEFT_ARROW`; the middle GPIO is the physically middle sensor and sends `ENTER`; the highest-numbered GPIO is the physically right sensor and sends `RIGHT_ARROW`.

GPIO 1, 2, 4 were chosen because they're the same touch-verified pins used for Touch 1–3 in the parent 11-input scaffold, and they're clear of GPIO 0 (BOOT), GPIO 3 (JTAG strapping — avoided per project policy even though it sits between two of these pins), GPIO 19/20 (USB D-/D+), GPIO 45/46 (strapping), and GPIO 48 (NeoPixel). If your enclosure wiring makes a different trio of pins more convenient, you can freely renumber `TOUCH_PINS[]` in the `.ino` — just keep the array in ascending GPIO order so index 0/1/2 stay left/middle/right.

---

## Wiring

Each TTP223 module connects:

| TTP223 Pin | Connection |
|------------|------------|
| VCC | 3V3 |
| GND | GND |
| SIG | GPIO 1, 2, or 4 (one sensor per pin) |

No pull-up resistor or `INPUT_PULLUP` is used — TTP223 modules drive their own SIG output HIGH on touch and LOW at rest, so a plain `INPUT` pin reads the signal directly with no inversion, identical to the touch-sensor wiring in the parent 11-input scaffold.

---

## No Crosstalk — How It's Guaranteed

- Each touch sensor has its **own** `DebouncedBtn` struct instance (its own `stable`, `last`, and `t` fields) stored in its own array slot.
- Each loop iteration reads, debounces, and dispatches each sensor **independently** — there is no shared/global debounce state anywhere in the sketch.
- Every keystroke is edge-triggered per sensor: holding one sensor touched does not repeat its key, and does nothing at all to the debounce timers of the other two sensors.
- `handleTouch(id)` only ever reads/writes the array element for that one `id`. Nothing in the send path is shared across sensors except the BLE `input` characteristic itself (a physical necessity — all keys share one BLE HID connection), which does not carry any state between calls.

In short: three independent debounce timers, three independent edge-trigger checks, three independent `sendKey()` calls. One sensor being touched, held, or bouncing can never suppress, delay, or spuriously trigger another sensor's keystroke.

---

## BLE Behavior

Identical to the parent scaffold and the 3-switch interface:

- Advertises continuously as **`BT Touch 3`** until a host connects.
- Appears as a standard BLE HID keyboard — no companion app required on any platform.
- Automatically resumes advertising after a disconnect (no manual re-pair needed unless the host "forgets" the device).
- Includes the standard Battery Service (UUID `0x180F`, pinned at 100%), which was required for reliable pairing on the Android device (Nokia G100 / Android 12) the parent project was verified against.
- Uses the NimBLE stack for a reduced memory footprint and fast reconnection.

## Platform Compatibility

| Platform | Status |
|----------|--------|
| iPadOS / iOS Switch Control | Expected (same HID report map as verified parent project) |
| Android Switch Access | Expected (same HID report map + Battery Service as verified parent project) |
| Windows / macOS / Linux | Expected (standard BLE keyboard profile) |

Since this build reuses the exact same report map, Battery Service, and reconnect logic that were already verified in the parent project, it should behave identically on all the same platforms — but re-verify on your specific target devices before deployment, since the input mapping (3 touch sensors → arrows/Enter instead of 11 inputs → function keys) is new.

---

## LED Feedback

Same fade engine as the parent project and the 3-switch interface: a color snaps to a *target* on activation and fades back to off if there's no activity for 250 ms.

| LED Color | Meaning |
|-----------|---------|
| Blue | Left sensor touched → LEFT_ARROW |
| Green | Middle sensor touched → ENTER |
| Red | Right sensor touched → RIGHT_ARROW |
| Off | Idle |

---

## Software Dependencies

- `NimBLE-Arduino`
- `Adafruit NeoPixel`

No proximity-sensor or mechanical-switch code is required for this build — they've been removed entirely, along with the `#define USE_...` sensor-selection block and the `<HIDKeyboardTypes.h>` include (unused across this project, since all keycodes are sent as raw hex values).

> See the repository-wide [pinned library versions](index.html#required-library-versions-pinned) for the exact versions this project has been verified against.

---

## Build and Flash Instructions

1. Install ESP32 board support in Arduino IDE (`Tools → Board → Boards Manager`, search "esp32", install **ESP32 by Espressif Systems**, version **2.0.17** — see the [board package note](index.html#required-board-package-version)).
2. Select `Tools → Board → ESP32 Arduino → ESP32S3 Dev Module`.
3. Open `BLE_3_TOUCH_INTERFACE.ino`.
4. Ensure `types_3touch.h` is present in the same folder.
5. Install `NimBLE-Arduino` and `Adafruit NeoPixel` via `Sketch → Include Library → Manage Libraries`.
6. Connect the board with a data-capable USB-C cable.
7. Select the correct serial port under `Tools → Port`.
8. Click **Verify**, then **Upload**.
9. Open the Serial Monitor at **115200** baud and confirm `=== SETUP COMPLETE ===` prints.

## Quick Functional Test

1. Pair with `BT Touch 3` from your host device's Bluetooth settings.
2. Touch the left sensor — LED flashes blue, host receives a Left Arrow keystroke.
3. Touch the middle sensor — LED flashes green, host receives an Enter keystroke.
4. Touch the right sensor — LED flashes red, host receives a Right Arrow keystroke.
5. Hold a finger on one sensor while touching the other two in quick succession — each of the other two should still fire exactly once, confirming no crosstalk.

---

[← Back to project overview](index.html)
