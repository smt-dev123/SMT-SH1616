#include "config.h"
#include "hardware_control.h"
#include "blynk_control.h"

const int espLedPin = 13;
const int resetWifiPin = 15;
Adafruit_MCP23X17 mcpRL1, mcpSW1;
RTC_DS3231 rtc;

bool ledState[16] = {0};
bool lastSwitchState[16] = {1};

int startHr[16] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
int startMin[16] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
int endHr[16] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
int endMin[16] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
bool isTimerActive[16] = {false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false};

unsigned long lastBlinkTime = 0;
bool ledStatus = false;

WiFiManager wm;

void setup()
{
  Serial.begin(115200);
  pinMode(espLedPin, OUTPUT);
  digitalWrite(espLedPin, LOW);

  // កំណត់ជើងប៊ូតុង Reset Wi-Fi ជា INPUT_PULLUP
  pinMode(resetWifiPin, INPUT_PULLUP);

  if (!mcpRL1.begin_I2C(0x20))
    Serial.println("រកមិនឃើញ MCP23X17 Relay (0x20) ទេ!");
  if (!mcpSW1.begin_I2C(0x21))
    Serial.println("រកមិនឃើញ MCP23X17 Switch (0x21) ទេ!");
  if (!rtc.begin())
    Serial.println("រកមិនឃើញ DS3231 RTC ទេ!");

  for (int i = 0; i < 16; i++)
  {
    mcpRL1.pinMode(i, OUTPUT);
    mcpRL1.digitalWrite(i, LOW);
  }

  for (int i = 0; i < 16; i++)
  {
    mcpSW1.pinMode(i, INPUT_PULLUP);
    lastSwitchState[i] = mcpSW1.digitalRead(i);
  }

  // --- WiFiManager ដំណើរការបែប Non-blocking ---
  wm.setConfigPortalBlocking(false); // មិនឱ្យវាគាំងរង់ចាំទូរស័ព្ទ Connect ទេ (ធ្វើឱ្យកូដធ្លាក់ទៅ loop បាន)
  wm.setConfigPortalTimeout(180);    // បើគ្មានអ្នកចូលទៅ Setup ទេ ៣ នាទីក្រោយវានឹងបិទ AP ខ្លួនឯង

  // ប្តូរឈ្មោះ AP
  Serial.println("កំពុងត្រួតពិនិត្យការភ្ជាប់ Wi-Fi...");
  if (!wm.autoConnect("SMT-HOME-CONFIG"))
  {
    Serial.println("មិនទាន់មាន Wi-Fi ស្គាល់ ឬកំពុងបើក Mode Setup (AP)...");
  }
  else
  {
    Serial.println("បានភ្ជាប់ Wi-Fi រួចរាល់ហើយ!");
  }

  // កំណត់ Token ឱ្យ Blynk
  Blynk.config(BLYNK_AUTH_TOKEN);
}

void loop()
{
  // អនុញ្ញាតឱ្យ WiFiManager ដំណើរការ Background Tasks (ដូចជាដោះស្រាយ Web Server ពេលយើង Setup)
  wm.process();

  // --- Reset Wi-Fi ---
  if (digitalRead(resetWifiPin) == LOW)
  {
    delay(5000); // ទុកពេល ៥ វិនាទី
    if (digitalRead(resetWifiPin) == LOW)
    {
      Serial.println("កំពុងលុបព័ត៌មាន Wi-Fi ចាស់...");
      wm.resetSettings();
      Serial.println("កំពុង Restart ឧបករណ៍...");
      ESP.restart();
    }
  }

  // --- Blynk & ភ្លើងលោត (Blinking) ---
  if (WiFi.status() == WL_CONNECTED)
  {
    Blynk.run();
    digitalWrite(espLedPin, HIGH); // ភ្លឺជាប់ = ភ្ជាប់អ៊ីនធឺណិត និង Blynk រួចរាល់
  }
  else
  {
    // ភ្លើងនឹងលោតបាត់ៗរៀងរាល់ ៥០០ មីលីវិនាទី ក្នុងករណី៖
    // - ដាច់ Wi-Fi ទូទៅ
    // - ឧបករណ៍កំពុងបញ្ចេញ Wi-Fi "SMT-HOME-CONFIG"
    if (millis() - lastBlinkTime >= 500)
    {
      lastBlinkTime = millis();
      ledStatus = !ledStatus;
      digitalWrite(espLedPin, ledStatus);
    }
  }

  // មុខងារពិនិត្យម៉ោង និង ឆែក Switch នៅតែដំណើរការធម្មតា ទោះគ្មាន Wi-Fi ឬកំពុង Setup Wi-Fi ថ្មី
  checkRTCTimers();
  checkPhysicalSwitches();
}