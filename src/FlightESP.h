//
// FlightESP.h
// FlightESP — виртуальный экран и пульт ESP32 для iOS-приложения.
//
// Лицензия: MIT. См. LICENSE.
//

#ifndef FlightESP_h
#define FlightESP_h

#include <Arduino.h>

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

// Типы контролов, отправляемые приложению.
enum ControlKind : uint8_t {
    KIND_BUTTON = 0,
    KIND_TOGGLE = 1,
};

class FlightESP {
public:
    typedef void (*ButtonCallback)(ControlAction action);
    typedef void (*ToggleCallback)(uint8_t index, bool state);

    static const size_t MAX_BUTTONS  = 12;
    static const size_t MAX_TOGGLES  = 12;
    static const size_t MAX_SCREEN_LINES = 4;
    static const size_t MAX_LINE_LEN = 32;

    FlightESP();

    // Подключить транспорт: Serial, BluetoothSerial (ESP32) или любой Stream.
    void begin(Stream& transport, const char* deviceName = "ESP32-Flight");

    // --- Конфигурация пульта (всё добавляется в коде) ---

    // Дирекциональная кнопка: up / down / left / right / ok / back.
    void addButton(ControlAction action);

    // Тумблер: выключатель функции (например "RGB Strip", "Scan Signal").
    void addToggle(const char* title);

    // --- Колбэки от приложения ---
    void onButton(ButtonCallback cb) { _onButton = cb; }
    void onToggle(ToggleCallback cb) { _onToggle = cb; }

    // --- Пульт ---
    void setBatteryVoltage(float volts) { _battery = volts; _dirty = true; }

    int  buttonCount() const { return _buttonCount; }
    int  toggleCount() const { return _toggleCount; }
    bool toggleState(uint8_t index) const {
        return index < _toggleCount ? _toggles[index].state : false;
    }

    // --- Виртуальный экран (Print-совместимый) ---
    // Всё, что печатается, приложение показывает на своём экране.
    size_t print(const char* text);
    size_t print(char c);
    void println();                       // перенос строки
    void println(const char* text);       // текст + перенос строки
    void println(char c);                 // символ + перенос строки

    void newline();   // перенос строки (движет строки вверх)
    void clear();     // очистить экран
    void sendScreen();            // принудительно отправить экран сейчас
    void sendBattery();           // отправить напряжение сейчас

    // --- Сервис ---
    // Вызывать в loop(). Читает входящие команды, отправляет изменения.
    void loop();

private:
    struct Toggle {
        char title[MAX_LINE_LEN];
        bool state;
    };

    Stream*   _transport;
    char      _deviceName[24];
    uint8_t   _buttons[MAX_BUTTONS];
    uint8_t   _buttonCount;

    Toggle    _toggles[MAX_TOGGLES];
    uint8_t   _toggleCount;

    char      _screen[MAX_SCREEN_LINES][MAX_LINE_LEN];
    uint8_t   _screenCount;
    bool      _screenDirty;

    float     _battery;
    bool      _dirty;

    ButtonCallback _onButton;
    ToggleCallback _onToggle;

    // буфер ввода построчного протокола
    char      _inBuf[32];
    uint8_t   _inLen;

    void readCommands();
    void sendInfo(const __FlashStringHelper* key, const char* value);
    void fireButton(ControlAction action);
    void fireToggle(uint8_t index, bool state);
};

#endif /* FlightESP_h */