//
// FlightESP.h
// FlightESP — виртуальный экран и пульт ESP32 с реальным подключением к iOS.
//
// Режимы подключения:
//   FLIGHTESP_SERIAL   — любой Stream (Serial / Bluetooth classic)
//   FLIGHTESP_BLE      — BLE-сервер (реально работает с iPhone)
//   FLIGHTESP_WIFI_AP  — ESP32 создаёт точку доступа + TCP-сервер
//   FLIGHTESP_WIFI_STA — ESP32 в сети роутера + Bonjour (mDNS)
//
// Лицензия: MIT. См. LICENSE.
//

#ifndef FlightESP_h
#define FlightESP_h

#include <Arduino.h>
#include <stdint.h>

#if defined(ARDUINO_ARCH_ESP32)
class BLECharacteristic;
class BLEServer;
class BLEAdvertising;
// WiFiServer / WiFiClient в ESP32-ядре 3.x — это typedef (NetworkServer /
// NetworkClient), поэтому forward-объявление невозможно: храним их как void*,
// а кастим в .cpp, где включены полные заголовки WiFi.h.
#endif

#define FLIGHTESP_VERSION "1.1.0"

// Действия кнопок, которые понимает приложение.
enum ControlAction : uint8_t {
    CTRL_NONE  = 0,
    CTRL_UP    = 1,  // вверх
    CTRL_DOWN  = 2,  // вниз
    CTRL_LEFT  = 3,  // влево
    CTRL_RIGHT = 4,  // вправо
    CTRL_OK    = 5,  // OK
    CTRL_BACK  = 6,  // назад
};

// Типы контролов.
enum ControlKind : uint8_t {
    KIND_BUTTON = 0,
    KIND_TOGGLE = 1,
};

// Режим подключения.
enum FlightESPMode : uint8_t {
    FLIGHTESP_SERIAL  = 0,
    FLIGHTESP_BLE     = 1,
    FLIGHTESP_WIFI_AP = 2,
    FLIGHTESP_WIFI_STA = 3,
};

// Конфигурация устройства. Задаётся в коде перед esp.begin(config).
struct FlightESPConfig {
    FlightESPMode mode        = FLIGHTESP_BLE;
    const char*   deviceName  = "ESP32-Flight";  // название устройства
    const char*   password    = "0000";          // пароль (WPA2 для AP, auth для BLE)
    uint16_t      screenWidth = 128;             // разрешение виртуального экрана
    uint16_t      screenHeight = 64;
    uint16_t      port        = 9000;            // TCP-порт для Wi-Fi режимов
    const char*   wifiSsid    = nullptr;         // роутер (только WIFI_STA)
    const char*   wifiPassword = nullptr;        // пароль роутера (только WIFI_STA)
};

class FlightESP {
public:
    typedef void (*ButtonCallback)(ControlAction action);
    typedef void (*ToggleCallback)(uint8_t index, bool state);

    static const size_t MAX_BUTTONS      = 12;
    static const size_t MAX_TOGGLES      = 12;
    static const size_t MAX_SCREEN_LINES = 4;
    static const size_t MAX_LINE_LEN     = 32;
    static const size_t MAX_WIFI_CLIENTS = 4;

    FlightESP();
    ~FlightESP();

    // Основной способ запуска — через конфиг (имя, пароль, разрешение, режим).
    void begin(const FlightESPConfig& config);

    // Legacy: транспорт вручную (Serial / Bluetooth classic).
    void begin(Stream& transport, const char* deviceName = "ESP32-Flight");

    // --- Конфигурация пульта (всё добавляется в коде) ---
    void addButton(ControlAction action);
    void addToggle(const char* title);

    // --- Колбэки от приложения ---
    void onButton(ButtonCallback cb) { _onButton = cb; }
    void onToggle(ToggleCallback cb)  { _onToggle = cb; }

    // --- Пульт ---
    void setBatteryVoltage(float volts) { _battery = volts; _dirty = true; }
    void setColor(uint8_t r, uint8_t g, uint8_t b);   // цвет экрана в приложении

    int  buttonCount() const { return _buttonCount; }
    int  toggleCount() const { return _toggleCount; }
    bool toggleState(uint8_t index) const {
        return index < _toggleCount ? _toggles[index].state : false;
    }

    // --- Виртуальный экран ---
    size_t print(const char* text);
    size_t print(char c);
    void println();                        // перенос строки
    void println(const char* text);        // текст + перенос строки
    void println(char c);                  // символ + перенос строки

    void newline();                        // перенос строки (движет строки вверх)
    void clear();                          // очистить экран

    void sendScreen();                     // принудительно отправить экран
    void sendBattery();                    // отправить напряжение
    void sendResolution();                 // отправить разрешение экрана

    // --- Сервис: вызывать в loop() ---
    void loop();

    // Внутренние точки входа из транспортов (BLE/Wi-Fi).
    void feedByte(char c);                 // одна принятая команда
    bool isAuthed() const { return _bleAuthOk || strlen(_config.password) == 0; }
    void onBleConnect();
    void onBleDisconnect();
    void authBle(const char* password);

private:
    struct Toggle {
        char title[MAX_LINE_LEN];
        bool state;
    };

    FlightESPConfig _config;
    Stream*         _serial = nullptr;

    uint8_t         _buttons[MAX_BUTTONS];
    uint8_t         _buttonCount;

    Toggle          _toggles[MAX_TOGGLES];
    uint8_t         _toggleCount;

    char            _screen[MAX_SCREEN_LINES][MAX_LINE_LEN];
    uint8_t         _screenCount;
    bool            _screenDirty;

    float           _battery;
    bool            _dirty;

    ButtonCallback  _onButton;
    ToggleCallback  _onToggle;

    char            _inBuf[40];
    uint8_t         _inLen;
    bool            _bleAuthOk = false;

#if defined(ARDUINO_ARCH_ESP32)
    BLECharacteristic* _charRX = nullptr;
    BLECharacteristic* _charTX = nullptr;
    BLECharacteristic* _charAuth = nullptr;
    BLEServer*         _bleServer = nullptr;
    void*              _wifiServer = nullptr;
    void*              _wifiClients[MAX_WIFI_CLIENTS];
    bool               _wifiUsed[MAX_WIFI_CLIENTS];
#endif

    void sendLine(const char* line, bool newline);
    void sendInfo(const char* key, const char* value);
    void parseCommand();
    void fireButton(ControlAction action);
    void fireToggle(uint8_t index, bool state);
    void startBle();
    void startWifiAP();
    void startWifiSTA();
    void pumpWifiClients();
    void sendToWifiClients(const char* line, bool newline);
#if defined(ARDUINO_ARCH_ESP32)
    void greetWifiClient(void* client);
#endif
};

#endif /* FlightESP_h */