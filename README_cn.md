# FlipClock

基于 ESP32-C3 + ST7789 LCD (240×320) 的极简翻页时钟，265 行 C++ 实现透视翻页动画，无闪烁、零浮点、极致精简。

[English](README.md)

## 效果

<p align="center">
  <img src="misc/demo.gif" alt="FlipClock 演示" width="200">
</p>

翻页动画通过梯形条带变形模拟透视缩短，上半卡片折下后下半卡片展开，配合侧面厚度渲染，还原真实翻页钟的视觉效果。

## 特性

- **265 行极简实现** — 单个 `main.cpp`，无框架依赖，无冗余抽象
- **运行时零浮点** — 三角函数预计算为 `int8_t` 查找表，纯整数动画
- **帧缓冲直写** — `memcpy` 替代 `drawPixel()`，性能提升 10 倍
- **RGB565 预合成精灵图** — 运行时无 alpha 混合开销
- **节省 ~165KB RAM** — canvas 仅分配可见的 240×48 区域
- **双缓冲无闪烁** — GFXcanvas16 全帧渲染 + 单次 SPI 传输
- **20FPS 翻页动画** — 透视梯形条带变形，还原机械翻页钟效果

## 硬件

| 组件 | 型号 |
|------|------|
| MCU | ESP32-C3 (esp32-c3-devkitm-1) |
| 显示屏 | ST7789, 240x320, SPI |
| Flash | DIO 模式, 80MHz |

### 接线

| 引脚 | GPIO |
|------|------|
| MOSI | 3 |
| SCLK | 2 |
| DC | 4 |
| RST | 5 |
| CS | 7 |

## 构建与烧录

需要 [PlatformIO](https://platformio.org/) 环境。

```bash
pio run                    # 构建
pio run --target upload    # 烧录
pio device monitor         # 串口监视 (115200)
```

## 项目结构

```
src/
  main.cpp      # 主程序：SPI 初始化、翻页动画渲染、时间推进
  digitals.h    # 0-9 数字位图 RGB565 预合成数据（上下半各 32x24）
platformio.ini  # PlatformIO 构建配置
```

## 依赖

- Arduino Framework (C++11)
- Adafruit GFX Library 1.12.6
- ST7789_AVR 1.2.3
