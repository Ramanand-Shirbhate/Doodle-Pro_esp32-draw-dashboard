
# DOODLE PRO (esp32-draw-dashboard) 
**Turn your ESP32 into a lightning‑fast, edge‑free drawing canvas.**  
Our 8‑bit parallel driver eliminates distortion, delivering smooth, real‑time strokes on a 3.5″ resistive TFT while keeping Wi‑Fi/Bluetooth ready for IoT dashboards or smart‑home panels.

---  

## About the Project  

Resistive touch screens are cheap and robust, but they suffer from **edge distortion** and **non‑linear coordinate mapping** when driven over the default SPI interface. This repository demonstrates a **high‑performance, zero‑offset drawing canvas** built around an ESP32 and a 3.5″ ILI9486‑based resistive TFT.  

Key goals:  

* **Eliminate edge distortion** – a custom‑mapped 8‑bit parallel bus provides deterministic timing and eliminates the latency and jitter of SPI.  
* **Real‑time stroke rendering** – continuous interpolation of touch points at >60 fps, enabling fluid sketching.  
* **Preserve wireless capabilities** – the pinout leaves all Wi‑Fi/Bluetooth pins free, allowing the board to evolve into an IoT dashboard, smart‑home control panel, or Bluetooth‑connected sketchpad.  
* **Open‑source & extensible** – built with the Arduino IDE, using widely‑available libraries (TFT_eSPI, TouchScreen) that can be re‑configured for other displays or ESP32 variants.

The result is a **smooth, lag‑free drawing experience** on a low‑cost resistive display, ready to be the UI front‑end for any ESP32‑based project.

---  

## Tech Stack  

| Layer | Technology / Library | Reason for Choice |
|-------|----------------------|-------------------|
| **Microcontroller** | **ESP32 (any Dev Board)** | Dual‑core, Wi‑Fi/Bluetooth, ample GPIO for 8‑bit parallel bus |
| **Display Driver** | **TFT_eSPI** (custom `USER_SETUP.h`) | Highly optimised, supports 8‑bit parallel mode, ILI9486 driver |
| **Touch Interface** | **TouchScreen.h** | Simple raw ADC reading, pressure (Z) gating for reliable touch detection |
| **IDE / Build System** | **Arduino IDE (C++)** | Fast prototyping, large community, easy library management |
| **Media Assets** | JPEG images (demo screenshots) | Visual proof of concept, used in the README only |

---  

## Repository Structure  

```
Doodle-Pro_esp32-draw-dashboard/
│
├── esp32Canvas.ino            # Main sketch – initializes the TFT, calibrates touch,
│                               runs the drawing loop, handles stroke interpolation,
│                               and provides placeholder hooks for Wi‑Fi/BLE features.
│
├── esp32TFTcallibration.ino   # Stand‑alone calibration utility. Guides the user
│                               through a 5‑point (or 9‑point) calibration routine
│                               and stores the resulting transformation matrix
│                               in EEPROM/Preferences for later use by esp32Canvas.
│
├── USER_SETUP.h               # TFT_eSPI configuration file.  
│   • Sets the driver to **ILI9486**  
│   • Enables **8‑bit parallel** mode and defines the exact GPIO mapping  
│   • Disables SPI pins to keep Wi‑Fi/BLE pins free  
│   • Adjusts clock speed, color depth, and other performance flags.
│
├── media/                     # Demo images used in documentation and for quick
│   ├── DSC01386.jpg           # visual verification of the drawing canvas.
│   ├── DSC01389.jpg
│   └── DSC01392.jpg
│
└── README.md                  # (this file)
```

### File‑by‑file breakdown  

| File | Purpose | Important Functions / Sections |
|------|---------|--------------------------------|
| **esp32Canvas.ino** | Core application. Sets up the display, reads calibrated touch data, interpolates points with Bresenham’s line algorithm, draws to the screen, and clears the canvas on long‑press. | `setup()` → init TFT, load calibration, init touch; `loop()` → read touch, pressure check, call `drawStroke()`. |
| **esp32TFTcallibration.ino** | One‑time calibration tool. Guides the user to touch known screen locations, computes scaling/offset, stores results. | `calibrateTouch()` – displays crosshairs, captures raw ADC values, writes to `Preferences`. |
| **USER_SETUP.h** | TFT_eSPI hardware definition. All GPIO pins are explicitly mapped to the ESP32 pins used for the 8‑bit parallel bus (D0‑D7, WR, RD, CS, DC, RST). | `#define ILI9486_DRIVER`, `#define TFT_PARALLEL_8_BIT`, pin definitions (`#define TFT_DC   2`, etc.). |
| **media/*.jpg** | Screenshots that demonstrate the drawing canvas in action (used only for documentation). | No code impact. |
| **README.md** | Project documentation (this file). | — |

---  

## Getting Started  

### Prerequisites  

1. **Hardware**  
   * ESP32 Development Board (e.g., ESP32‑DevKitC, WROOM‑32)  
   * 3.5″ Resistive TFT with ILI9486 driver (8‑bit parallel interface)  
   * Jumper wires / breadboard or a custom PCB that follows the pinout defined in `USER_SETUP.h`  

2. **Software**  
   * Arduino IDE 2.x (or the classic 1.8.x) with **ESP32 Board Support** installed  
   * Libraries (install via Library Manager)  
     - **TFT_eSPI** (latest) – *do not edit the library’s default `User_Setup.h`*  
     - **TouchScreen** (by Paul Stoffregen)  
   * (Optional) **Preferences** library – already bundled with ESP32 core for EEPROM‑like storage  

### Wiring (as defined in `USER_SETUP.h`)  

| ESP32 Pin | TFT Pin | Function |
|-----------|---------|----------|
| GPIO 2    | DC      | Data/Command select |
| GPIO 4    | CS      | Chip select (active low) |
| GPIO 5    | RST     | Reset |
| GPIO 12   | WR      | Write strobe |
| GPIO 13   | RD      | Read strobe (tied low if not used) |
| GPIO 14‑21| D0‑D7   | 8‑bit parallel data bus |
| GPIO 34‑39| X+, Y+, Z (ADC) | Touch panel analog inputs (configurable in sketch) |

> **Tip:** Keep all Wi‑Fi/BLE pins (e.g., GPIO 0, 1, 3, 21‑23) free – they are not used by the display driver.

### Installation  

1. **Clone the repository** (or download the zip)  

   ```bash
   git clone https://github.com/yourusername/esp32-draw-dashboard.git
   cd esp32-draw-dashboard
   ```

2. **Configure TFT_eSPI**  

   * In the library folder `TFT_eSPI/User_Setup.h` comment out any existing `#include <User_Setup.h>` line.  
   * Copy the repository’s `USER_SETUP.h` into the same folder (or edit the library’s `User_Setup_Select.h` to point to this file).  

3. **Open the sketch**  

   * Launch Arduino IDE → *File → Open* → select `esp32Canvas.ino`.  

4. **Select the board**  

   * Tools → Board → **ESP32 Arduino** → *Your ESP32 Dev Board*  
   * Set the appropriate **Upload Speed** (115200 works fine).  

5. **Install required libraries** (if not already present)  

   * Sketch → Include Library → Manage Libraries…  
   * Search for **TFT_eSPI** → Install  
   * Search for **TouchScreen** → Install  

### Calibration (first run)  

1. Upload `esp32TFTcallibration.ino` to the board.  
2. The display will show a series of cross‑hairs; touch each point as prompted.  
3. After the final point, the sketch stores the calibration matrix in non‑volatile storage.  
4. Re‑upload `esp32Canvas.ino`. The drawing canvas will now use the saved calibration data.

### Running the Drawing Canvas  

1. Upload `esp32Canvas.ino`.  
2. Power the ESP32 (USB or external 5 V).  
3. The screen will clear to a white background and display a small “Ready” indicator.  
4. **Draw** with a stylus or finger: the canvas interpolates between successive touch points, producing smooth lines.  
5. **Long‑press (≥2 s)** anywhere to clear the screen.  

### Extending the Project  

* **Wi‑Fi Dashboard** – after confirming the drawing works, enable Wi‑Fi in `setup()` and serve the canvas as a remote control surface via a simple WebSocket server.  
* **Bluetooth Sketchpad** – use `BluetoothSerial` to stream coordinates to a paired device.  
* **Custom UI Widgets** – draw buttons, sliders, or icons on the same TFT; use touch zones to trigger actions.  

---  

## License  

This project is released under the **MIT License** – see the `LICENSE` file for details.  

---  

## Acknowledgements  

* **Bodmer** – author of the TFT_eSPI library, which makes parallel‑bus driving feasible on ESP32.  
* **Paul Stoffregen** – creator of the TouchScreen library for reliable resistive touch handling.  

---  

*Happy drawing! 🚀*
