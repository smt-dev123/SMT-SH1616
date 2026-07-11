#ifndef BLYNK_CONTROL_H
#define BLYNK_CONTROL_H

#include "config.h"
#include "hardware_control.h"

int syncPinIndex = -1;
unsigned long lastSyncTime = 0;

void processBlynkSync()
{
    if (syncPinIndex >= 0 && syncPinIndex < 16)
    {
        if (millis() - lastSyncTime >= 1000)
        { // ពន្យារពេល 1 វិនាទី កុំឱ្យទាញចរន្តព្រមគ្នា
            int targetVPin = V0 + syncPinIndex;
            Blynk.syncVirtual(targetVPin);
            Serial.printf("🔄 Synced Pin: V%d\n", targetVPin);

            syncPinIndex++;
            lastSyncTime = millis();

            if (syncPinIndex >= 16)
            {
                Serial.println("✅ All 16 channels synced successfully!");
                syncPinIndex = -1;
            }
        }
    }
}

// មុខងារទាញយក State មកវិញម្តងមួយៗនៅពេលប្រព័ន្ធភ្ជាប់ទៅ Blynk បានជោគជ័យ
BLYNK_CONNECTED()
{
    blynkRtc.begin();
    Serial.println("⏰ Internet Time Synced via Blynk RTC!");

    syncPinIndex = 0;
    lastSyncTime = millis();
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