---
layout: default
title: BLE 3-Switch Arrow/Enter Node
---

[← Back to project overview](index.html)

# BLE 3-Switch Arrow/Enter Node

**ESP32-S3 SuperMini · BLE HID Keyboard · 3-Switch Directional Controller**

A stripped-down variant of the BLE Adaptive Switch Node scaffold. This build has **no touch sensors and no proximity sensor** — only 3 mechanical switches mounted on top of an enclosure (left / middle / right), each sending a unique HID keystroke: `LEFT_ARROW`, `ENTER`, and `RIGHT_ARROW`.

> 📄 Source: [`BLE_3_SWITCH_INTERFACE/`](https://github.com/mrhunsaker/Assistive_Switch_Interfaces/tree/main/BLE_3_SWITCH_INTERFACE) in the repository

---

## Pin / Key / Position Map

| Position | GPIO | pinMode | HID Key | LED Color |
|----------|------|---------------|-------------|-----------|
| Left (lowest GPIO) | 4 | `INPUT_PULLUP` | LEFT_ARROW (`0x50`) | Blue |
| Middle | 5 | `INPUT_PULLUP` | ENTER (`0x28`) | Green |
| Right (highest GPIO) | 6 | `INPUT_PULLUP` | RIGHT_ARROW (`0x4F`) | Red |

The lowest-numbered GPIO is the physically left switch and sends `LEFT_ARROW`; the middle GPIO is the physically middle switch and sends `ENTER`; the highest-numbered GPIO is the physically right switch and sends `RIGHT_ARROW`.

GPIO 4, 5, 6 were chosen because they're free general-purpose pins on the ESP32-S3 SuperMini, well clear of GPIO 0 (BOOT), GPIO 3 (JTAG strapping — avoided per the parent project's policy even though it isn't adjacent here), GPIO 19/20 (USB D-/D+), GPIO 45/46 (strapping), and GPIO 48 (NeoPixel). If your enclosure wiring makes a different trio of pins more convenient, you can freely renumber `SWITCH_PINS[]` in the `.ino` — just keep the array in ascending GPIO order so index 0/1/2 stay left/middle/right.

---

## Wiring

Each switch connects between a GPIO pin and GND. `INPUT_PULLUP` is used internally, so no external resistor is needed — a simple normally-open momentary switch is all that's required, identical to the mechanical-switch wiring in the parent scaffold.

---

## No Crosstalk — How It's Guaranteed

- Each switch has its **own** `DebouncedBtn` struct instance (its own `stable`, `last`, and `t` fields) stored in its own array slot.
- Each loop iteration reads, debounces, and dispatches each switch **independently** — there is no shared/global debounce state anywhere in the sketch.
- Every keystroke is edge-triggered per switch: holding one switch down does not repeat its key, and does nothing at all to the debounce timers of the other two switches.
- `handleSwitch(id)` only ever reads/writes the array element for that one `id`. Nothing in the send path is shared across switches except the BLE `input` characteristic itself (a physical necessity — all keys share one BLE HID connection), which does not carry any state between calls.

In short: three independent debounce timers, three independent edge-trigger checks, three independent `sendKey()` calls. One switch being pressed, held, or bouncing can never suppress, delay, or spuriously trigger another switch's keystroke.

---

## BLE Behavior

Identical to the parent scaffold:

- Advertises continuously as **`BT Switch 3`** until a host connects.
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

Since this build reuses the exact same report map, Battery Service, and reconnect logic that were already verified in the parent project, it should behave identically on all the same platforms — but re-verify on your specific target devices before deployment, since the input mapping (3 switches → arrows/Enter instead of 11 inputs → function keys) is new.

---

## LED Feedback

Same fade engine as the parent project: a color snaps to a *target* on activation and fades back to off if there's no activity for 250 ms.

| LED Color | Meaning |
|-----------|---------|
| Blue | Left switch pressed → LEFT_ARROW |
| Green | Middle switch pressed → ENTER |
| Red | Right switch pressed → RIGHT_ARROW |
| Off | Idle |

---

## Software Dependencies

- `NimBLE-Arduino`
- `Adafruit NeoPixel`

No touch-sensor or proximity-sensor libraries are required for this build — they've been removed entirely, along with the `#define USE_...` sensor-selection block and the `<HIDKeyboardTypes.h>` include (unused in either project, since all keycodes are sent as raw hex values).

> See the repository-wide [pinned library versions](index.html#required-library-versions-pinned) for the exact versions this project has been verified against.

---

## Build and Flash Instructions

1. Install ESP32 board support in Arduino IDE (`Tools → Board → Boards Manager`, search "esp32", install **ESP32 by Espressif Systems**, version **2.0.17** — see the [board package note](index.html#required-board-package-version)).
2. Select `Tools → Board → ESP32 Arduino → ESP32S3 Dev Module`.
3. Open `BLE_3_SWITCH_INTERFACE.ino`.
4. Ensure `types_3switch.h` is present in the same folder.
5. Install `NimBLE-Arduino` and `Adafruit NeoPixel` via `Sketch → Include Library → Manage Libraries`.
6. Connect the board with a data-capable USB-C cable.
7. Select the correct serial port under `Tools → Port`.
8. Click **Verify**, then **Upload**.
9. Open the Serial Monitor at **115200** baud and confirm `=== SETUP COMPLETE ===` prints.

## Quick Functional Test

1. Pair with `BT Switch 3` from your host device's Bluetooth settings.
2. Press the left switch — LED flashes blue, host receives a Left Arrow keystroke.
3. Press the middle switch — LED flashes green, host receives an Enter keystroke.
4. Press the right switch — LED flashes red, host receives a Right Arrow keystroke.
5. Hold any one switch down while pressing the other two in quick succession — each of the other two should still fire exactly once, confirming no crosstalk.

---

[← Back to project overview](index.html)
