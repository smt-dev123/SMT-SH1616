#ifndef LCD_CONTROL_H
#define LCD_CONTROL_H

#include <LiquidCrystal_I2C.h>
#include "config.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);

void initLCD()
{
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("** SMT-SH1616 **");
    lcd.setCursor(0, 1);
    lcd.print("System Booting..");
    delay(1500);
    lcd.clear();
}

void updateLCDDisplay()
{
    static unsigned long lastLCDUpdate = 0;
    static unsigned long lastPageChange = 0;
    static int currentPage = 1;

    // ឆ្លាស់ទំព័រ (Page) រាល់ 5 វិនាទីម្តង
    if (millis() - lastPageChange >= 5000)
    {
        lastPageChange = millis();
        currentPage = (currentPage == 1) ? 2 : 1;
        lcd.clear(); // សម្អាតអេក្រង់ពេលដូរ Page ការពារអក្សរព្រិច និងជាន់គ្នា
    }

    // ធ្វើបច្ចុប្បន្នភាពទិន្នន័យរាល់ ១ វិនាទីម្តង
    if (millis() - lastLCDUpdate >= 1000)
    {
        lastLCDUpdate = millis();

        if (currentPage == 1)
        {
            // ================== PAGE 1: TIME & NETWORK ==================
            // DateTime now = rtc.now();
            lcd.setCursor(0, 0);
            char timeStr[17];
            snprintf(timeStr, sizeof(timeStr), "TIME: %02d:%02d:%02d", hour(), minute(), second());
            lcd.print(timeStr);

            lcd.setCursor(0, 1);
            if (isEthernetConnected)
            {
                lcd.print("*NET: ETHERNET");
            }
            else if (WiFi.status() == WL_CONNECTED)
            {
                lcd.print("*NET: " + WiFi.SSID());
            }
            else
            {
                lcd.print("*NET: NO NETWORK");
            }
        }
        else
        {
            // ================== PAGE 2: RELAY STATUS (1-16) ==================
            // ជួរទី១៖ លេខឆានែល (1 ដល់ 9 និង 0=10, 1=11... 6=16)
            lcd.setCursor(0, 0);
            lcd.print("1234567890123456");

            // ជួរទី២៖ State 0 ឬ 1 របស់ Relay ទាំង ១៦
            lcd.setCursor(0, 1);
            for (int i = 0; i < 16; i++)
            {
                lcd.print(ledState[i]);
            }
        }
    }
}

#endif