//
// ColorScreen — простой цветной виртуальный экран.
//
// Кнопки пульта меняют цвет экрана в приложении:
//   UP / LEFT   — следующий цвет
//   DOWN / RIGHT — предыдущий цвет
//   OK           — экран вкл/выкл (белый / цветной)
// Тумблеры:
//   RGB Strip    — тоже меняет цвет экрана
//   Scan Signal  — просто вывод на экран
//

#include <FlightESP.h>

FlightESP esp;

// Палитра экрана.
static const uint8_t palette[][3] = {
  {255,   0,   0},  // 0 RED
  {  0, 255,   0},  // 1 GREEN
  {  0,   0, 255},  // 2 BLUE
  {255, 255,   0},  // 3 YELLOW
  {255,   0, 255},  // 4 MAGENTA
  {  0, 255, 255},  // 5 CYAN
};
static const uint8_t PALETTE_SIZE = sizeof(palette) / sizeof(palette[0]);

static uint8_t colorIndex = 0;
static bool screenOn = true;

const char* colorName(uint8_t i) {
  switch (i) {
    case 0: return "RED";
    case 1: return "GREEN";
    case 2: return "BLUE";
    case 3: return "YELLOW";
    case 4: return "MAGENTA";
    default: return "CYAN";
  }
}

void paintScreen() {
  if (screenOn) {
    esp.setColor(palette[colorIndex][0], palette[colorIndex][1], palette[colorIndex][2]);
    esp.clear();
    esp.print("Screen: ");
    esp.println(colorName(colorIndex));
  } else {
    esp.setColor(255, 255, 255);
    esp.clear();
    esp.println("Screen off");
  }
}

void onButton(ControlAction action) {
  switch (action) {
    case CTRL_UP:
    case CTRL_LEFT:
      colorIndex = (colorIndex + 1) % PALETTE_SIZE;
      break;
    case CTRL_DOWN:
    case CTRL_RIGHT:
      colorIndex = (colorIndex + PALETTE_SIZE - 1) % PALETTE_SIZE;
      break;
    case CTRL_OK:
      screenOn = !screenOn;
      break;
    default:
      return;   // BACK — без действия
  }
  paintScreen();
}

void onToggle(uint8_t index, bool state) {
  if (!state) return;
  esp.clear();
  if (index == 0) {
    // RGB Strip: перекрасить экран
    colorIndex = (colorIndex + 1) % PALETTE_SIZE;
    screenOn = true;
    paintScreen();
  } else {
    esp.setColor(128, 128, 128);
    esp.print("Scan Signal");
    esp.newline();
    esp.println("START");
  }
}

void setup() {
  Serial.begin(115200);

  esp.begin(Serial, "ESP32-Flight");   // транспорт + имя устройства

  // Кнопки пульта.
  esp.addButton(CTRL_UP);
  esp.addButton(CTRL_DOWN);
  esp.addButton(CTRL_LEFT);
  esp.addButton(CTRL_RIGHT);
  esp.addButton(CTRL_OK);
  esp.addButton(CTRL_BACK);

  // Тумблеры.
  esp.addToggle("RGB Strip");
  esp.addToggle("Scan Signal");

  esp.onButton(onButton);
  esp.onToggle(onToggle);

  esp.setBatteryVoltage(3.72);

  paintScreen();   // стартовый цвет — RED
}

void loop() {
  esp.loop();   // разбирает команды и шлёт изменения экрана
}