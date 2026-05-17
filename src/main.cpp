#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <ST7789_AVR.h>

#define TFT_MOSI    3
#define TFT_SCLK    2
#define TFT_DC      4
#define TFT_RST     5
#define TFT_CS      7
#define SCR_WD 240
#define SCR_HT 320

ST7789_AVR lcd = ST7789_AVR(TFT_DC, TFT_RST, TFT_CS);
GFXcanvas16 *canvas = nullptr;

void setup(void) 
{
  Serial.begin(115200);
  delay(2000);
  if (!SPI.begin(TFT_SCLK, -1, TFT_MOSI)) {
    Serial.println("SPI begin failed");
    return;
  }
  canvas = new GFXcanvas16(240, 48);
  lcd.init(SCR_WD, SCR_HT);
  lcd.invertDisplay(true);
  lcd.fillScreen(RED);

 }

void loop() {
  static uint32_t ts_nxt_render = 0, ts_nxt_report = 0, render_count = 0;
  uint32_t ts_now = millis();
  if(ts_nxt_render < ts_now){
    ts_nxt_render = ts_now + 33; // FPS -> 30

    canvas->fillScreen(BLACK);
    canvas->setCursor(0, 0);
    canvas->setTextColor(WHITE, BLUE);
    canvas->setTextSize(3);
    canvas->printf("T: %u", ts_now);
    lcd.drawImage(0, 40, canvas->width(), canvas->height(), canvas->getBuffer());
    render_count++;
  }
  if(ts_nxt_report < ts_now){
    ts_nxt_report = ts_now + 10000;
    Serial.printf("render_count: %u / 10 seconds\n", render_count);
    render_count = 0;
  }
}
