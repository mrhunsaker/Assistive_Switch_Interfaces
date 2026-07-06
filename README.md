---
title: Assistive Switch Interfaces
category: Accessibility
author: Michael Ryan Hunsaker, M.Ed., Ph.D.
tags:
  - accessibility
  - switch-access
  - esp32
  - arduino
  - ble
  - iOS
  - iPadOS
  - Android
  - open-source-hardware
---

# Assistive Switch Interfaces

**Open-source, low-cost Bluetooth adaptive switch interfaces built on the ESP32-S3, for iPadOS/iOS Switch Control and Android Switch Access.**

This repository collects firmware for turning inexpensive, off-the-shelf components — capacitive touch pads, a proximity sensor, and ordinary 3.5 mm mono-jack mechanical switches — into standard **Bluetooth Low Energy (BLE) HID keyboards**. Every device in this repo pairs like any commercial Bluetooth keyboard: no companion app, no proprietary dongle, no vendor lock-in. iPadOS/iOS Switch Control and Android Switch Access recognize each device immediately and let a caregiver, teacher, or AT specialist map every physical input to a scanning, selection, or navigation action exactly as they would for a commercial switch interface costing many times more.

The goal is straightforward: assistive technology that a classroom, clinic, or individual family can build, flash, repair, and adapt themselves, using parts anyone can buy and code anyone can read.

> 📖 **Full documentation site:** [mrhunsaker.github.io/Assistive_Switch_Interfaces](https://mrhunsaker.github.io/Assistive_Switch_Interfaces/) — built automatically from the `docs/` folder in this repo via GitHub Actions (see `.github/workflows/pages.yml`).

---

[TOC]

---

# Why This Project Exists

Commercial switch interfaces are reliable but expensive, and they're closed boxes — if one input degrades, breaks, or doesn't suit a particular student or client, there's no way to open it up and change it. Every firmware in this repository is built around the same small set of design commitments:

- **Cheap, commodity hardware.** An ESP32-S3 SuperMini board, TTP223 touch modules, a standard I²C proximity sensor, and plain normally-open mechanical switches — nothing custom-fabricated, nothing proprietary.
- **Zero-install pairing.** Every device advertises as a standard BLE HID keyboard. There is no companion app to install, update, or lose support over time.
- **No crosstalk between inputs.** Every physical input gets its own independent debounce/edge-trigger state. One input being held, bouncing, or mid-press can never suppress, delay, or falsely trigger another input's keystroke.
- **Legible feedback.** An onboard RGB LED gives instant, simple color feedback (by input *type*, not by individual sensor) so non-technical staff can verify a device is working in seconds.
- **Fully open.** All firmware, wiring diagrams, and documentation are released under the Apache License 2.0 (see `LICENSE`) so any district, clinic, or individual maker can build, modify, and redistribute these devices without restriction.

---

# Projects in This Repository

| Folder | Documentation Page | What It Is | Inputs | HID Keys Sent |
|---|---|---|---|---|
| [`BLE_ADAPTIVE_SWITCH_EXPANDED_SCAFFOLD/`](./BLE_ADAPTIVE_SWITCH_EXPANDED_SCAFFOLD/) | [View →](https://mrhunsaker.github.io/Assistive_Switch_Interfaces/ble-adaptive-switch-expanded-scaffold.html) | The full 11-input adaptive switch node — the primary classroom/clinical device in this repo | 6 capacitive touch sensors, 1 proximity sensor, 4 mechanical switches | F1–F6 (touch), F8 (proximity), F9–F12 (switch) |
| [`BLE_3_SWITCH_INTERFACE/`](./BLE_3_SWITCH_INTERFACE/) | [View →](https://mrhunsaker.github.io/Assistive_Switch_Interfaces/ble-3-switch-interface.html) | A simplified 3-switch directional controller — no touch or proximity hardware, just three mechanical switches (left/middle/right) | 3 mechanical switches | LEFT_ARROW, ENTER, RIGHT_ARROW |
| [`BLE_3_TOUCH_INTERFACE/`](./BLE_3_TOUCH_INTERFACE/) | [View →](https://mrhunsaker.github.io/Assistive_Switch_Interfaces/ble-3-touch-interface.html) | The touch-sensor parallel of the 3-switch interface — no mechanical switches, just three TTP223 touch sensors (left/middle/right) | 3 capacitive touch sensors | LEFT_ARROW, ENTER, RIGHT_ARROW |

Each folder has its own complete README with full pinout, wiring, BLE behavior, LED reference, and build/flash instructions specific to that device. This top-level README covers what's shared across the whole project — the parts you need to get right once, regardless of which firmware you're flashing.

### BLE Adaptive Switch Node (Expanded Scaffold)

The flagship device: 11 independent inputs (6 touch, 1 proximity, 4 switch), each mapped to a unique HID key so that Switch Control / Switch Access can tell every input apart. Includes a district-oriented implementation manual (`BLE_Adaptive_Switch_District_Manual_Complete.docx`) written for teachers, paraeducators, AT specialists, and IT/technology staff, covering daily use and testing, pairing on iPad/iPhone/Android, firmware flashing, and asset management — no programming background required for the day-to-day sections.

### BLE 3-Switch Arrow/Enter Node

A minimal variant for use cases that only need directional navigation and selection: three mechanical switches mounted left/middle/right on an enclosure, sending `LEFT_ARROW`, `ENTER`, and `RIGHT_ARROW` respectively. Built from the same debounce/BLE/LED architecture as the expanded scaffold, with the touch and proximity hardware removed entirely.

### BLE 3-Touch Arrow/Enter Node

The touch-sensor parallel of the 3-switch interface: the exact same left/middle/right → `LEFT_ARROW`/`ENTER`/`RIGHT_ARROW` mapping and no-crosstalk debounce architecture, but built with three TTP223 capacitive touch sensors instead of mechanical switches. Useful where a momentary mechanical switch isn't practical or tolerable for a given user, but a light touch is.

---

# Shared Hardware Platform

All firmware in this repository targets the **ESP32-S3 SuperMini** (chip: ESP32S3FH4R2, 4 MB in-package flash, 2 MB in-package PSRAM). Both projects follow the same GPIO safety policy:

- GPIO 0, 45, and 46 are avoided (boot/strapping pins).
- GPIO 19/20 are avoided (USB D-/D+).
- GPIO 3 is avoided project-wide, even where it would be electrically safe for a given input, to keep the GPIO policy consistent across every variant built from this codebase.
- GPIO 48 is reserved for the onboard NeoPixel status LED.

Mechanical switches always use `INPUT_PULLUP` (switch closure pulls the pin LOW, treated as "pressed" in firmware) so no external resistors are needed. Capacitive touch pads (where present) are read directly as `INPUT`, since TTP223 modules drive their own output HIGH on touch.

---

# Shared Firmware Architecture

Every device in this repo is built from the same core pattern, regardless of how many or which kinds of inputs it has:

1. Each physical input is read every loop iteration and passed through a shared-*logic*-but-independent-*state* debounce function — every input has its own debounce struct instance, so timing on one input never affects another.
2. A debounced **transition into the active state** (not the held state) fires exactly one HID keystroke — press immediately followed by release a few tens of milliseconds later. Holding an input does not repeat the keystroke.
3. Keystrokes are sent over a NimBLE HID input report characteristic and notified to the connected host; if nothing is connected, the send is skipped and logged, with no queuing or buffering.
4. A standard Bluetooth **Battery Service** (UUID `0x180F`), pinned at 100%, is included in every build. This isn't cosmetic — some Android devices refuse to complete BLE HID keyboard pairing without it, and this was verified against a Nokia G100 running Android 12.
5. BLE reconnection is automatic: a `NimBLEServerCallbacks` subclass restarts advertising on every disconnect, so devices don't need to be manually re-paired unless a host has "forgotten" them.
6. An onboard RGB LED gives simple, type-based (not sensor-based) color feedback, fading back to off a fixed interval after the last event.

---

# Required Library Versions (Pinned)

These exact versions are what the firmware in this repository has been built and verified against. **Using different versions — especially newer major versions — is not guaranteed to compile or behave correctly**, and NimBLE's HID API in particular has changed across versions in ways that can silently break pairing or keystroke delivery.

| Library | Pinned Version |
|---|---|
| NimBLE-Arduino | **2.5.0** |
| Adafruit NeoPixel | **1.15.5** |
| SparkFun APDS9960 RGB and Gesture Sensor | **1.4.2** |
| Adafruit BusIO | **1.170.4** |
| Adafruit GFX Library | **1.12.3** |
| Adafruit SSD1306 | **2.5.14** |

Install all of these via `Sketch → Include Library → Manage Libraries` in the Arduino IDE, and pin each one to the exact version above rather than accepting the latest available release.

---

# Required Board Package Version

This project targets the **`esp32` board package (Espressif Systems) version 2.0.17** in the Arduino IDE Boards Manager.

> ⚠️ **This firmware will NOT work on board package version 3.3.10 (or other 3.x releases).** The Arduino-ESP32 3.x core line introduced breaking changes relative to the 2.x line that are incompatible with how this project's NimBLE HID stack and pin/peripheral handling are written. If you install board package 3.x, expect compile errors, pairing failures, or non-functional HID output even if the sketch appears to upload successfully.

To install the correct version:

1. `Tools → Board → Boards Manager`.
2. Search "esp32" (by Espressif Systems).
3. Use the version dropdown to select **2.0.17** specifically — do not install "latest."
4. If a newer version is already installed, use Boards Manager to downgrade to 2.0.17 before building any sketch in this repository.

---

# Getting Started

1. Pick the project that matches your needs — the full 11-input node, the 3-switch controller, or the 3-touch controller — and open its folder for the complete pinout and wiring diagram.
2. Install Arduino IDE board package **esp32 2.0.17** and the pinned library versions listed above.
3. Follow the build/flash instructions in that project's own README.
4. Pair the finished device with an iPad, iPhone, or Android device like any standard Bluetooth keyboard, then configure Switch Control (iOS/iPadOS) or Switch Access (Android) to recognize each key.

Each sub-project README also includes a Quick Functional Test so you can confirm every input is wired and firing correctly before deploying the device to a student or client.

---

# License

Released under the **Apache License 2.0** (see `LICENSE`). You are free to build, modify, and redistribute any device in this repository, including for commercial or district-wide deployment, subject to the terms of that license.

---

# Audience

This project is written to be usable by:

- **Individual makers and families** building a single device for personal or in-home use.
- **Teachers and paraeducators** who need to power on, pair, and test a device day-to-day with no programming background.
- **AT specialists** configuring Switch Control / Switch Access mappings for a specific student.
- **District IT/technology staff** flashing, maintaining, and inventorying devices at scale.

The expanded scaffold's district manual is written explicitly for that full range of roles; the individual project READMEs cover the technical build/flash process for maker and IT audiences.
