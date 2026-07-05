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
  - Unique color per input

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

The device paired as a standard BLE keyboard and successfully generated switch events without custom middleware or application software.

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
| 1 | TP4056 Type-c USB 5V 1A 18650 Lithium Battery Charging Board <br >with Dual Protection (optional) |

---

# Hardware Architecture

## Touch Inputs

| Input | GPIO | HID Key |
|---------|------|---------|
| Touch 1 | GPIO 1 | F1 |
| Touch 2 | GPIO 2 | F2 |
| Touch 3 | GPIO 4 | F3 |
| Touch 4 | GPIO 5 | F4 |
| Touch 5 | GPIO 6 | F5 |
| Touch 6 | GPIO 7 | F6 |

## Proximity Input

| Interface | GPIO | HID Key |
|-----------|------|---------|
| I²C | SDA=8 SCL=9 | F8 |

## Mechanical Switches

| Input | GPIO | HID Key |
|---------|------|---------|
| Switch 1 | GPIO 10 | F9 |
| Switch 2 | GPIO 11 | F10 |
| Switch 3 | GPIO 12 | F11 |
| Switch 4 | GPIO 13 | F12 |

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

Although functional as a digital input, it is a reset-time strapping pin. Eliminating GPIO 3 from the design avoids deployment issues and simplifies support for educational and assistive technology environments.

---

# Wiring Guide

## TTP223 Sensors

| TTP223 Pin | Connection |
|------------|------------|
| VCC | 3V3 |
| GND | GND |
| SIG | GPIO 1,2,4,5,6,7 on SP32-S3 Supermini |

## I²C Proximity Sensor

| Sensor Pin (APSD-9930 or APDS-9960) | Connection |
|------------|------------|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 8 on ESP32-S3 Supermini |
| SCL | GPIO 9 on ESP32-S3 Supermini |

## Mechanical Adaptive Switches

Each switch connects between:

- GPIO input
- Ground

Inputs use INPUT_PULLUP internally. 

---

# HID Key Mapping

| Source | HID Key |
|----------|---------|
| Touch 1 (TTP223) | F1 |
| Touch 2 (TTP223) | F2 |
| Touch 3 (TTP223) | F3 |
| Touch 4 (TTP223) | F4 |
| Touch 5 (TTP223) | F5 |
| Touch 6 (TTP223) | F6 |
| Proximity (APDS-9930/9960) | F8 |
| Switch 1 (Mono Jack Input) | F9 |
| Switch 2 (Mono Jack Input) | F10 |
| Switch 3 (Mono Jack Input) | F11 |
| Switch 4 (Mono Jack Input) | F12 |

Note: F7 is intentionally unused.

---

# Bluetooth Operation

## Device Name

Default = `BT Switch`

## Behavior

- Advertises continuously until connected
- Appears as a standard BLE keyboard
- Automatically resumes advertising after disconnect
  - Does not advertise when connected. No interference if multiple units used near each other

- No proprietary protocol required

---

# Proximity Sensor Support

The firmware scaffold includes support for:

- APDS9930
- APDS9960
- VCNL4040
- VL53L0X

Sensor-specific initialization may require enabling the appropriate library and code section.

---

# Software Dependencies

Required:

- `NimBLE-Arduino`
- `Adafruit NeoPixel`
- `Wire`

Optional (sensor dependent):

- `Adafruit VCNL4040`
- `Adafruit VL53L0X`
- `SparkFun APDS9960`

---

# Build and Flash Instructions

1. Install ESP32 board support in `Arduino IDE`.
2. Select `ESP32S3 Dev Module`.
3. Open `BLE_ADAPTIVE_SWITCH_EXPANDED_SCAFFOLD.ino`.
4. Ensure `types_expanded.h` is present (should open in adjacebnt window automatically)
5. Install required libraries.
6. Connect the board using USB-C.
7. Select the correct serial port.
8. Upload the firmware.
9. Open Serial Monitor at 115200 baud.

---

# Android Configuration

1. Pair the device through Android Bluetooth settings.
2. Install `Switch Access` from the Google Play Store
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

# Troubleshooting

## Device Does Not Appear

- Confirm Bluetooth advertising started.
- Verify board power.
- Remove old pairings and re-pair (it may be connecting to another computer or nearby device)

## Switch Activates Repeatedly

- Increase debounce interval.
- Check switch wiring.

## Proximity Sensor Not Detected

- Verify SDA/SCL wiring.
- Confirm correct sensor library.
- Check I²C address.

## Upload Problems

- Verify USB cable supports data.
- Select correct COM port.
- Hold BOOT if necessary during upload.

---

## CHANGE LOG

