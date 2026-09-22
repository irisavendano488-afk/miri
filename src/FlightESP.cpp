//
// FlightESP.cpp
//

#include "FlightESP.h"

#if defined(ARDUINO_ARCH_ESP32)
#define FLIGTHESP_PRINT_ARGV_AVAILABLE 1
#endif

FlightESP::FlightESP()
    : _transport(nullptr),
      _buttonCount(0),
      _toggleCount(0),
      _screenCount(0),
      _screenDirty(true),
      _battery(0.0f),
      _dirty(true),
      _onButton(nullptr),
      _onToggle(nullptr),
      _inLen(0) {
    memset(_deviceName, 0, sizeof(_deviceName));
    memset(_buttons, CTRL_NONE, sizeof(_buttons));
    memset(_toggles, 0, sizeof(_toggles));
    memset(_screen, 0, sizeof(_screen));
    memset(_inBuf, 0, sizeof(_inBuf));
}

static inline bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

void FlightESP::begin(Stream& transport, const char* deviceName) {
    _transport = &transport;
    if (deviceName) {
        strncpy(_deviceName, deviceName, sizeof(_deviceName) - 1);
    }
    if (strlen(_deviceName) == 0) {
        strncpy(_deviceName, "ESP32-Flight", sizeof(_deviceName) - 1);
    }
    sendInfo(F("INFO"), _deviceName);
}

// --- Конфигурация ---

void FlightESP::addButton(ControlAction action) {
    if (_buttonCount >= MAX_BUTTONS) return;
    if (action == CTRL_NONE || action > CTRL_BACK) return;
    _buttons[_buttonCount++] = action;
    _dirty = true;
}

void FlightESP::addToggle(const char* title) {
    if (_toggleCount >= MAX_TOGGLES) return;
    if (!title) return;
    strncpy(_toggles[_toggleCount].title, title, MAX_LINE_LEN - 1);
    _toggles[_toggleCount].title[MAX_LINE_LEN - 1] = '\0';
    _toggles[_toggleCount].state = false;
    _toggleCount++;
    _dirty = true;
}

// --- Виртуальный экран ---

void FlightESP::newline() {
    for (uint8_t i = 1; i < MAX_SCREEN_LINES; i++) {
        memcpy(_screen[i - 1], _screen[i], MAX_LINE_LEN);
    }
    _screen[MAX_SCREEN_LINES - 1][0] = '\0';
    _screenCount = MAX_SCREEN_LINES;
    _screenDirty = true;
}

size_t FlightESP::print(const char* text) {
    if (!text) return 0;
    uint8_t line = (_screenCount > 0) ? (_screenCount - 1) : 0;
    size_t  wrote = 0;
    if (_screenCount == 0) {
        _screen[0][0] = '\0';
        _screenCount = 1;
    }
    while (*text) {
        char c = *text++;
        if (c == '\n') {
            newline();  // сдвигает строки, последняя становится пустой
            line = MAX_SCREEN_LINES - 1;
            wrote++;
            continue;
        }
        size_t len = strlen(_screen[line]);
        if (len >= MAX_LINE_LEN - 1) {
            newline();
            line = MAX_SCREEN_LINES - 1;
        }
        _screen[line][len] = c;
        _screen[line][len + 1] = '\0';
        wrote++;
    }
    _screenDirty = true;
    return wrote;
}

size_t FlightESP::print(char c) {
    char buf[2] = { c, '\0' };
    return print((const char*)buf);
}

void FlightESP::println() {
    newline();
}

void FlightESP::println(const char* text) {
    print(text);
    newline();
}

void FlightESP::println(char c) {
    print(c);
    newline();
}

void FlightESP::clear() {
    memset(_screen, 0, sizeof(_screen));
    _screenCount = 0;
    _screenDirty = true;
}

void FlightESP::sendScreen() {
    if (!_transport || !_screenDirty) return;
    _transport->print(F("DISP "));
    _transport->print(_deviceName);
    _transport->println();
    for (uint8_t i = 0; i < MAX_SCREEN_LINES; i++) {
        _transport->print(F("L "));
        _transport->println(_screen[i]);
    }
    _transport->println(F("END"));
    _screenDirty = false;
}

void FlightESP::setColor(uint8_t r, uint8_t g, uint8_t b) {
    if (!_transport) return;
    _transport->print(F("COL "));
    _transport->print((unsigned int)r);
    _transport->print(' ');
    _transport->print((unsigned int)g);
    _transport->print(' ');
    _transport->print((unsigned int)b);
    _transport->print(' ');
    _transport->println(_deviceName);
}

void FlightESP::sendBattery() {
    if (!_transport) return;
    _transport->print(F("BATT "));
    _transport->print(_battery, 2);
    _transport->print(' ');
    _transport->println(_deviceName);
}

void FlightESP::sendInfo(const __FlashStringHelper* key, const char* value) {
    if (!_transport) return;
    _transport->print(key);
    _transport->print(' ');
    _transport->println(value);
}

// --- События ---

void FlightESP::fireButton(ControlAction action) {
    if (_onButton) _onButton(action);
}

void FlightESP::fireToggle(uint8_t index, bool state) {
    if (index >= _toggleCount) return;
    _toggles[index].state = state;
    if (_onToggle) _onToggle(index, state);
}

// --- Протокол ---

// Команды приложения -> ESP32:
//   PING
//   BTN <action>          action: up|down|left|right|ok|back
//   TOG <index> <0|1>
// ESP32 -> приложение:
//   INFO <deviceName>
//   DISP <deviceName>   ... фрейм экрана
//   L <line>
//   END
//   BATT <volts> <deviceName>

void FlightESP::readCommands() {
    while (_transport->available()) {
        char c = (char)_transport->read();
        if (c == '\n' || c == '\r') {
            if (_inLen == 0) continue;
            _inBuf[_inLen] = '\0';

            char* cmd = _inBuf;

            if (strcmp(cmd, "PING") == 0) {
                sendInfo(F("PONG"), _deviceName);
            } else if (strncmp(cmd, "BTN ", 4) == 0) {
                const char* action = cmd + 4;
                ControlAction a = CTRL_NONE;
                if (strcmp(action, "up") == 0) a = CTRL_UP;
                else if (strcmp(action, "down") == 0) a = CTRL_DOWN;
                else if (strcmp(action, "left") == 0) a = CTRL_LEFT;
                else if (strcmp(action, "right") == 0) a = CTRL_RIGHT;
                else if (strcmp(action, "ok") == 0) a = CTRL_OK;
                else if (strcmp(action, "back") == 0) a = CTRL_BACK;
                if (a != CTRL_NONE) fireButton(a);
            } else if (strncmp(cmd, "TOG ", 4) == 0) {
                char* p = cmd + 4;
                int index = atoi(p);
                while (*p && *p != ' ') p++;
                int state = (*p == ' ') ? atoi(p + 1) : 0;
                if (index >= 0 && index < _toggleCount) {
                    fireToggle((uint8_t)index, state != 0);
                }
            }
            _inLen = 0;
        } else {
            if (_inLen < sizeof(_inBuf) - 1) {
                _inBuf[_inLen++] = c;
            } else {
                _inLen = 0;  // слишком длинная строка — сброс
            }
        }
    }
}

void FlightESP::loop() {
    if (!_transport) return;
    readCommands();
    if (_screenDirty) sendScreen();
    if (_dirty) {
        sendInfo(F("CTRL"), _deviceName);
        _dirty = false;
    }
}