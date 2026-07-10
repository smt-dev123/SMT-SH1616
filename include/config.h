#ifndef CONFIG_H
#define CONFIG_H

// ===== BLYNK CREDENTIALS (from build_flags) =====
#ifndef MY_FIWI_SSID
#warning "MY_FIWI_SSID not defined! Using empty string."
#define MY_FIWI_SSID "SMT-SH1616"
#endif

#ifndef MY_BLYNK_TEMPLATE_ID
#warning "MY_BLYNK_TEMPLATE_ID not defined! Using empty string."
#define MY_BLYNK_TEMPLATE_ID ""
#endif

#ifndef MY_BLYNK_TEMPLATE_NAME
#warning "MY_BLYNK_TEMPLATE_NAME not defined! Using empty string."
#define MY_BLYNK_TEMPLATE_NAME ""
#endif

#ifndef MY_BLYNK_AUTH_TOKEN
#warning "MY_BLYNK_AUTH_TOKEN not defined! Using empty string."
#define MY_BLYNK_AUTH_TOKEN ""
#endif

#define MY_SSID MY_FIWI_SSID
#define BLYNK_TEMPLATE_ID MY_BLYNK_TEMPLATE_ID
#define BLYNK_TEMPLATE_NAME MY_BLYNK_TEMPLATE_NAME
#define BLYNK_AUTH_TOKEN MY_BLYNK_AUTH_TOKEN

// ===== W5500 PINS (from build_flags) =====
#ifndef W5500_CS_PIN
#define W5500_CS_PIN 5
#endif

#ifndef W5500_RST_PIN
#define W5500_RST_PIN 4
#endif

#ifndef W5500_INT_PIN
#define W5500_INT_PIN 34
#endif

// ===== SPI PINS =====
#ifndef SPI_SCK_PIN
#define SPI_SCK_PIN 18
#endif

#ifndef SPI_MISO_PIN
#define SPI_MISO_PIN 19
#endif

#ifndef SPI_MOSI_PIN
#define SPI_MOSI_PIN 23
#endif

// ===== LED PINS =====
#ifndef ESP_LED_PIN
#define ESP_LED_PIN 13
#endif

#ifndef RESET_WIFI_PIN
#define RESET_WIFI_PIN 15
#endif

// ===== I2C ADDRESSES =====
#ifndef MCP_RL1_ADDR
#define MCP_RL1_ADDR 0x20
#endif

#ifndef MCP_SW1_ADDR
#define MCP_SW1_ADDR 0x21
#endif

// ===== INCLUDES =====
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <Ethernet.h>

#include <BlynkApiArduino.h>
#include <Blynk/BlynkProtocol.h>
#include <Adapters/BlynkArduinoClient.h>

#include "DualClient.h"

// Define custom Blynk object that supports DualClient
class BlynkDual : public BlynkProtocol<BlynkArduinoClient>
{
    typedef BlynkProtocol<BlynkArduinoClient> Base;

public:
    BlynkDual(BlynkArduinoClient &transp) : Base(transp) {}
    void config(const char *auth, const char *domain = BLYNK_DEFAULT_DOMAIN, uint16_t port = BLYNK_DEFAULT_PORT)
    {
        Base::begin(auth);
        this->conn.begin(domain, port);
    }
    void config(const char *auth, IPAddress ip, uint16_t port = BLYNK_DEFAULT_PORT)
    {
        Base::begin(auth);
        this->conn.begin(ip, port);
    }
};

extern BlynkDual Blynk;

#include <WidgetRTC.h>
#include <WidgetTimeInput.h>
#include <WiFiManager.h>
#include <Adafruit_MCP23X17.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

// ===== EXTERNAL VARIABLES =====
extern Adafruit_MCP23X17 mcpRL1, mcpSW1;
extern RTC_DS3231 rtc;
extern WidgetRTC blynkRtc;
extern LiquidCrystal_I2C lcd;

extern bool ledState[16];
extern bool lastSwitchState[16];

extern int startHr[16];
extern int startMin[16];
extern int endHr[16];
extern int endMin[16];
extern bool isTimerActive[16];

extern unsigned long lastBlinkTime;
extern bool ledStatus;
extern bool isEthernetConnected;

// ===== FUNCTION PROTOTYPES =====
void controlRelay(int pin, int state);
void initW5500();
void checkEthernetStatus();

#endif