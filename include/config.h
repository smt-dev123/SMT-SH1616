#ifndef CONFIG_H
#define CONFIG_H

#define BLYNK_TEMPLATE_ID MY_BLYNK_TEMPLATE_ID
#define BLYNK_TEMPLATE_NAME MY_BLYNK_TEMPLATE_NAME
#define BLYNK_AUTH_TOKEN MY_BLYNK_AUTH_TOKEN

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_MCP23X17.h>
#include <RTClib.h>
#include <WiFiManager.h>

extern const int espLedPin;
extern const int resetWifiPin;
extern const int espLedPin;

extern Adafruit_MCP23X17 mcpRL1, mcpSW1;
extern RTC_DS3231 rtc;

extern bool ledState[16];
extern bool lastSwitchState[16];

extern int startHr[16];
extern int startMin[16];
extern int endHr[16];
extern int endMin[16];
extern bool isTimerActive[16];

extern unsigned long lastBlinkTime;
extern bool ledStatus;

void controlRelay(int pin, int state);

#endif