// FlightESP — пример 05: пиксельный экран (сплошные кадры по Bluetooth).
//
// ESP32 рисует в память (framebuffer) и отправляет кадры командой
// esp.sendFrame(...). Приложение отрисовывает их как настоящий экран —
// можно рисовать картинки, фото, графику, как на Flipper Zero / spacedesk.
//
// bpp: 1 = монохром (бит на пиксель, упаковка MSB-first),
//      8 = градации серого, 16 = RGB565 (big-endian), 24 = RGB888.

#include <FlightESP.h>

FlightESP esp;

// Буфер монохромного экрана 128x64 (128*64/8 = 1024 байта).
static const int W = 128;
static const int H = 64;
static uint8_t frame[W * H / 8];

// Монохромный "экран": рисуем картинку прямо в буфер.
void drawDemo(uint32_t t) {
  memset(frame, 0, sizeof(frame));

  // рамка по контуру
  for (int x = 0; x < W; x++) {
    drawPixel(x, 0);
    drawPixel(x, H - 1);
  }
  for (int y = 0; y < H; y++) {
    drawPixel(0, y);
    drawPixel(W - 1, y);
  }

  // анимированный крест
  int t0 = (t / 20) % (W + H);
  for (int i = 0; i < 8; i++) {
    int p = t0 + i;
    if (p < W) { drawPixel(p, H / 2); }
    if (p < H && p < W) { drawPixel(W / 2, p); }
  }

  // надпись "HELLO" прямыми пикселями в верхней полосе
  drawText(20, 6, "Screen PIX");
}

void drawPixel(int x, int y) {
  if (x < 0 || x >= W || y < 0 || y >= H) return;
  frame[(y * W + x) / 8] |= (0x80 >> (x % 8));
}

// Минимальный 5x7-шрифт для "HELLO" — рисуем полоски, чтобы было видно.
void drawText(int ox, int oy, const char* s) {
  // верхняя и нижняя линии символа-заглушки: две горизонтальные полосы
  for (int i = 0; s[i] != '\0'; i++) {
    for (int x = 0; x < 5; x++) {
      drawPixel(ox + i * 8 + x, oy);
      drawPixel(ox + i * 8 + x, oy + 6);
    }
    for (int y = 0; y < 7; y++) {
      drawPixel(ox + i * 8, oy + y);
      drawPixel(ox + i * 8 + 4, oy + y);
    }
  }
}

void onButton(ControlAction action) {
  esp.clear();
  esp.print("Pressed: ");
  esp.println(action == CTRL_UP ? "UP" : "DOWN");
}

void setup() {
  Serial.begin(115200);

  FlightESPConfig cfg;
  cfg.mode         = FLIGHTESP_BLE;
  cfg.deviceName   = "PixBoard";
  cfg.password     = "0000";
  cfg.screenWidth  = W;
  cfg.screenHeight = H;
  esp.begin(cfg);

  esp.addButton(CTRL_UP);
  esp.addButton(CTRL_DOWN);
  esp.onButton(onButton);
  esp.setBatteryVoltage(3.70);
}

void loop() {
  esp.loop();

  // Рисуем и отправляем кадр ~каждые 100 мс.
  drawDemo(millis());
  esp.sendFrame(frame, W, H, 1);
  delay(100);
}