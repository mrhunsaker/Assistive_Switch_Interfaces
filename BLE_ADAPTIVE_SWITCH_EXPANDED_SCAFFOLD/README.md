---
title: Bluetooth Adaptive Switch Node
category: Accessibility
author: Michael Ryan Hunsaker, M.Ed., Ph.D.
tags: 
  - accessibility
  - switch-access
  - esp32
  - arduino
  - iOS
  - iPadOS
  - Android
footer: ${pageNo} / ${pageCount}
header: ${title} / ${author}
---

# BLE Adaptive Switch Node

**ESP32-S3 SuperMini · BLE HID Keyboard · Adaptive Switch Controller for Assistive Technology**

A Bluetooth Low Energy (BLE) HID keyboard designed for adaptive access applications. The device presents 11 independent inputs as unique HID keyboard events, enabling accessibility frameworks such as iPadOS/iOS Switch Control and Android Switch Access to identify and map each switch independently.

The firmware is optimized for assistive technology deployments where reliability, low latency, and simple configuration are more important than high input density. GPIO 3 is intentionally excluded from the design to avoid any reset-time strapping behavior.

---

[TOC]

---

# What This Firmware Actually Does

This section describes the real, current behavior of `BLE_ADAPTIVE_SWITCH_EXPANDED_SCAFFOLD.ino`, so anyone extending the project knows exactly what they're building on.

1. **On boot**, the firmware configures 6 touch pins as `INPUT`, 4 switch pins as `INPUT_PULLUP`, starts I²C for the proximity sensor, initializes the NeoPixel, and starts a NimBLE BLE HID keyboard server advertising as `BT Switch`.
2. **Every loop iteration** (roughly every 3 ms):
   - Each of the 6 touch pins is read directly (`digitalRead`). TTP223 modules drive their output HIGH on touch, so no inversion is needed.
   - The proximity sensor is polled for a raw reading, which is compared against `PROX_THRESHOLD` to produce a simple near/far boolean.
   - Each of the 4 switch pins is read and **inverted**, because they use `INPUT_PULLUP` — a switch closure pulls the pin LOW, and the firmware treats LOW as "pressed."
   - Every one of those 11 raw boolean readings passes through a shared debounce function (`updateBtn`) that only reports a change once the signal has been stable for 35 ms. This is what prevents a noisy or bouncy sensor from sending dozens of keystrokes per touch.
   - A debounced **transition into the active state** (not the active state itself) triggers one HID keystroke: a key-down report immediately followed by a key-up report 30–50 ms later. This is what makes the device "edge-triggered" — holding a switch down does not repeat the keystroke.
3. **Sending a key** is handled by `sendKey()`, which writes directly to the BLE HID input report characteristic and notifies the connected host. If nothing is connected, the function logs that it skipped the send and does nothing else — no queuing, no buffering.
4. **LED feedback** is type-based, not sensor-based: any touch sets the LED's *target* color to green, any switch sets it to blue, and the proximity sensor sets it to red. A separate `fadeStep()` call each loop nudges the LED's current RGB value one step closer to the target, producing a smooth fade rather than a hard snap. 250 ms after the last event, the target resets to off, and the LED fades back to black.
5. **BLE reconnection** is automatic: a `NimBLEServerCallbacks` subclass restarts advertising any time the host disconnects, so the device doesn't need a manual re-pair after every disconnect (only after the host "forgets" it).
6. A standard **Battery Service** (UUID `0x180F`) is created and pinned at 100%. This isn't cosmetic — some Android devices (including the one this firmware was verified against) will not finish pairing with a BLE HID keyboard that omits it.

In short: 11 physical inputs → 1 shared debounce/edge-trigger pipeline → 1 of 12 possible HID keycodes (F1–F6, F8–F12) → BLE notify → LED color tells you which *category* of input just fired.

---

# Features

- 11 independent adaptive inputs
  - 6 capacitive touch sensors
  - 1 proximity sensor
  - 4 mechanical switch inputs

- 11 unique HID key assignments
  - F1–F6
  - F8–F12

- BLE HID keyboard implementation
  - No companion application required
  - Standard Bluetooth keyboard profile

- Verified accessibility support
  - iPadOS Switch Control
  - Android Switch Access

- NimBLE Bluetooth stack
  - Reduced memory footprint
  - Fast reconnection
  - Automatic advertising restart

- Visual status indication
  - WS2812 NeoPixel on GPIO 48
  - One color per **input type** (not per individual sensor) — see LED Color Reference below

- 35 ms debounce protection

- Edge-triggered operation
  - Single event per activation
  - No auto-repeat

---

# Platform Compatibility

| Platform | Status |
|----------|--------|
| iPadOS Switch Control | Verified |
| iOS Switch Control | Verified |
| Android Switch Access | Verified |
| Windows | Verified |
| macOS | Verified |
| Linux | Verified |
| ChromeOS | Anticipated |

## Verified Android Device

The current firmware has been successfully tested on:

- Nokia G100
- Android 12
- Android Switch Access

The device paired as a standard BLE keyboard and successfully generated switch events without custom middleware or application software. The Battery Service described above was required for this pairing to succeed.

---

# Hardware Requirements

| Qty | Component |
|------|-----------|
| 1 | ESP32-S3 SuperMini |
| Up to 6 | TTP223 capacitive touch sensors |
| 1 | Supported I²C proximity sensor |
| Up to 4 | Adaptive mechanical switches |
| 1 | USB-C cable |
| 1 | 3.7V 1100mAh Lithium polymer Battery (optional) |
| 1 | TP4056 Type-C USB 5V 1A 18650 Lithium Battery Charging Board with Dual Protection (optional) |

---

# Hardware Architecture

## Touch Inputs

| Input | GPIO | HID Key | LED Color |
|---------|------|---------|-----------|
| Touch 1 | GPIO 1 | F1 | Green |
| Touch 2 | GPIO 2 | F2 | Green |
| Touch 3 | GPIO 4 | F3 | Green |
| Touch 4 | GPIO 5 | F4 | Green |
| Touch 5 | GPIO 6 | F5 | Green |
| Touch 6 | GPIO 7 | F6 | Green |

## Proximity Input

| Interface | GPIO | HID Key | LED Color |
|-----------|------|---------|-----------|
| I²C | SDA=8 SCL=9 | F8 | Red |

## Mechanical Switches

| Input | GPIO | HID Key | LED Color |
|---------|------|---------|-----------|
| Switch 1 | GPIO 10 | F9 | Blue |
| Switch 2 | GPIO 11 | F10 | Blue |
| Switch 3 | GPIO 12 | F11 | Blue |
| Switch 4 | GPIO 13 | F12 | Blue |

---

# LED Color Reference

The NeoPixel uses a deliberately simple **3-color scheme based on input type**, not one unique color per sensor. This is the easiest scheme for classroom staff to memorize and is the current, correct behavior of the firmware:

| LED Color | Meaning |
|-----------|---------|
| Green | A touch sensor was activated (any of F1–F6) |
| Red | The proximity sensor was activated (F8) |
| Blue | A mechanical switch / mono-jack input was activated (any of F9–F12) |
| Off | Idle — no input in the last 250 ms, or not yet powered |

If you need to tell *which specific* touch sensor or switch fired, use the Serial Monitor at 115200 baud — every activation is also logged there with its GPIO number and assigned key.

---

# GPIO Reference

## Reserved / Avoided Pins

| GPIO | Reason |
|--------|--------|
| 0 | BOOT strapping pin |
| 19 | USB D− |
| 20 | USB D+ |
| 45 | VDD_SPI strapping |
| 46 | Input-only boot strapping |
| 48 | NeoPixel LED in use |

## GPIO 3 Policy

GPIO 3 is intentionally not used.

Although functional as a digital input, it is a reset-time strapping pin. Eliminating GPIO 3 from the design avoids deployment issues and simplifies support for educational and assistive technology environments. There is no "Touch 7" / F7 input as a result — F7 is intentionally unused.

---

# Wiring Guide

## TTP223 Sensors

| TTP223 Pin | Connection |
|------------|------------|
| VCC | 3V3 |
| GND | GND |
| SIG | GPIO 1, 2, 4, 5, 6, or 7 on the ESP32-S3 SuperMini (one sensor per pin) |

## I²C Proximity Sensor

| Sensor Pin (APDS-9930 or APDS-9960) | Connection |
|------------|------------|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 8 on ESP32-S3 SuperMini |
| SCL | GPIO 9 on ESP32-S3 SuperMini |

## Mechanical Adaptive Switches

Each switch connects between:

- GPIO input
- Ground

Inputs use `INPUT_PULLUP` internally, so no external resistor is required — a simple normally-open switch wired to ground is all that's needed.

---

# HID Key Mapping

| Source | HID Key | LED Color |
|----------|---------|-----------|
| Touch 1 (TTP223) | F1 | Green |
| Touch 2 (TTP223) | F2 | Green |
| Touch 3 (TTP223) | F3 | Green |
| Touch 4 (TTP223) | F4 | Green |
| Touch 5 (TTP223) | F5 | Green |
| Touch 6 (TTP223) | F6 | Green |
| Proximity (APDS-9930/9960/VCNL4040/VL53L0X) | F8 | Red |
| Switch 1 (Mono Jack Input) | F9 | Blue |
| Switch 2 (Mono Jack Input) | F10 | Blue |
| Switch 3 (Mono Jack Input) | F11 | Blue |
| Switch 4 (Mono Jack Input) | F12 | Blue |

Note: F7 is intentionally unused (it was historically reserved for a 7th touch sensor on GPIO 3, which this design deliberately omits).

---

# Bluetooth Operation

## Device Name

Default = `BT Switch`

To change it, edit this line in the `.ino` file before flashing:

```cpp
NimBLEDevice::init("BT Switch");
```

## Behavior

- Advertises continuously until connected
- Appears as a standard BLE keyboard
- Automatically resumes advertising after disconnect
  - Does not advertise when connected. No interference if multiple units used near each other

- No proprietary protocol required
- Includes a standard Battery Service (UUID `0x180F`) reporting 100% — required for reliable pairing with some Android devices

---

# Proximity Sensor Support

The firmware scaffold includes support for:

- APDS9930 *(enabled by default in this build — see `#define USE_APDS9930` near the top of the `.ino` file)*
- APDS9960
- VCNL4040
- VL53L0X

Only **one** sensor family can be active per build. Open the `.ino` file and make sure exactly one of the following lines is uncommented, with the rest commented out:

```cpp
#define USE_APDS9930  // <-- Enable APDS9930
// #define USE_VCNL4040
// #define USE_VL53L0X
// #define USE_APDS9960
```

Sensor-specific initialization may require enabling the appropriate library and code section, and you should review `PROX_I2C_ADDR` / `PROX_THRESHOLD` for your specific sensor — the default threshold (200 out of 0–255) is tuned for the APDS-9930/9960.

---

# Software Dependencies

Required:

- `NimBLE-Arduino`
- `Adafruit NeoPixel`
- `Wire` (bundled with the ESP32 Arduino core)

Optional (sensor dependent — install only the one matching your `#define`):

- `SparkFun APDS9960` (covers both APDS-9930 and APDS-9960)
- `Adafruit VCNL4040`
- `Adafruit VL53L0X`

> **Known build quirk:** the sketch includes `<HIDKeyboardTypes.h>`, but no named constant from that header is actually referenced anywhere in the code (all keycodes are sent as raw hex values, e.g. `0x3A`). If your Arduino IDE reports `HIDKeyboardTypes.h: No such file or directory`, the simplest fix is to delete that one `#include` line — nothing else in the sketch depends on it.

---

# Build and Flash Instructions

1. Install ESP32 board support in `Arduino IDE` (`Tools → Board → Boards Manager`, search "esp32", install **ESP32 by Espressif Systems**).
2. Select `Tools → Board → ESP32 Arduino → ESP32S3 Dev Module`.
3. Open `BLE_ADAPTIVE_SWITCH_EXPANDED_SCAFFOLD.ino`.
4. Ensure `types_expanded.h` is present in the same folder (it should open in an adjacent tab automatically).
5. Install the required libraries listed above via `Sketch → Include Library → Manage Libraries`.
6. Connect the board using a **data-capable** USB-C cable (many cheap USB-C cables are charge-only).
7. Select the correct serial port under `Tools → Port`.
8. Click **Verify** first to confirm it compiles, then click **Upload**.
9. Open `Tools → Serial Monitor` and set the baud rate to **115200**.
10. Power-cycle the board and confirm the setup messages print (`=== SETUP START ===` through `=== SETUP COMPLETE ===`).

---

# Android Configuration

1. Pair the device through Android Bluetooth settings.
2. Install `Switch Access` from the Google Play Store (if not already installed/enabled).
3. Enable `Switch Access`.
4. Add switches through Switch Access settings.
5. Activate each physical input to register its associated HID key.
6. Assign actions as desired.

---

# iPadOS / iOS Configuration

1. Open Settings.
2. Accessibility → Switch Control.
3. Add New Switch.
4. Select External.
5. Activate a switch on the device.
6. Assign an action.
7. Repeat as required.

---

# Quick Functional Test (Any Platform)

After flashing and pairing, confirm all three input *types* before deploying the device:

1. **Touch:** lightly touch any TTP223 pad. LED should flash **green**, and one F1–F6 keystroke should register on the host.
2. **Proximity:** bring a hand within a few centimeters of the proximity sensor. LED should flash **red**, and one F8 keystroke should register.
3. **Switch:** press a mechanical switch connected to any jack. LED should flash **blue**, and one F9–F12 keystroke should register.

If a category doesn't respond, check the Serial Monitor output for that input before assuming a hardware fault — it will print the GPIO and keycode for every successful activation, and an I²C error code if the proximity sensor isn't responding.

---

# Troubleshooting

## Device Does Not Appear

- Confirm Bluetooth advertising started (check Serial Monitor for `=== BLE READY, ADVERTISING ===`).
- Verify board power.
- Remove old pairings and re-pair (it may be connecting to another computer or nearby device).

## Switch Activates Repeatedly

- Increase the debounce interval (`DEBOUNCE_MS` in the `.ino` file, default 35 ms).
- Check switch wiring for an intermittent short.

## Proximity Sensor Not Detected

- Verify SDA/SCL wiring (GPIO 8 / GPIO 9).
- Confirm exactly one `#define USE_...` sensor line is active and it matches your physical sensor.
- Check the I²C address (`PROX_I2C_ADDR`) against your sensor's datasheet.
- Watch the Serial Monitor at startup for an explicit init failure message.

## Upload Problems

- Verify the USB cable supports data, not just charging.
- Select the correct COM/serial port.
- Hold BOOT (and briefly tap RESET) if the board doesn't enter programming mode automatically.

## `HIDKeyboardTypes.h` Not Found

- Delete the `#include <HIDKeyboardTypes.h>` line near the top of the sketch. No keycodes in this firmware are referenced by name from that header.

---

## CHANGE LOG

- Corrected this README to match the actual `EXPANDED_SCAFFOLD` firmware (11 inputs: 6 touch, 1 proximity, 4 switch) instead of an older 3+3 input revision.
- Simplified the LED feedback scheme to 3 colors by input type: **green** = touch, **red** = proximity, **blue** = switch.
- Documented the Battery Service requirement for Android pairing.
- Documented the `HIDKeyboardTypes.h` build quirk and its fix.
- Fixed wiring-guide typos (board name, sensor model number).
