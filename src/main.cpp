#include "config.h"
#include "hardware_control.h"
#include "blynk_control.h"

// ===== USE DEFINES FROM BUILD_FLAGS =====
const int espLedPin = ESP_LED_PIN;
const int resetWifiPin = RESET_WIFI_PIN;
const int mcpRL1Addr = MCP_RL1_ADDR;
const int mcpSW1Addr = MCP_SW1_ADDR;

// MAC Address for W5500
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

Adafruit_MCP23X17 mcpRL1, mcpSW1;
RTC_DS3231 rtc;

bool ledState[16] = {0};
bool lastSwitchState[16] = {1};

int startHr[16];
int startMin[16];
int endHr[16];
int endMin[16];
bool isTimerActive[16] = {false};

unsigned long lastBlinkTime = 0;
bool ledStatus = false;
bool isEthernetConnected = false;

WiFiManager wm;

// ===== INIT W5500 =====
void initW5500()
{
  pinMode(W5500_RST_PIN, OUTPUT);
  digitalWrite(W5500_RST_PIN, LOW);
  delay(50);
  digitalWrite(W5500_RST_PIN, HIGH);
  delay(200);

  SPI.begin(); // Initialize SPI
  Ethernet.init(W5500_CS_PIN);
  Serial.println("🌐 Initializing W5500 Ethernet...");

  if (Ethernet.begin(mac) == 0)
  {
    Serial.println("❌ Ethernet DHCP failed!");
    Serial.println("   Check cable connection or DHCP server");
    isEthernetConnected = false;
  }
  else
  {
    Serial.print("✅ Ethernet connected! IP: ");
    Serial.println(Ethernet.localIP());
    isEthernetConnected = true;
  }
}

// ===== CHECK ETHERNET STATUS =====
void checkEthernetStatus()
{
  if (isEthernetConnected)
  {
    if (Ethernet.linkStatus() == LinkOFF)
    {
      Serial.println("⚠️ Ethernet link lost!");
      isEthernetConnected = false;
    }
  }
  else
  {
    static unsigned long lastReconnect = 0;
    if (millis() - lastReconnect > 10000)
    {
      lastReconnect = millis();
      Serial.println("🔄 Attempting Ethernet reconnect...");
      initW5500();
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial.println("\n🚀 System Starting...");

  // ===== DEBUG BUILD FLAGS =====
  Serial.println("=== Build Configuration ===");
  Serial.printf("W5500 CS Pin: %d\n", W5500_CS_PIN);
  Serial.printf("W5500 RST Pin: %d\n", W5500_RST_PIN);
  Serial.printf("ESP LED Pin: %d\n", ESP_LED_PIN);
  Serial.printf("Reset WiFi Pin: %d\n", RESET_WIFI_PIN);
  Serial.printf("MCP Relay Addr: 0x%02X\n", MCP_RL1_ADDR);
  Serial.printf("MCP Switch Addr: 0x%02X\n", MCP_SW1_ADDR);
  Serial.printf("Blynk Template ID: %s\n", BLYNK_TEMPLATE_ID);
  Serial.printf("Blynk Template Name: %s\n", BLYNK_TEMPLATE_NAME);
  Serial.println("==========================\n");

  // ===== INITIALIZE PINS =====
  pinMode(espLedPin, OUTPUT);
  digitalWrite(espLedPin, LOW);
  pinMode(resetWifiPin, INPUT_PULLUP);

  // ===== INITIALIZE I2C DEVICES =====
  Wire.begin();

  if (!mcpRL1.begin_I2C(mcpRL1Addr))
    Serial.println("❌ MCP23X17 Relay not found!");
  else
    Serial.println("✅ MCP23X17 Relay initialized");

  if (!mcpSW1.begin_I2C(mcpSW1Addr))
    Serial.println("❌ MCP23X17 Switch not found!");
  else
    Serial.println("✅ MCP23X17 Switch initialized");

  if (!rtc.begin())
    Serial.println("❌ DS3231 RTC not found!");
  else
    Serial.println("✅ RTC initialized");

  // ===== CONFIGURE MCP PINS =====
  for (int i = 0; i < 16; i++)
  {
    mcpRL1.pinMode(i, OUTPUT);
    mcpRL1.digitalWrite(i, LOW);

    mcpSW1.pinMode(i, INPUT_PULLUP);
    lastSwitchState[i] = mcpSW1.digitalRead(i);
  }

  // ===== ETHERNET (PRIMARY) =====
  initW5500();

  // ===== WIFI (BACKUP) =====
  if (!isEthernetConnected)
  {
    Serial.println("📡 No Ethernet, starting WiFi Manager...");
    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(180);

    if (!wm.autoConnect("SHMT-HOME-CONFIG"))
    {
      Serial.println("⚠️ WiFi Config Portal started");
    }
  }

  // ===== BLYNK =====
  Blynk.config(BLYNK_AUTH_TOKEN);

  Serial.println("✅ Setup complete!\n");
}

void loop()
{
  // ===== WIFI MANAGER PROCESS =====
  wm.process();

  // ===== RESET WIFI BUTTON (Hold 3 seconds) =====
  if (digitalRead(resetWifiPin) == LOW)
  {
    delay(3000);
    if (digitalRead(resetWifiPin) == LOW)
    {
      Serial.println("🗑️ Resetting WiFi settings...");
      wm.resetSettings();
      Serial.println("🔄 Restarting...");
      ESP.restart();
    }
  }

  // ===== NETWORK HANDLING =====
  bool networkConnected = false;

  // Priority 1: Ethernet (W5500)
  checkEthernetStatus();
  if (isEthernetConnected)
  {
    Blynk.run();
    networkConnected = true;
    digitalWrite(espLedPin, HIGH);
  }

  // Priority 2: WiFi (Backup)
  if (!networkConnected && WiFi.status() == WL_CONNECTED)
  {
    Blynk.run();
    networkConnected = true;
    digitalWrite(espLedPin, HIGH);
  }

  // No connection - Blink LED
  if (!networkConnected)
  {
    if (millis() - lastBlinkTime >= 500)
    {
      lastBlinkTime = millis();
      ledStatus = !ledStatus;
      digitalWrite(espLedPin, ledStatus);
    }
  }

  // ===== HARDWARE FUNCTIONS =====
  checkRTCTimers();
  checkPhysicalSwitches();
}