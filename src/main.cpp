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
bool isNetworkReady = false;

WiFiManager wm;

// ===== BLYNK TASK FOR FREE-RTOS =====
// បង្កើត Task ថ្មីមួយដើម្បីឱ្យ Blynk ដើរដាច់ដោយឡែកពី loop() ទប់ស្កាត់ការគាំងពេលមាន WiFi តែអត់ Internet
TaskHandle_t blynkTaskHandle;

void blynkTask(void *pvParameters)
{
  static unsigned long lastBlynkCheck = 0;
  for (;;)
  {
    if (isNetworkReady)
    {
      if (!Blynk.connected())
      {
        if (millis() - lastBlynkCheck > 7000)
        {
          lastBlynkCheck = millis();
          Serial.println("🔄 Network is ready. Connecting to Blynk Server...");
          Blynk.disconnect();
          Blynk.connect(); // បើគាំងនៅទីនេះ វាគាំងតែក្នុង Task នេះទេ លែងប៉ះពាល់ដល់ loop() ទៀតហើយ!
        }
      }
      else
      {
        Blynk.run();
        processBlynkSync();
      }
      digitalWrite(espLedPin, HIGH);
    }
    else
    {
      if (Blynk.connected())
        Blynk.disconnect();

      if (millis() - lastBlinkTime >= 500)
      {
        lastBlinkTime = millis();
        ledStatus = !ledStatus;
        digitalWrite(espLedPin, ledStatus);
      }
    }

    // ចាំបាច់ត្រូវមាន vTaskDelay ដើម្បីឱ្យ FreeRTOS អាចដកដង្ហើម និងបើកផ្លូវឱ្យ loop() ដើរ
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

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
  initW5500();

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

  // បញ្ជាឱ្យ Blynk Task ចាប់ផ្តើមដំណើរការស្របគ្នាជាមួយ loop()
  xTaskCreatePinnedToCore(
      blynkTask,        // Task function
      "BlynkTask",      // Name of task
      4096,             // Stack size
      NULL,             // Parameters
      1,                // Priority (1 is good for network tasks)
      &blynkTaskHandle, // Task handle
      1);               // Pin to Core 1 (same as loop, safe for I2C)

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

  // ===== ទប់ស្កាត់ TCP/IP Crash =====
  isNetworkReady = false;

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

  // ===== HARDWARE FUNCTIONS =====
  updateLCDDisplay();
  checkRTCTimers();
  checkPhysicalSwitches();
}
