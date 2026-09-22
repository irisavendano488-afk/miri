//
// FlightESP.cpp
//
#include "FlightESP.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>
#include <WiFiServer.h>
#include <ESPmDNS.h>

// ---- UUID сервиса и характеристик FlightESP ----
#define BLE_SERVICE_UUID "11111111-a1b2-c3d4-e5f6-1234567890ab"
#define BLE_RX_UUID      "11111111-a1b2-c3d4-e5f6-1234567890ac"
#define BLE_TX_UUID      "11111111-a1b2-c3d4-e5f6-1234567890ad"
#define BLE_AUTH_UUID    "11111111-a1b2-c3d4-e5f6-1234567890ae"
#endif

FlightESP::FlightESP()
    : _serial(nullptr),
      _buttonCount(0),
      _toggleCount(0),
      _screenCount(0),
      _screenDirty(true),
      _battery(0.0f),
      _dirty(true),
      _onButton(nullptr),
      _onToggle(nullptr),
      _inLen(0) {
    memset(_buttons, CTRL_NONE, sizeof(_buttons));
    memset(_toggles, 0, sizeof(_toggles));
    memset(_screen, 0, sizeof(_screen));
    memset(_inBuf, 0, sizeof(_inBuf));
#if defined(ARDUINO_ARCH_ESP32)
    memset(_wifiClients, 0, sizeof(_wifiClients));
    memset(_wifiUsed, 0, sizeof(_wifiUsed));
#endif
}

FlightESP::~FlightESP() {
#if defined(ARDUINO_ARCH_ESP32)
    for (size_t i = 0; i < MAX_WIFI_CLIENTS; i++) {
        delete reinterpret_cast<WiFiClient*>(_wifiClients[i]);
        _wifiClients[i] = nullptr;
    }
    delete reinterpret_cast<WiFiServer*>(_wifiServer);
    _wifiServer = nullptr;
#endif
}

// ---------------------------------------------------------------- begin

void FlightESP::begin(const FlightESPConfig& config) {
    _config = config;
    _dirty = true;
    _screenDirty = true;

    switch (_config.mode) {
        case FLIGHTESP_BLE:     startBle();     break;
        case FLIGHTESP_WIFI_AP: startWifiAP();  break;
        case FLIGHTESP_WIFI_STA: startWifiSTA(); break;
        case FLIGHTESP_SERIAL:
        default:
            // для SERIAL нужно использовать begin(Stream&, name)
            break;
    }
    sendInfo("INFO", _config.deviceName);
    sendResolution();
    sendBattery();
    sendScreen();
}

void FlightESP::begin(Stream& transport, const char* deviceName) {
    _config.mode = FLIGHTESP_SERIAL;
    _config.deviceName = deviceName;
    _config.password = "";
    _serial = &transport;
    _dirty = true;
    _screenDirty = true;

    sendInfo("INFO", _config.deviceName);
    sendResolution();
    sendBattery();
    sendScreen();
}

// ------------------------------------------------------------- transport

void FlightESP::sendLine(const char* line, bool newline) {
    if (!line) return;
    switch (_config.mode) {
        case FLIGHTESP_SERIAL:
            if (_serial) {
                _serial->print(line);
                if (newline) _serial->println();
            }
            break;
#if defined(ARDUINO_ARCH_ESP32)
        case FLIGHTESP_BLE:
            if (_charTX) {
                String data(line);
                if (newline) data += '\n';
                _charTX->setValue(data);
                _charTX->notify();
            }
            break;
        case FLIGHTESP_WIFI_AP:
        case FLIGHTESP_WIFI_STA:
            sendToWifiClients(line, newline);
            break;
#endif
        default:
            break;
    }
}

void FlightESP::sendInfo(const char* key, const char* value) {
    if (!key || !value) return;
    char buf[MAX_LINE_LEN * 2];
    snprintf(buf, sizeof(buf), "%s %s", key, value);
    sendLine(buf, true);
}

void FlightESP::sendResolution() {
    char buf[32];
    snprintf(buf, sizeof(buf), "RES %u %u", _config.screenWidth, _config.screenHeight);
    sendLine(buf, true);
}

// ------------------------------------------------------------ controls

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

// --------------------------------------------------------------- screen

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
    size_t wrote = 0;
    if (_screenCount == 0) {
        _screen[0][0] = '\0';
        _screenCount = 1;
    }
    while (*text) {
        char c = *text++;
        if (c == '\n') {
            newline();
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

void FlightESP::println() { newline(); }

void FlightESP::println(const char* text) { print(text); newline(); }

void FlightESP::println(char c) { print(c); newline(); }

void FlightESP::clear() {
    memset(_screen, 0, sizeof(_screen));
    _screenCount = 0;
    _screenDirty = true;
}

void FlightESP::sendScreen(bool force) {
    if (!force && !_screenDirty) return;
    char buf[2 + MAX_LINE_LEN];
    for (uint8_t i = 0; i < MAX_SCREEN_LINES; i++) {
        snprintf(buf, sizeof(buf), "L %s", _screen[i]);
        sendLine(buf, true);
    }
    sendLine("END", true);
    _screenDirty = false;
}

void FlightESP::sendBattery() {
    char buf[40];
    snprintf(buf, sizeof(buf), "BATT %.2f %s", _battery, _config.deviceName);
    sendLine(buf, true);
}

void FlightESP::setColor(uint8_t r, uint8_t g, uint8_t b) {
    char buf[40];
    snprintf(buf, sizeof(buf), "COL %u %u %u %s", (unsigned)r, (unsigned)g, (unsigned)b,
             _config.deviceName);
    sendLine(buf, true);
}

// ------------------------------------------------------------- events

void FlightESP::fireButton(ControlAction action) {
    if (_onButton) _onButton(action);
}

void FlightESP::fireToggle(uint8_t index, bool state) {
    if (index >= _toggleCount) return;
    _toggles[index].state = state;
    if (_onToggle) _onToggle(index, state);
}

// ------------------------------------------------------------ protocol

// Команды приложение -> ESP32:
//   PING
//   BTN <action>          action: up|down|left|right|ok|back
//   TOG <index> <0|1>
// ESP32 -> приложение:
//   INFO <deviceName>
//   RES <width> <height>
//   L <line> ... END
//   BATT <volts> <deviceName>
//   COL <r> <g> <b> <deviceName>

void FlightESP::parseCommand() {
    if (_inLen == 0) return;
    _inBuf[_inLen] = '\0';

    char* cmd = _inBuf;

    if (strcmp(cmd, "PING") == 0) {
        sendInfo("PONG", _config.deviceName);
        sendInfo("INFO", _config.deviceName);
        sendResolution();
        sendBattery();
        sendScreen(true);
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
}

void FlightESP::feedByte(char c) {
    if (c == '\n' || c == '\r') {
        if (_inLen > 0) parseCommand();
        return;
    }
    if (_inLen < sizeof(_inBuf) - 1) {
        _inBuf[_inLen++] = c;
    } else {
        _inLen = 0;
    }
}

// ------------------------------------------------------- BLE transport

#if defined(ARDUINO_ARCH_ESP32)

class FlightESPServerCallbacks : public BLEServerCallbacks {
public:
    explicit FlightESPServerCallbacks(FlightESP* esp) : _esp(esp) {}
    void onConnect(BLEServer* server) override {
        (void)server;
        if (_esp) _esp->onBleConnect();
    }
    void onDisconnect(BLEServer* server) override {
        (void)server;
        if (_esp) _esp->onBleDisconnect();
    }
private:
    FlightESP* _esp;
};

class FlightESPRxCallback : public BLECharacteristicCallbacks {
public:
    explicit FlightESPRxCallback(FlightESP* esp) : _esp(esp) {}
    void onWrite(BLECharacteristic* characteristic) override {
        if (!_esp || !_esp->isAuthed()) return;
        String val = characteristic->getValue();
        for (size_t i = 0; i < val.length(); i++) {
            _esp->feedByte(val[i]);
        }
    }
private:
    FlightESP* _esp;
};

class FlightESPAuthCallback : public BLECharacteristicCallbacks {
public:
    explicit FlightESPAuthCallback(FlightESP* esp) : _esp(esp) {}
    void onWrite(BLECharacteristic* characteristic) override {
        if (!_esp) return;
        String val = characteristic->getValue();
        _esp->authBle(val.c_str());
    }
private:
    FlightESP* _esp;
};

void FlightESP::startBle() {
    BLEDevice::init(_config.deviceName);
    _bleServer = BLEDevice::createServer();
    _bleServer->setCallbacks(new FlightESPServerCallbacks(this));

    BLEService* service = _bleServer->createService(BLE_SERVICE_UUID);

    _charRX = service->createCharacteristic(
        BLE_RX_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
    _charRX->setCallbacks(new FlightESPRxCallback(this));

    _charTX = service->createCharacteristic(
        BLE_TX_UUID,
        BLECharacteristic::PROPERTY_NOTIFY);
    _charTX->addDescriptor(new BLE2902());

    _charAuth = service->createCharacteristic(
        BLE_AUTH_UUID,
        BLECharacteristic::PROPERTY_WRITE);
    _charAuth->setCallbacks(new FlightESPAuthCallback(this));

    service->start();

    BLEAdvertising* adv = _bleServer->getAdvertising();
    adv->addServiceUUID(BLE_SERVICE_UUID);
    adv->setScanResponse(true);
    adv->start();

    _bleAuthOk = (strlen(_config.password) == 0);
}

void FlightESP::onBleConnect() {
    // Новый клиент подписался на TX — отправляем текущее состояние.
    sendInfo("INFO", _config.deviceName);
    sendResolution();
    sendBattery();
    sendScreen();
}

void FlightESP::onBleDisconnect() {
    _bleAuthOk = false;
    if (_bleServer) {
        _bleServer->getAdvertising()->start();  // снова доступен для поиска
    }
}

void FlightESP::authBle(const char* password) {
    if (strlen(_config.password) == 0) {
        _bleAuthOk = true;
        return;
    }
    _bleAuthOk = (strcmp(password ? password : "", _config.password) == 0);
    sendInfo(_bleAuthOk ? "AUTH OK" : "AUTH FAIL", _config.deviceName);
}

// ------------------------------------------------------ WiFi transport

static FlightESPConfig defaultWifiPassword(FlightESPConfig config) {
    // WPA2 требует минимум 8 символов.
    if (config.password && strlen(config.password) >= 8) {
        return config;
    }
    config.password = "fligthesp";   // безопасное значение по умолчанию
    return config;
}

void FlightESP::startWifiAP() {
    _config = defaultWifiPassword(_config);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(_config.deviceName, _config.password);  // SSID = имя устройства

    _wifiServer = new WiFiServer(_config.port);
    reinterpret_cast<WiFiServer*>(_wifiServer)->begin();
    for (size_t i = 0; i < MAX_WIFI_CLIENTS; i++) {
        _wifiClients[i] = new WiFiClient();
        _wifiUsed[i] = false;
    }
}

void FlightESP::startWifiSTA() {
    WiFi.mode(WIFI_STA);
    if (_config.wifiSsid && strlen(_config.wifiSsid)) {
        WiFi.begin(_config.wifiSsid,
                   _config.wifiPassword ? _config.wifiPassword : "");
    }
    // Bonjour: телефон ищет устройство по _fligthesp._tcp
    if (_config.deviceName) {
        MDNS.begin(_config.deviceName);
        MDNS.addService("fligthesp", "tcp", _config.port);
    }

    _wifiServer = new WiFiServer(_config.port);
    reinterpret_cast<WiFiServer*>(_wifiServer)->begin();
    for (size_t i = 0; i < MAX_WIFI_CLIENTS; i++) {
        _wifiClients[i] = new WiFiClient();
        _wifiUsed[i] = false;
    }
}

void FlightESP::sendToWifiClients(const char* line, bool newline) {
    for (size_t i = 0; i < MAX_WIFI_CLIENTS; i++) {
        if (_wifiUsed[i] && reinterpret_cast<WiFiClient*>(_wifiClients[i])->connected()) {
            reinterpret_cast<WiFiClient*>(_wifiClients[i])->print(line);
            if (newline) reinterpret_cast<WiFiClient*>(_wifiClients[i])->println();
        }
    }
}

void FlightESP::greetWifiClient(void* raw) {
    WiFiClient* client = reinterpret_cast<WiFiClient*>(raw);
    if (!client || !client->connected()) return;
    char buf[MAX_LINE_LEN * 2];
    snprintf(buf, sizeof(buf), "INFO %s", _config.deviceName);
    client->println(buf);
    snprintf(buf, sizeof(buf), "RES %u %u", _config.screenWidth, _config.screenHeight);
    client->println(buf);
    snprintf(buf, sizeof(buf), "BATT %.2f %s", _battery, _config.deviceName);
    client->println(buf);
    for (uint8_t i = 0; i < MAX_SCREEN_LINES; i++) {
        snprintf(buf, sizeof(buf), "L %s", _screen[i]);
        client->println(buf);
    }
    client->println("END");
}

void FlightESP::pumpWifiClients() {
    WiFiServer* server = reinterpret_cast<WiFiServer*>(_wifiServer);
    if (!server) return;

    // принимаем новых клиентов
    if (server->hasClient()) {
        WiFiClient newClient = server->available();
        bool accepted = false;
        for (size_t i = 0; i < MAX_WIFI_CLIENTS; i++) {
            if (!_wifiUsed[i]) {
                WiFiClient* slot = reinterpret_cast<WiFiClient*>(_wifiClients[i]);
                slot->stop();
                *slot = newClient;
                _wifiUsed[i] = true;
                accepted = true;
                greetWifiClient(slot);
                break;
            }
        }
        if (!accepted) {
            newClient.stop();
        }
    }

    // читаем данные от всех клиентов
    for (size_t i = 0; i < MAX_WIFI_CLIENTS; i++) {
        if (!_wifiUsed[i]) continue;
        WiFiClient* slot = reinterpret_cast<WiFiClient*>(_wifiClients[i]);
        if (!slot->connected()) {
            slot->stop();
            _wifiUsed[i] = false;
            continue;
        }
        while (slot->available()) {
            feedByte((char)slot->read());
        }
    }
}

#endif  // ARDUINO_ARCH_ESP32

// -------------------------------------------------------------- service

void FlightESP::loop() {
    switch (_config.mode) {
        case FLIGHTESP_SERIAL:
            if (_serial) {
                while (_serial->available()) {
                    feedByte((char)_serial->read());
                }
            }
            break;
#if defined(ARDUINO_ARCH_ESP32)
        case FLIGHTESP_BLE:
            break;   // ввод приходит через колбэки характеристик
        case FLIGHTESP_WIFI_AP:
        case FLIGHTESP_WIFI_STA:
            pumpWifiClients();
            break;
#endif
        default:
            break;
    }

    if (_screenDirty) sendScreen();
    if (_dirty) {
        sendBattery();
        _dirty = false;
    }
}