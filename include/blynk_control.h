#ifndef BLYNK_CONTROL_H
#define BLYNK_CONTROL_H

#include "config.h"
#include "hardware_control.h"

// មុខងារទាញយក State មកវិញម្តងមួយៗនៅពេលប្រព័ន្ធភ្ជាប់ទៅ Blynk បានជោគជ័យ
BLYNK_CONNECTED()
{
    blynkRtc.begin();
    Serial.println("⏰ Internet Time Synced via Blynk RTC!");

    for (int i = 0; i < 16; i++)
    {
        int targetVPin = V0 + i; // ឧបមាថាប្រើ V0 ដល់ V15 សម្រាប់ Relay 1 ដល់ 16
        Blynk.syncVirtual(targetVPin);
        Serial.printf("🔄 Synced Pin: V%d\n", targetVPin);
        delay(500); // ពន្យារពេល ០.៥ វិនាទី កុំឱ្យទាញចរន្តព្រមគ្នា និងការពារ Buffer ពេញ
    }

    Serial.println("✅ All 16 channels synced successfully!");
}

BLYNK_WRITE_DEFAULT()
{
    int vPin = request.pin;

    if (vPin >= 0 && vPin <= 15)
    {
        BlynkParam::iterator it = param.begin();

        // ១. ប្រសិនបើ Data ជា Time Input Widget (Timer)
        if (it < param.end() && ++it < param.end())
        {
            TimeInputParam t(param);
            int index = vPin;

            if (t.hasStartTime() && t.hasStopTime())
            {
                startHr[index] = t.getStartHour();
                startMin[index] = t.getStartMinute();
                endHr[index] = t.getStopHour();
                endMin[index] = t.getStopMinute();
                isTimerActive[index] = true;
                Serial.printf("V%d បានកំណត់ Timer: %02d:%02d ដល់ %02d:%02d\n", vPin, startHr[index], startMin[index], endHr[index], endMin[index]);
            }
            else
            {
                isTimerActive[index] = false;
                Serial.printf("V%d បានលុប Timer ចោល\n", vPin);
            }
        }
        // ២. ប្រសិនបើ Data ជាលេខទោល (Button / Switch)
        else
        {
            int buttonState = param.asInt();
            controlRelay(vPin, buttonState);
            Serial.printf("V%d ប៊ូតុងបញ្ជា: %s\n", vPin, buttonState ? "បើក (ON)" : "បិទ (OFF)");
        }
    }
}

#endif