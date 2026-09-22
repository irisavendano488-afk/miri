// FlightESP — пример 04: своё устройство через конфиг.
//
// Здесь настраиваются имя устройства, пароль, разрешение виртуального
// экрана и транспорт (Bluetooth LE или Wi-Fi).
//
//   mode = FLIGHTESP_BLE     — телефон находит устройство по Bluetooth
//   mode = FLIGHTESP_WIFI_AP — ESP32 раздаёт точку доступа (hotspot)
//   mode = FLIGHTESP_WIFI_STA— ESP32 подключается к вашему роутеру и
//                              публикуется как _fligthesp._tcp
//
// Установка: Скетч → Подключить библиотеку → Добавить .ZIP библиотеку…
// (файл FlightESP-1.1.0.zip). Затем Скетч → Проверка/Компиляция, залить.

#include <FlightESP.h>

FlightESP esp;

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
  esp.println("Hello from my device!");
}

FlightESPConfig cfg;

void setup() {
  Serial.begin(115200);

  // --- Настройки устройства ---
  cfg.mode         = FLIGHTESP_BLE;          // BLE / WIFI_AP / WIFI_STA
  cfg.deviceName   = "QuadCopter";           // имя устройства / точка доступа
  cfg.password     = "0000";                 // пароль (BLE); для WIFI_AP >= 8 символов
  cfg.screenWidth  = 128;                    // разрешение экрана
  cfg.screenHeight =  64;

  // WiFi (необязательно, для WIFI_AP / WIFI_STA):
  //   cfg.port         = 9000;
  // WIFI_STA требует, чтобы ESP32 подключился к роутеру:
  //   cfg.wifiSsid     = "MyRouter";
  //   cfg.wifiPassword = "router-pass";

  esp.begin(cfg);

  esp.addButton(CTRL_UP);
  esp.addButton(CTRL_DOWN);
  esp.addButton(CTRL_LEFT);
  esp.addButton(CTRL_RIGHT);
  esp.addButton(CTRL_OK);
  esp.addButton(CTRL_BACK);

  esp.addToggle("RGB Strip");
  esp.addToggle("Scan Signal");

  esp.onButton(onButton);

  esp.setBatteryVoltage(3.70);
  esp.print("FlightESP v");
  esp.print(FLIGHTESP_VERSION);
  esp.println(" ready!");
}

void loop() {
  esp.loop();   // обязательно: принимает команды и шлёт экран
}