# 🤖 ESP32 Industrial Smart Home Automation (MCP23017 I2C Expander)

![Blynk Powered](https://img.shields.io/badge/Blynk-IoT-blue?style=for-the-badge&logo=blynk)
![Framework](https://img.shields.io/badge/Framework-Arduino-official?style=for-the-badge&logo=arduino)
![Hardware](https://img.shields.io/badge/I2C%20Expander-MCP23017%20x2-red?style=for-the-badge)
![Network](https://img.shields.io/badge/Network-Ethernet%20%2F%20WiFi%20AP-green?style=for-the-badge&logo=wi-fi)

ប្រព័ន្ធគ្រប់គ្រងការបិទ-បើកឧបករណ៍អគ្គិសនីកម្រិតឧស្សាហកម្មចំនួន **១៦ ឆានែល (16 Channels)** ដោយប្រើប្រាស់បន្ទះ **MCP23017 I/O Expander ចំនួន ២ គ្រាប់** តាមរយៈខ្សែសញ្ញា I2C តែ ២ ខ្សែ (SDA & SCL) ជួយសន្សំសំចៃជើង GPIO លើ ESP32 យ៉ាងច្រើន។

គម្រោងនេះត្រូវបានរចនាឡើងដើម្បីដំណើរការបានទាំងលក្ខណៈ **Online (Blynk Cloud)** និង **Offline (Local System)** ដោយគ្រប់ឆានែលទាំងអស់ (V0-V15) អាចបញ្ជាបានទាំងដៃ (Physical Switch) និងរត់តាមម៉ោងកំណត់ (Timer) ផ្ទាល់ខ្លួនស្របពេលគ្នា។

---

## 🌐 ប្រព័ន្ធបណ្តាញពីរផ្លូវ & មុខងារ AP Configuration (192.168.4.1)

ប្រព័ន្ធនេះផ្តល់អាទិភាពខ្ពស់ទៅលើស្ថិរភាពបណ្តាញអ៊ីនធឺណិត ដោយគាំទ្រទាំង **Ethernet (ខ្សែ LAN)** និង **WiFi**។

### តើអ្វីទៅជាអាសយដ្ឋាន IP `192.168.4.1`?

នៅពេលដែលប្រព័ន្ធដាច់អ៊ីនធឺណិត ឬមិនទាន់បានកំណត់ WiFi ដំបូង ESP32 នឹងបំប្លែងខ្លួនវាទៅជាឧបករណ៍ផ្សាយ WiFi ផ្ទាល់ខ្លួន (ហៅថា **Captive Portal / Access Point Mode**)។

- អាសយដ្ឋាន **`192.168.4.1`** គឺជា Gateway លំនាំដើម (Default IP Address) របស់ ESP32។
- នៅពេលទូរស័ព្ទរបស់អ្នកភ្ជាប់ទៅកាន់ WiFi របស់ ESP32 ហើយបើក Web Browser វាយលេខ `192.168.4.1` នោះវានឹងបង្ហាញ **ទំព័រវ៉ិបសាយសម្រាប់កំណត់ទិន្នន័យ (WiFi Manager Page)** ភ្លាមៗដោយមិនបាច់ប្រើអ៊ីនធឺណិតឡើយ។ អ្នកអាចជ្រើសរើស WiFi ក្នុងផ្ទះ និងវាយ Password រួចចុច Save នោះ ESP32 នឹងចងចាំទិន្នន័យនេះក្នុង Flash Memory (EEPROM) ជារៀងរហូត។

---

## 🔌 ការកំណត់អាសយដ្ឋាន MCP23017 (I2C Addressing)

ដើម្បីកុំឱ្យជាន់អាសយដ្ឋាន (Address) គ្នានៅលើខ្សែ I2C តែមួយ យើងត្រូវកំណត់ជើង Hardware Address (A0, A1, A2) លើបន្ទះ MCP23017 នីមួយៗដូចខាងក្រោម៖

| ឧបករណ៍ (Device) | ជើង A2 | ជើង A1 | ជើង A0 | I2C Hex Address | មុខងារចម្បង (Function)                     |
| :-------------- | :----: | :----: | :----: | :-------------: | :----------------------------------------- |
| **MCP23017 #1** |  GND   |  GND   |  GND   |   **`0x20`**    | បញ្ជា Output ទៅកាន់ **16-Channel Relay**   |
| **MCP23017 #2** |  GND   |  GND   |  VCC   |   **`0x21`**    | អានទិន្នន័យ Input ពី **16-Channel Switch** |
| **DS3231 RTC**  |   -    |   -    |   -    |   **`0x68`**    | ម៉ូឌុលនាឡិកា (រត់ស្របគ្នានៅលើខ្សែ I2C)     |

---

## 🗺️ គំនូសបំព្រួញលំហូរការងារ (System Flowchart)

```mermaid
flowchart TD
    A["Start"] --> B["setup()"]
    B --> C["Initialize I2C, RTC"]
    C --> C1["mcp_relay.begin_I2C(0x20) & set all OUTPUT"]
    C1 --> C2["mcp_switch.begin_I2C(0x21) & set all INPUT_PULLUP"]

    %% Network Selection %%
    C2 --> D{"Ethernet Cable Connected?"}
    D -->|Yes| E["Blynk.begin(auth, ... via Ethernet)"]
    D -->|No| F{"WiFi Credentials Saved?"}
    F -->|No| G["Start AP Mode Configuration WebServer"]
    G --> G1["Connect Phone & Open [http://192.168.4.1](http://192.168.4.1)"] --> F
    F -->|Yes| H["Blynk.begin(auth, saved_ssid, saved_pass)"]

    %% Main Loop %%
    E --> I["loop()"]
    H --> I
    I --> J{"Network Connected?"}
    J -->|Yes| K["Blynk.run() & espLedPin HIGH"]
    J -->|No| L["Blink espLedPin (500ms) & Run Offline"]

    %% Omni-Channel Processing %%
    K --> M["Check System Timer (Every 1000ms)"]
    L --> M
    M --> N{"Timer check due?"}

    %% 16 Channel Timer Scan %%
    N -->|Yes| O["Scan 16 Timers (V16-V31) using RTC time"]
    O --> P{"isTimerActive[i]?"}
    P -->|Yes| Q{"Match Start/End Time?"}
    Q -->|Yes| R["mcp_relay.digitalWrite(i, targetState) & Sync Blynk"]
    Q -->|No| S["Go to Switch Scan"]
    P -->|No| S
    N -->|No| S
    R --> S

    %% 16 Channel Switch Scan via MCP23017 (0x21) %%
    S --> T["mcp_switch.digitalRead(i) for all 16 pins"]
    T --> U{"State Changed & Debounce > 50ms?"}
    U -->|Yes| V["mcp_relay.digitalWrite(i, targetState) & Sync Blynk"]
    U -->|No| W["Continue Loop"]
    V --> W
    W --> I

    %% Blynk Background Callbacks %%
    X["BLYNK_WRITE(V0..V15) - App Switch"] --> AD["mcp_relay.digitalWrite(pin, state)"]
    Y["BLYNK_WRITE(V16..V31) - App Timer"] --> AA["processTimeInput(channel)"]
    AA --> AB["Update 16-Channel Timer Arrays"]
    Z["BLYNK_CONNECTED"] --> AC["blynkRtc.begin() & Sync All Button States"]
```
