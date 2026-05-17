#include <Arduino.h>
#include <SPI.h>
#include <math.h>
#include <Adafruit_GFX.h>
#include <ST7789_AVR.h>
#include "digitals.h"

// 硬件引脚
#define TFT_MOSI 3
#define TFT_SCLK 2
#define TFT_DC   4
#define TFT_RST  5
#define TFT_CS   7

// 屏幕
#define SCR_WD 240
#define SCR_HT 320

// 卡片尺寸
#define CW   32
#define CH   48
#define HALF 24

// 布局
#define GAP      4
#define COLON_W  8
#define N_STRIPS 12

// 动画参数
#define FPS          20
#define TOTAL_FRAMES 20
#define ANIM_HALF    10

// 颜色 (RGB565)
#define BG_COLOR   DGREY
#define CARD_BG    0x0841
#define COLON_CLR  0x8C51

ST7789_AVR lcd(TFT_DC, TFT_RST, TFT_CS);
GFXcanvas16 *canvas = nullptr;

// 布局：6个数字的X坐标 [秒个,秒十,分个,分十,时个,时十]
static int8_t digit_x[6];
static int8_t colon_x[2];

// 翻页动画查找表 (启动时计算，运行时零浮点)
struct StripEntry {
  int8_t dy_top, dy_bot, ext;
};
struct FlipEntry {
  int8_t vis;
  StripEntry upper[N_STRIPS];
  StripEntry lower[N_STRIPS];
};
static FlipEntry flip_table[ANIM_HALF];

// 数字状态
struct Digit {
  uint8_t cur;
  uint8_t max_val;
  uint8_t old;
  int8_t anim_frame; // -1 = 无动画
};
static Digit digits[6];

void calc_layout() {
  int group_w = CW * 2 + GAP;
  int colon_gap = GAP + COLON_W + GAP;
  int total_w = group_w * 3 + colon_gap * 2;
  int x = (SCR_WD - total_w) / 2;
  // 组顺序: [时十,时个] [分十,分个] [秒十,秒个]
  // 索引:     5     4     3     2     1     0
  for (int g = 0; g < 3; g++) {
    digit_x[5 - g * 2] = x;
    digit_x[4 - g * 2] = x + CW + GAP;
    x += group_w;
    if (g < 2) {
      colon_x[g] = x + GAP + COLON_W / 2;
      x += colon_gap;
    }
  }
}

void build_flip_table() {
  for (int f = 0; f < ANIM_HALF; f++) {
    float angle = (float)f / ANIM_HALF * (float)M_PI / 2;
    int vis = max(1, (int)roundf((float)HALF * cosf(angle)));
    float widen = 0.25f * sinf(angle);
    flip_table[f].vis = (int8_t)vis;
    for (int i = 0; i < N_STRIPS; i++) {
      float t = (float)i / N_STRIPS;
      float sh = (float)vis / N_STRIPS;
      flip_table[f].upper[i] = {
        (int8_t)roundf(-vis + i * sh),
        (int8_t)roundf(-vis + (i + 1) * sh),
        (int8_t)roundf(widen * CW * (1 - t) / 2)
      };
      flip_table[f].lower[i] = {
        (int8_t)roundf(i * sh),
        (int8_t)roundf((i + 1) * sh),
        (int8_t)roundf(widen * CW * t / 2)
      };
    }
  }
}

void init_time() {
  // 从 12:00:00 开始
  digits[0] = {0, 9, 0, -1}; // 秒个
  digits[1] = {0, 5, 0, -1}; // 秒十
  digits[2] = {0, 9, 0, -1}; // 分个
  digits[3] = {0, 5, 0, -1}; // 分十
  digits[4] = {2, 9, 2, -1}; // 时个
  digits[5] = {1, 2, 1, -1}; // 时十
}

void advance(int idx) {
  if (idx >= 6) return;
  Digit &d = digits[idx];
  d.old = d.cur;
  d.cur++;
  bool overflow = false;
  if (idx == 4) {
    // 时个位: 23:59:59 → 00:00:00
    int h_max = (digits[5].cur == 2) ? 3 : 9;
    if (d.cur > h_max) { d.cur = 0; overflow = true; }
  } else if (d.cur > d.max_val) {
    d.cur = 0;
    overflow = true;
  }
  d.anim_frame = 0;
  if (overflow) advance(idx + 1);
}

// 将位图半张复制到canvas
void blit_half(const uint16_t *src, int cx, int cy) {
  uint16_t *buf = canvas->getBuffer();
  for (int y = 0; y < HALF; y++)
    memcpy(&buf[(cy + y) * SCR_WD + cx], &src[y * CW], CW * 2);
}

// 梯形条带渲染 (翻页动画核心)
void draw_trapezoid(const uint16_t *src, const StripEntry *strips, int cx) {
  uint16_t *buf = canvas->getBuffer();
  for (int i = 0; i < N_STRIPS; i++) {
    int dy_top = HALF + strips[i].dy_top;
    int dy_bot = HALF + strips[i].dy_bot;
    int dst_h = dy_bot - dy_top;
    if (dst_h < 1) continue;
    int mid = dst_h >> 1;
    int ext = strips[i].ext;
    for (int y = 0; y < dst_h; y++) {
      int cy = dy_top + y;
      if (cy < 0 || cy >= CH) continue;
      uint16_t *row = &buf[cy * SCR_WD];
      int src_row = 2 * i + ((y < mid) ? 0 : 1);
      memcpy(&row[cx], &src[src_row * CW], CW * 2);
      // 透视扩展 (模拟卡片厚度)
      for (int e = 0; e < ext; e++) {
        if (cx - 1 - e >= 0) row[cx - 1 - e] = CARD_BG;
        if (cx + CW + e < SCR_WD) row[cx + CW + e] = CARD_BG;
      }
    }
  }
}

void render_card(int idx) {
  Digit &d = digits[idx];
  int cx = digit_x[idx];

  if (d.anim_frame < 0 || d.anim_frame >= TOTAL_FRAMES) {
    // 静态卡片
    blit_half(DIGIT_UPPER[d.cur], cx, 0);
    blit_half(DIGIT_LOWER[d.cur], cx, HALF);
    if (d.anim_frame >= TOTAL_FRAMES) d.anim_frame = -1;
    return;
  }

  // 底层: 新数字上半 + 旧数字下半
  blit_half(DIGIT_UPPER[d.cur], cx, 0);
  blit_half(DIGIT_LOWER[d.old], cx, HALF);

  // 翻页动画叠加
  if (d.anim_frame < ANIM_HALF) {
    // 上半翻转: 旧数字上半折下来
    draw_trapezoid(DIGIT_UPPER[d.old], flip_table[d.anim_frame].upper, cx);
  } else {
    // 下半翻转: 新数字下半展开
    draw_trapezoid(DIGIT_LOWER[d.cur],
                   flip_table[TOTAL_FRAMES - 1 - d.anim_frame].lower, cx);
  }
}

void render_colon(int cx) {
  uint16_t *buf = canvas->getBuffer();
  int y1 = CH * 3 / 10;
  int y2 = CH * 7 / 10;
  for (int dy = -2; dy <= 2; dy++) {
    for (int dx = -2; dx <= 2; dx++) {
      if (dx * dx + dy * dy > 5) continue;
      if (y1 + dy >= 0 && y1 + dy < CH)
        buf[(y1 + dy) * SCR_WD + cx + dx] = COLON_CLR;
      if (y2 + dy >= 0 && y2 + dy < CH)
        buf[(y2 + dy) * SCR_WD + cx + dx] = COLON_CLR;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  if (!SPI.begin(TFT_SCLK, -1, TFT_MOSI)) {
    Serial.println("SPI init failed");
    return;
  }
  canvas = new GFXcanvas16(SCR_WD, CH);
  lcd.init(SCR_WD, SCR_HT);
  lcd.invertDisplay(true);
  lcd.fillScreen(BG_COLOR);

  calc_layout();
  build_flip_table();
  init_time();

  Serial.println("FlipClock ready");
}

void loop() {
  static uint32_t next_second = 0;
  static uint32_t next_render = 0;
  static uint32_t fps_count = 0;
  uint32_t now = millis();

  // 每秒推进
  if (next_second == 0) next_second = now + 1000;
  if ((int32_t)(now - next_second) >= 0) {
    next_second += 1000;
    advance(0);
  }

  // 20FPS 渲染
  if ((int32_t)(now - next_render) >= 0) {
    next_render = now + 1000 / FPS;

    canvas->fillScreen(BG_COLOR);
    for (int i = 0; i < 6; i++) render_card(i);
    for (int i = 0; i < 2; i++) render_colon(colon_x[i]);
    lcd.drawImage(0, (SCR_HT - CH) / 2, SCR_WD, CH, canvas->getBuffer());

    // 推进动画帧
    for (int i = 0; i < 6; i++)
      if (digits[i].anim_frame >= 0) digits[i].anim_frame++;
    fps_count++;
  }

  // 帧率报告
  static uint32_t next_report = 0;
  if (next_report == 0) next_report = now + 10000;
  if ((int32_t)(now - next_report) >= 0) {
    next_report += 10000;
    Serial.printf("FPS: %.1f\n", fps_count / 10.0);
    fps_count = 0;
  }
}
