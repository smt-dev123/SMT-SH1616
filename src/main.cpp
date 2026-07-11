#include "config.h"
#include "hardware_control.h"
#include "blynk_control.h"
#include "lcd_control.h"
#include "ethernet_control.h"

// ===== USE DEFINES FROM BUILD_FLAGS =====
const int espLedPin = ESP_LED_PIN;
const int resetWifiPin = RESET_WIFI_PIN;
const int mcpRL1Addr = MCP_RL1_ADDR;
const int mcpSW1Addr = MCP_SW1_ADDR;

// MAC Address for W5500
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
bool isEthernetConnected = false;

DualClient dualClient;
BlynkArduinoClient blynkTransport(dualClient);
BlynkDual Blynk(blynkTransport);

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
  initW5500(); // ហៅអនុគមន៍ពី ethernet_control.h

  if (!isEthernetConnected)
  {
    Serial.println("📡 No Ethernet, starting WiFi Manager...");
    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(180);
    wm.setSaveConfigCallback(saveConfigCallback);

    if (!wm.autoConnect(MY_SSID))
    {
      Serial.println("⚠️ WiFi Config Portal started");
    }
  }

  // ===== BLYNK CONFIG =====
  Blynk.config(BLYNK_AUTH_TOKEN, IPAddress(64, 225, 16, 22), 80);
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

  // ឆែកស្ថានភាពខ្សែ Ethernet ជាប្រចាំ (រក្សាទុក Non-blocking ពី ethernet_control.h)
  checkEthernetStatus();

  // ===== កែសម្រួលប្រព័ន្ធគ្រប់គ្រងបណ្តាញ ដើម្បីទប់ស្កាត់ TCP/IP Crash =====
  static unsigned long lastBlynkCheck = 0;
  bool isNetworkReady = false;

  // ពិនិត្យមើលថា តើមានបណ្តាញណាមួយដែលអាចប្រើការបានពិតប្រាកដ?
  if (isEthernetConnected)
  {
    isNetworkReady = true;
  }
  else
  {
    wm.process(); // ដំណើរការ WiFiManager Background ករណីអត់មាន Ethernet
    if (WiFi.status() == WL_CONNECTED)
    {
      isNetworkReady = true;
    }
  }

  // បើមានបណ្តាញច្បាស់លាស់ (ទោះជា LAN ឬ WiFi) ទើបអនុញ្ញាតឱ្យ Blynk ធ្វើការ
  if (isNetworkReady)
  {
    if (!Blynk.connected())
    {
      // ព្យាយាមភ្ជាប់ទៅ Blynk តែម្តងគត់រាល់ ៧ វិនាទី (កុំឱ្យ Aggressive ពេកនាំឱ្យ Stack ពេញ)
      if (millis() - lastBlynkCheck > 7000)
      {
        lastBlynkCheck = millis();
        Serial.println("🔄 Network is ready. Connecting to Blynk Server...");

        // កាត់ផ្តាច់ Socket ចាស់ដែលខូចចោល រួចបង្កើត Fresh Connection
        Blynk.disconnect();

        // បង្ខំឱ្យរត់ទៅរក IP ផ្ទាល់របស់ Blynk តែម្តងដើម្បីជៀសវាង DNS Crash
        Blynk.connect();
      }
    }
    else
    {
      Blynk.run(); // ដំណើរការ Blynk ធម្មតាពេលភ្ជាប់ជោគជ័យ
      processBlynkSync(); // ដំណើរការទាញយក State ម្តងមួយៗ
    }
    digitalWrite(espLedPin, HIGH);
  }
  else
  {
    // ករណីដាច់ទាំងពីរ៖ បិទ Blynk ចោលភ្លាម ការពារប្រព័ន្ធទាញកូដគាំង (Invalid mbox)
    if (Blynk.connected())
      Blynk.disconnect();

    // ឱ្យ LED ភ្លឹបភ្លែតៗប្រាប់សញ្ញាដាច់ Net
    if (millis() - lastBlinkTime >= 500)
    {
      lastBlinkTime = millis();
      ledStatus = !ledStatus;
      digitalWrite(espLedPin, ledStatus);
    }
  }

  // ===== HARDWARE FUNCTIONS =====
  updateLCDDisplay();
  checkRTCTimers();
  checkPhysicalSwitches();
}
