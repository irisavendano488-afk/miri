//
// RGBScanner — тумблеры и действия.
// Тумблеры включают RGB-ленту (D0-D2) и режим "Scan Signal" (D3).
// Подключите светодиоды/LED-ленту к пинам 0, 2, 4.
//

#include <FlightESP.h>

const uint8_t PIN_RGB_R = 0;
const uint8_t PIN_RGB_G = 2;
const uint8_t PIN_RGB_B = 4;
const uint8_t PIN_SCAN  = 5;

FlightESP esp;

void applyRGB(bool on) {
  digitalWrite(PIN_RGB_R, on ? HIGH : LOW);
  digitalWrite(PIN_RGB_G, on ? HIGH : LOW);
  digitalWrite(PIN_RGB_B, on ? HIGH : LOW);
}

// Переключение тумблера на телефоне: index — номер тумблера, state — вкл/выкл.
void onToggle(uint8_t index, bool state) {
  esp.clear();
  switch (index) {
    case 0: // RGB Strip
      applyRGB(state);
      esp.print("RGB: ");
      esp.println(state ? "ON" : "OFF");
      break;
    case 1: // Scan Signal
      digitalWrite(PIN_SCAN, state ? HIGH : LOW);
      esp.print("Scan: ");
      esp.println(state ? "START" : "STOP");
      break;
    case 2: // Buzzer
      esp.print("Buzzer: ");
      esp.println(state ? "ON" : "OFF");
      break;
  }
  esp.setBatteryVoltage(3.72);
}

void setup() {
  pinMode(PIN_RGB_R, OUTPUT);
  pinMode(PIN_RGB_G, OUTPUT);
  pinMode(PIN_RGB_B, OUTPUT);
  pinMode(PIN_SCAN,  OUTPUT);

  Serial.begin(115200);

  esp.begin(Serial, "esp32-wifi-7C9E");

  esp.addToggle("RGB Strip");
  esp.addToggle("Scan Signal");
  esp.addToggle("Buzzer");

  esp.onToggle(onToggle);

  esp.print("RGBScanner");
  esp.newline();
  esp.print("Ready!");
}

void loop() {
  esp.loop();
  delay(10);
}