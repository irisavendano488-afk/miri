# FlightESP — библиотека для ESP32

Библиотека для **Arduino IDE / PlatformIO / ESP-IDF (Arduino-ядро)**.
Работает вместе с iOS-приложением **FlightESP**: телефон превращается в
виртуальный экран и пульт вашего ESP32.

## Возможности

- **Виртуальный экран.** Всё, что вы `print()`ите из скетча, приложение
  показывает на своём экране, как на физическом дисплее;
- **Кнопки.** `up / down / left / right / OK / back` — добавляются в одну строку кода;
- **Тумблеры.** Включают/выключают функции: RGB-лента, **scan signal**, зуммер и т.д.;
- **Данные устройства.** Напряжение батареи в вольтах, имя устройства;
- **Транспорт:** обычный `Serial`, Bluetooth (классический) или любой `Stream`.

## Установка в Arduino IDE

1. Скачайте ZIP: **Code → Download ZIP** (или `FlightESP-1.0.0.zip` из релизов).
2. В Arduino IDE: **Скетч → Подключить библиотеку → Добавить .ZIP библиотеку…**.
3. Выберите скачанный архив. Готово — появится пункт `FlightESP`.

## Быстрый старт

Откройте `Файл → Примеры → FlightESP → 01_BasicScreen`, загрузите на ESP32 и
откройте Serial Monitor (115200 baud) — увидите тот же экран, что и в приложении.

```cpp
#include <FlightESP.h>

FlightESP esp;

void onButton(ControlAction action) {
  esp.clear();
  esp.print("Pressed: ");
  esp.println(action == CTRL_UP ? "UP"
              : action == CTRL_DOWN ? "DOWN" : "OK/BACK");
}

void setup() {
  Serial.begin(115200);
  esp.begin(Serial, "ESP32-Flight");

  esp.addButton(CTRL_UP);
  esp.addButton(CTRL_DOWN);
  esp.addButton(CTRL_LEFT);
  esp.addButton(CTRL_RIGHT);
  esp.addButton(CTRL_OK);
  esp.addButton(CTRL_BACK);
  esp.onButton(onButton);

  esp.setBatteryVoltage(3.72);   // напряжение батареи
  esp.print("Ready!");
}

void loop() {
  esp.loop();   // обязательно!
}
```

## Тумблеры

```cpp
esp.addToggle("RGB Strip");
esp.addToggle("Scan Signal");

void onToggle(uint8_t index, bool state) {
  // index: 0 = "RGB Strip", 1 = "Scan Signal"
}
```

## Протокол (ESP32 ⇄ приложение)

Строковый, построчный, через `Stream`:

| Скетч → телефон                | Телефон → скетч          |
|--------------------------------|--------------------------|
| `INFO <имя>`                   | `PING`                   |
| `DISP <имя>` / `L <строка>` / `END` | `BTN up\|down\|left\|right\|ok\|back` |
| `BATT <вольты> <имя>`          | `TOG <индекс> <0\|1>`    |

iOS-адаптер BLE/Wi-Fi появится в приложении вслед за протоколом.

## Лицензия

MIT — см. `LICENSE`.