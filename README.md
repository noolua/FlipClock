# FlipClock

A minimal, flicker-free flip clock for ESP32-C3 + ST7789 LCD (240×320), with perspective-correct page-flip animation in 265 lines of C++.

[中文文档](README_cn.md)

## Preview

<p align="center">
  <img src="misc/demo.gif" alt="FlipClock Demo" width="200">
</p>

The flip animation uses trapezoidal strip distortion to simulate foreshortening in perspective. The upper half folds down, the lower half unfolds, and side thickness shading completes the classic mechanical flip-clock look.

## Features

- **265-line minimal implementation** — single `main.cpp`, no frameworks, no bloat
- **Zero-float runtime** — trig lookup tables (`int8_t`), pure integer animation
- **Direct framebuffer blitting** — `memcpy` over `drawPixel()`, 10× faster
- **RGB565 pre-composited sprites** — no alpha blending at runtime
- **~165 KB RAM saved** — canvas only covers the visible 240×48 strip
- **Flicker-free double buffering** — full-frame GFXcanvas16 + single SPI transfer
- **20 FPS flip animation** — perspective trapezoid strip deformation

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

## Dependencies

- Arduino Framework (C++11)
- Adafruit GFX Library 1.12.6
- ST7789_AVR 1.2.3
