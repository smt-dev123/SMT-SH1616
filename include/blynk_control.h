#ifndef BLYNK_CONTROL_H
#define BLYNK_CONTROL_H

#include "config.h"
#include "hardware_control.h"

BLYNK_WRITE_DEFAULT()
{
    int vPin = request.pin;

    // ពិនិត្យមើលថា vPin ស្ថិតក្នុង Range
    if (vPin >= 0 && vPin <= 15)
    {
        BlynkParam::iterator it = param.begin();

        // ១. ប្រសិនបើ Data ដែលផ្ញើមកមានច្រើនធាតុ (វាជា Time Input Widget)
        if (it < param.end() && ++it < param.end())
        {
            TimeInputParam t(param);
            int index = vPin; // ប្រើ vPin ធ្វើជា Index ផ្ទុកទិន្នន័យម៉ោងតែម្តង

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
                // បើលុប Timer ចោលវិញ (Clear Timer)
                isTimerActive[index] = false;
                Serial.printf("V%d បានលុប Timer ចោល\n", vPin);
            }
        }
        // ២. ប្រសិនបើ Data ជាលេខទោល (វាជា Button Widget ឬ Switch)
        else
        {
            int buttonState = param.asInt();
            controlRelay(vPin, buttonState);
            Serial.printf("V%d ប៊ូតុងបញ្ជា: %s\n", vPin, buttonState ? "បើក (ON)" : "បិទ (OFF)");
        }
    }
}

#endif