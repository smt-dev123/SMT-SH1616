#ifndef LCD_CONTROL_H
#define LCD_CONTROL_H

#include <LiquidCrystal_I2C.h>
#include "config.h"

LiquidCrystal_I2C lcd(0x27, 20, 4);

void initLCD()
{
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("********************");
    lcd.setCursor(0, 1);
    lcd.print("**** SMT-SH1616 ****");
    lcd.setCursor(2, 2);
    lcd.print("System Booting...");
    lcd.setCursor(0, 3);
    lcd.print("********************");
    delay(1500);
    lcd.clear();
}

void updateLCDDisplay()
{
    static unsigned long lastLCDUpdate = 0;

    // ធ្វើបច្ចុប្បន្នភាពទិន្នន័យរាល់ ១ វិនាទីម្តង
    if (millis() - lastLCDUpdate >= 1000)
    {
        lastLCDUpdate = millis();

        // ================== TIME & NETWORK ==================
        // DateTime now = rtc.now();
        lcd.setCursor(0, 0);
        char timeStr[21];
        snprintf(timeStr, sizeof(timeStr), "TIME: %02d:%02d:%02d    ", hour(), minute(), second());
        lcd.print(timeStr);

        lcd.setCursor(0, 1);
        if (isEthernetConnected)
        {
            lcd.print("NETWORK: ETHERNET   ");
        }
        else if (WiFi.status() == WL_CONNECTED)
        {
            String netStr = "NETWORK: " + WiFi.SSID();
            while (netStr.length() < 20)
                netStr += " ";
            lcd.print(netStr.substring(0, 20));
        }
        else
        {
            lcd.print("NETWORK: NO NETWORK ");
        }

        // ================== 2: RELAY STATUS (1-16) ==================
        // ជួរទី៣៖ លេខឆានែល (1 ដល់ 9 និង 0=10, 1=11... 6=16)
        lcd.setCursor(2, 2);
        lcd.print("1234567890123456  ");

        // ជួរទី៤៖ State 0 ឬ 1 របស់ Relay ទាំង ១៦
        lcd.setCursor(2, 3);
        for (int i = 0; i < 16; i++)
        {
            lcd.print(ledState[i]);
        }
        lcd.print("    ");
    }
}

#endif