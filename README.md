# FlipClock

ESP32-C3 flip clock with a ST7789 LCD (240x320), rendering digit transitions with a realistic page-flip animation.

[中文文档](README_cn.md)

## Preview

The flip animation uses trapezoidal strip distortion to simulate foreshortening in perspective. The upper half folds down, the lower half unfolds, and side thickness shading completes the classic mechanical flip-clock look.

## Hardware

| Component | Model |
|-----------|-------|
| MCU | ESP32-C3 (esp32-c3-devkitm-1) |
| Display | ST7789, 240x320, SPI |
| Flash | DIO mode, 80 MHz |

### Wiring

| Pin | GPIO |
|-----|------|
| MOSI | 3 |
| SCLK | 2 |
| DC | 4 |
| RST | 5 |
| CS | 7 |

## Build & Flash

Requires [PlatformIO](https://platformio.org/).

```bash
pio run                    # build
pio run --target upload    # flash
pio device monitor         # serial monitor (115200)
```

## Project Structure

```
src/
  main.cpp      # Entry point: SPI init, flip animation rendering, time keeping
  digitals.h    # 0-9 digit bitmaps as pre-composited RGB565 data (upper/lower halves, 32x24 each)
platformio.ini  # PlatformIO build configuration
```

## Technical Details

- **Animation**: 20 FPS. Each digit change triggers a 20-frame flip (frames 1-10 upper half folds down, frames 11-20 lower half unfolds)
- **Rendering**: Off-screen via GFXcanvas16, full-frame memcpy to LCD — no partial-refresh flicker
- **Performance**: Float math pre-computed into lookup tables at startup; zero floating-point ops at runtime
- **Digit cards**: 32x48 px, 24 px per half, 12 trapezoidal strips per deformation

## Dependencies

- Arduino Framework (C++11)
- Adafruit GFX Library 1.12.6
- ST7789_AVR 1.2.3
