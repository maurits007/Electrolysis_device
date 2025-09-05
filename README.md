# Electrolysis Device Controller – Arduino R4 Wifi

## Version
**v1.0** – Last updated: 2025-09-05  

---

## Table of Contents
1. [Overview](#overview)  
2. [Startup](#startup)  
3. [Normal Usage](#normal-usage)  
4. [Buttons](#buttons)  
5. [Main menu](#main-menu)  
6. [Advanced Settings / Calibration](#advanced-settings--calibration)  
7. [LED Indicators](#led-indicators)  
8. [Bugs / Troubleshooting](#bugs--troubleshooting) 
9. [Values achieved](#values-achieved) 

---

## Overview
This Arduino project allows you to control an electrolysis device via a physical interface (buttons and OLED) or a web interface through WiFi.  
You can set the desired current, the operating time, and monitor voltage, current, and resistance in real-time.

---

## Startup
1. The Arduino will create a WiFi hotspot:  
   - **SSID:** `ArduinoAP`  
   - **Password:** `12345678`  
2. Connect to the WiFi (if wanted) and access the web server using a browser:  
   - Type the IP address shown on the bottom of the OLED screen (usually `192.168.4.1`).  
3. Connect the electrolysis device to the PCB:  
   - **Red wire:** `Ld +`  
   - **Black wire:** `Ld -`  
   - If connected correctly, the LED should light **yellow/green** (not red).  

---

## Normal Usage
1. Start the device and optionally connect via WiFi.  
2. Connect the electrolysis device as described above.  
3. Select the desired current (`I want`) and time (`Time`).  
4. Press the **small button** to start:  
   - The LED turns **green** if running.  
   - The OLED shows `I real` (actual current) and elapsed time.  
5. When the reaction is complete:  
   - LED turns **blue**.  
   - The ending screen is displayed.  
   - Press the small button or use the web interface to restart.  

---

## Buttons

| Button | Function |
|--------|---------|
| **Encoder (turnable)** | Select a menu line by turning. Press to start editing the value. Press again to stop editing. |
| **Small button** | Start/stop electrolysis. LED indicates the device state. |

---

## Main menu
The OLED screen has **8 lines**:

| Line | Description |
|------|------------|
| Time | Time passed / total wanted time |
| I want | Desired current |
| I real | Actual current through the device |
| V load | Voltage across the device (mV) |
| R load | Device resistance (ohms) – inaccurate if device is off |
| Reset time | Reset the timer to restart the electrolysis reaction |
| Advanced Settings | Access [calibration options](#advanced-settings--calibration ) and PCB selection |
| IP | IP address to access the web interface |

> Bottom-right corner: ❚❚ (paused) or ▶ (running)

---

## Advanced Settings / Calibration
The advanced menu has **5 lines**:

Here the PCB can be calibrated.

| Line | Description |
|------|------------|
| PCB number | Select which PCB configuration to use (2 saved configurations supported) |
| Cur. res. | Current sense resistor value for calibration |
| A0 | Multiplier for output voltage calibration (`I load`) |
| A1 | Multiplier for measured current calibration (`I meas`) |
| Load | Multiplier for measured voltage over the load |

---

## LED Indicators

| Color | Meaning |
|-------|---------|
| Red | Device not connected correctly |
| Yellow | Electrolysis off |
| Green | Electrolysis running |
| Blue | Electrolysis finished |

---

## Bugs / Troubleshooting
- **OLED screen freezes:** Unplug and plug the Arduino back in.  
- **Red LED stays on:** Check that `Ld +` and `Ld -` are connected correctly.  
- **Keeps pausing:** There is a safety measure if there is too much power over the MOSFET. Increasing or decreasing the current both help decreasing the power and the device will be able to start again.


## Values achieved
- **No external voltage:** 210 mA
- **6V ext:** 310mA


---
-------
## Future extensions
- **Using external voltage:** First voltage dividers and zener diodes have to be added to the pins reading the voltage over the load to protect it against overvoltage.
- **using 3V3 as ADC reference:** Currently no significant improvements were found using it.

- **Improvements PCB:**
   1. Make holes LCD screen bigger.
   2. Correct orientation MOSFET or replace by through-hole MOSFET
   3. To use pin A4, A5 the screen has to be connected to QWIIC.
   
