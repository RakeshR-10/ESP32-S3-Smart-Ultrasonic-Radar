# ESP32-S3 Smart Ultrasonic Radar System

A low-cost radar-style scanning prototype built around an ESP32-S3, HC-SR04 ultrasonic sensor, servo motor, and TFT display. The project includes physical-hardware firmware, a Wokwi simulation, the project report, a demonstration video, and 3D-printable mechanical models.

## Repository contents

- `src/physical_hardware/physical_radar_final.ino` — final physical-hardware code, based on the approved radar code with the flicker correction and existing display/servo behavior preserved.
- `simulation/wokwi/` — Wokwi simulation sketch, `diagram.json`, and library list.
- `docs/SMART_RADAR_Final.pdf` — project report.
- `demo/radar_demo.mp4` — supplied demonstration video.
- `3d-models/` — STL models, OpenSCAD source, and model ZIP archives for the servo mount and enclosure variants.

## Hardware described in the report

- ESP32-S3 development board
- HC-SR04 ultrasonic sensor
- SG90 servo motor
- 1.8-inch ST7735 TFT display
- External 5 V supply for the servo
- Voltage divider on the HC-SR04 ECHO line
- 3D-printed mounting parts

## Physical-hardware pin mapping

| Signal | ESP32-S3 GPIO |
|---|---:|
| HC-SR04 TRIG | 7 |
| HC-SR04 ECHO (through voltage divider) | 16 |
| Servo signal | 17 |
| TFT CS | 4 |
| TFT DC | 15 |
| TFT RESET | 9 |
| TFT MOSI | 11 |
| TFT SCK | 12 |

Use a common ground between the ESP32-S3 and the external servo supply. Do not connect the HC-SR04 ECHO output directly to an ESP32 GPIO without level reduction; the ESP32-S3 GPIO is 3.3 V logic.

## Wokwi simulation

Open `simulation/wokwi/` in a Wokwi ESP32 project and use its sketch and `diagram.json`. The simulation uses an **ILI9341 display** for Wokwi compatibility, while the physical-hardware code targets the **ST7735 display**. They are separate sketches and should not be interchanged without adapting the display library and dimensions.

Required Wokwi libraries are listed in `simulation/wokwi/libraries.txt` and in the diagram dependencies.

## Build notes

The physical sketch is intended for an ESP32-S3 board using the Arduino framework and the Adafruit GFX, Adafruit ST7735, and ESP32Servo libraries. PlatformIO users can create a PlatformIO project and copy the sketch into its `src` directory; add the required libraries to `platformio.ini` as appropriate for the installed library versions.

## Test status and limitations

The project report records that the TFT radar graphics and servo movement were observed, but reliable ultrasonic distance measurement was **not yet confirmed** during the reported tests because repeated `NO ECHO` readings occurred. Sensor wiring, the ECHO voltage divider, common ground, and sensor operation should be validated before claiming dependable object detection.

## 3D models

The `3d-models/` directory includes STL files for printing and an OpenSCAD source file where available. Check dimensions and fit against your actual board, display, servo, and wiring before printing; model variants may differ.

## Demonstration

See `demo/radar_demo.mp4` for the supplied project video.

## Author

Rakesh R  
B.E. Electronics and Communication Engineering  
Sri Eshwar College of Engineering
