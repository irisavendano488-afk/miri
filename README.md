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
- **Транспорт:** `Serial`, **Bluetooth LE**, **Wi-Fi** (точка доступа ESP32 или подключение к роутеру). У каждого устройства свой `deviceName`, `password` и разрешение экрана.

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

## Настройка устройства (BLE / Wi-Fi)

Вместо `begin(Serial, …)` можно задать всё сразу через конфиг
(см. пример `04_MyDevice`):

```cpp
#include <FlightESP.h>

FlightESP esp;

FlightESPConfig cfg;              // значения по умолчанию уже заполнены
cfg.mode = FLIGHTESP_BLE;         // FLIGHTESP_SERIAL | BLE | WIFI_AP | WIFI_STA
cfg.deviceName = "QuadCopter";    // имя устройства (BLE/AP)
cfg.password    = "0000";         // пароль доступа (BLE AUTH); для WIFI_AP >= 8 симв.
cfg.screenWidth = 128;            // разрешение виртуального экрана
cfg.screenHeight = 64;
// для WIFI_AP дополнительно: cfg.port = 9000
// для WIFI_STA: cfg.wifiSsid = "HomeWiFi", cfg.wifiPassword = "pass"

void setup() {
  Serial.begin(115200);
  esp.begin(cfg);
  esp.addButton(CTRL_UP);
  esp.addButton(CTRL_OK);
  esp.addToggle("RGB Strip");
  esp.onButton(myButtonHandler);
  esp.setBatteryVoltage(3.70);
}

void loop() { esp.loop(); }
```

Что происходит на ESP32:
- **BLE:** у устройства сервис `11111111-a1b2-c3d4-e5f6-1234567890ab`
  (RX — write, TX — notify, AUTH — пароль `cfg.password`);
- **WIFI_AP:** ESP32 раздаёт точку доступа с SSID = `cfg.deviceName`
  и паролем `cfg.password` (WPA2, минимум 8 символов);
- **WIFI_STA:** ESP32 подключается к вашему роутеру и публикуется как
  Bonjour-сервис `_fligthesp._tcp`, порт `cfg.port` (по умолчанию 9000).

## Протокол (ESP32 ⇄ приложение)

Строковый, построчный: через `Serial`, характеристику BLE `TX` или TCP-соединение.

| Скетч → телефон                | Телефон → скетч          |
|--------------------------------|--------------------------|
| `INFO <имя>`                   | `PING`                   |
| `L <строка>` … `END`           | `BTN up\|down\|left\|right\|ok\|back` |
| `RES <w> <h>`                  | `TOG <индекс> <0\|1>`    |
| `BATT <вольты> <имя>`          | `AUTH <пароль>` (BLE)    |
| `COL <r> <g> <b> <имя>`        |                          |

Цвет экрана из скетча — `esp.setColor(r, g, b)`: приложение перекрашивает
виртуальный экран (см. пример `03_ColorScreen`).

## Лицензия

MIT — см. `LICENSE`.