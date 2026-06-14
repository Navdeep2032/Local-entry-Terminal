# ESP32 Hardware Terminal

A secure local hardware terminal prototype built on ESP32 that manages user entry using Role-Based Access Control (RBAC) — no network required.

Built as part of an embedded systems assignment at **IIT Guwahati**.

---

## Demo

> 🔗 **Wokwi Simulation:** [Click to open](https://wokwi.com/projects/466788091244715009)

---

## Features

- **4-digit ID entry** with `*` masking on LCD display
- **Role-Based Access Control (RBAC)** — hardcoded users mapped via C++ structs
- **Non-blocking 7-second timeout** — clears incomplete input using `millis()`, no `delay()` stalls
- **Personalized welcome screen** — shows name + custom message per user on access granted
- **Unique LED + buzzer pattern** per role on successful login
- **Security lockout** — 3 consecutive wrong IDs triggers a 15-second visual lockout, disabling the keypad
- **Backspace support** — `<` key deletes last digit, `=` key submits

---

## Hardware Components

| Component | Quantity |
|---|---|
| ESP32 DevKit V1 | 1 |
| 4×4 Matrix Keypad | 1 |
| LCD 16×2 with I2C backpack (addr 0x27) | 1 |
| LED – Green | 1 |
| LED – Blue | 1 |
| LED – Red | 1 |
| 220Ω Resistor | 3 |
| Passive Buzzer | 1 |
| Jumper Wires | — |

---

## Pin Assignment

| Component | Pin | ESP32 GPIO |
|---|---|---|
| Keypad Row 1–4 | R1, R2, R3, R4 | 13, 12, 14, 27 |
| Keypad Col 1–4 | C1, C2, C3, C4 | 26, 25, 33, 32 |
| LCD SDA | SDA | 21 |
| LCD SCL | SCL | 22 |
| LCD Power | VCC / GND | 3V3 / GND |
| LED Red | Anode → 220Ω → GND | 2 |
| LED Green | Anode → 220Ω → GND | 4 |
| LED Blue | Anode → 220Ω → GND | 5 |
| Buzzer | + / − | 18 / GND |

---

## Keypad Layout

```
1  2  3  A
4  5  6  B
7  8  9  C
<  0  =  D
```

| Key | Function |
|---|---|
| `0–9` | Enter digit |
| `<` | Backspace |
| `=` | Submit ID |

---

## Valid Test IDs

| ID | Name | Role | LED Pattern | Buzzer |
|---|---|---|---|---|
| `1234` | Alice | Admin | Green rapid triple-blink → solid green | Triple ascending beep |
| `5678` | Bob | Operator | Blue double slow pulse → solid blue | Double beep |
| `9012` | Carol | Operator | Blue & green alternate → solid blue | Two-tone beep |
| `3456` | Dave | Viewer | Red long flash → solid blue | Single low beep |

---

## State Machine

```
ST_IDLE ──► ST_INPUT ──► ST_GRANTED (2s) ──► ST_IDLE
                │
                └──► ST_DENIED (2s) ──► ST_IDLE
                          │
                    (3rd failure)
                          │
                          └──► ST_LOCKOUT (15s) ──► ST_IDLE
```

- **ST_IDLE** — waiting for first keypress
- **ST_INPUT** — digits being entered; 7s inactivity resets to idle
- **ST_GRANTED** — access granted, shows welcome screen for 3s
- **ST_DENIED** — wrong ID, shows attempt count for 2s
- **ST_LOCKOUT** — keypad disabled, countdown shown on LCD for 15s

---

## Libraries Required

Install via Arduino Library Manager or Wokwi's library panel:

- [`LiquidCrystal I2C`](https://github.com/johnrickman/LiquidCrystal_I2C) — v1.1.2
- [`Keypad`](https://github.com/Chris--A/Keypad) — v3.1.1

---

## File Structure

```
├── terminal.cpp            # Main source code
├── diagram.json            # Wokwi circuit wiring
├── ciruit_screenshot.png   # Screen shot of ciruit
└── README.md
```

---

## How to Run on Wokwi

1. Go to [wokwi.com](https://wokwi.com) → New Project → ESP32
2. Replace `sketch.ino` with the contents of `terminal.cpp`
3. Replace `diagram.json` with the provided file
4. Add libraries: `LiquidCrystal I2C` and `Keypad`
5. Hit **Run** — serial monitor shows `[BOOT] Terminal ready.`

---

## License

MIT License — feel free to use and modify.