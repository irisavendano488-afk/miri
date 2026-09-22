//
// BasicScreen — минимальный пример.
// Виртуальный экран + кнопки пульта.
//
// Прошиваете ESP32, открываете Serial Monitor (115200 baud)
// и видите тот же экран, который показывает приложение FlightESP.
//

#include <FlightESP.h>

FlightESP esp;

// Нажатие кнопки на телефоне.
void onButton(ControlAction action) {
  esp.clear();
  esp.print("Pressed: ");
  switch (action) {
    case CTRL_UP:    esp.println("UP");    break;
    case CTRL_DOWN:  esp.println("DOWN");  break;
    case CTRL_LEFT:  esp.println("LEFT");  break;
    case CTRL_RIGHT: esp.println("RIGHT"); break;
    case CTRL_OK:    esp.println("OK");    break;
    case CTRL_BACK:  esp.println("BACK");  break;
    default:         esp.println("?");     break;
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

  esp.onButton(onButton);

  esp.setBatteryVoltage(3.72);          // напряжение батареи (необязательно)

  esp.print("FlightESP v1.0");
  esp.newline();
  esp.print("Ready!");
}

void loop() {
  esp.loop();   // разбирает команды, шлёт изменения экрана
}