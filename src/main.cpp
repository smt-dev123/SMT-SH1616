#include "config.h"
#include "hardware_control.h"
#include "blynk_control.h"
#include "lcd_control.h"

// ===== USE DEFINES FROM BUILD_FLAGS =====
const int espLedPin = ESP_LED_PIN;
const int resetWifiPin = RESET_WIFI_PIN;
const int mcpRL1Addr = MCP_RL1_ADDR;
const int mcpSW1Addr = MCP_SW1_ADDR;

// MAC Address for W5500
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

Adafruit_MCP23X17 mcpRL1, mcpSW1;
RTC_DS3231 rtc;
WidgetRTC blynkRtc;

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

// ===== WIFI MANAGER CALLBACK =====
// រត់អូតូនៅពេលអ្នកប្រើប្រាស់កំណត់ WiFi រួចហើយចុច Save លើទូរស័ព្ទ
void saveConfigCallback()
{
  Serial.println("💾 WiFi settings saved successfully!");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Saved!");
  lcd.setCursor(0, 1);
  lcd.print("Restarting...   ");
  delay(1500);
  ESP.restart(); // ចាប់ផ្តើមឡើងវិញភ្លាម ការពារការគាំងទាក់ទិន្នន័យ (Network Core Hang)
}

// ===== INIT W5500 =====
void initW5500()
{
  pinMode(W5500_RST_PIN, OUTPUT);
  digitalWrite(W5500_RST_PIN, LOW);
  delay(50);
  digitalWrite(W5500_RST_PIN, HIGH);
  delay(200);

  SPI.begin();
  Ethernet.init(W5500_CS_PIN);
  Serial.println("🌐 Initializing W5500 Ethernet...");

  if (Ethernet.begin(mac) == 0)
  {
    Serial.println("❌ Ethernet DHCP failed!");
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

  // ===== INITIALIZE I2C & LCD =====
  Wire.begin();
  initLCD();

  // ===== DEBUG BUILD FLAGS =====
  Serial.println("=== Build Configuration ===");
  Serial.printf("Blynk Template ID: %s\n", BLYNK_TEMPLATE_ID);
  Serial.println("==========================\n");

  // ===== INITIALIZE PINS =====
  pinMode(espLedPin, OUTPUT);
  digitalWrite(espLedPin, LOW);
  pinMode(resetWifiPin, INPUT_PULLUP);

  // ===== INITIALIZE I2C DEVICES =====
  if (!mcpRL1.begin_I2C(mcpRL1Addr))
    Serial.println("❌ MCP23X17 Relay not found!");
  if (!mcpSW1.begin_I2C(mcpSW1Addr))
    Serial.println("❌ MCP23X17 Switch not found!");
  if (!rtc.begin())
    Serial.println("❌ DS3231 RTC not found!");

  // ===== CONFIGURE MCP PINS =====
  for (int i = 0; i < 16; i++)
  {
    mcpRL1.pinMode(i, OUTPUT);
    mcpRL1.digitalWrite(i, LOW);

    mcpSW1.pinMode(i, INPUT_PULLUP);
    lastSwitchState[i] = mcpSW1.digitalRead(i);
  }

  // ===== NETWORK INITIALIZATION =====
  initW5500(); // ចាប់ផ្តើម Ethernet ជាអាទិភាពចម្បង

  if (!isEthernetConnected)
  {
    Serial.println("📡 No Ethernet, starting WiFi Manager...");
    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(180);
    wm.setSaveConfigCallback(saveConfigCallback); // ហៅ Callback ពេលចុច Save WiFi លើ Portal

    if (!wm.autoConnect("SHMT-HOME-CONFIG"))
    {
      Serial.println("⚠️ WiFi Config Portal started");
    }
  }

  // ===== BLYNK CONFIG =====
  Blynk.config(BLYNK_AUTH_TOKEN);
  Serial.println("✅ Setup complete!\n");
}

void loop()
{
  // ===== RESET WIFI BUTTON (Hold 3 seconds) =====
  if (digitalRead(resetWifiPin) == LOW)
  {
    delay(100);
    unsigned long holdTime = millis();
    while (digitalRead(resetWifiPin) == LOW)
    {
      if (millis() - holdTime > 3000)
      {
        Serial.println("🗑️ Resetting WiFi settings...");
        wm.resetSettings();
        Serial.println("🔄 Restarting...");
        ESP.restart();
      }
    }
  }

  // ឆែកស្ថានភាពខ្សែ Ethernet ជាប្រចាំ
  checkEthernetStatus();

  // ===== NETWORK LOGIC (PREVENT CONFLICT) =====
  if (isEthernetConnected)
  {
    // ករណីទី១៖ បើ Ethernet ដំណើរការ រត់តែ Blynk តាមរន្ធដោតទៅបានហើយ
    Blynk.run();
    digitalWrite(espLedPin, HIGH);
  }
  else
  {
    // ករណីទី២៖ បើគ្មាន Ethernet ទើបអនុញ្ញាតឱ្យ WiFi Manager ដំណើរការ Background Process
    wm.process();

    if (WiFi.status() == WL_CONNECTED)
    {
      Blynk.run();
      digitalWrite(espLedPin, HIGH);
    }
    else
    {
      // ករណីដាច់អ៊ីនធឺណិតទាំងពីរ (ឱ្យ LED ភ្លឹបភ្លែតៗ)
      if (millis() - lastBlinkTime >= 500)
      {
        lastBlinkTime = millis();
        ledStatus = !ledStatus;
        digitalWrite(espLedPin, ledStatus);
      }
    }
  }

  // ===== HARDWARE FUNCTIONS =====
  updateLCDDisplay();      // ធ្វើបច្ចុប្បន្នភាព LCD ឆ្លាស់ទំព័ររាល់ ៤ វិនាទី
  checkRTCTimers();        // ពិនិត្យម៉ោងបើក/បិទ ពី RTC
  checkPhysicalSwitches(); // ពិនិត្យប៊ូតុងកុងតាក់ជញ្ជាំង
}