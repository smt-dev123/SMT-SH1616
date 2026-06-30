#ifndef HARDWARE_CONTROL_H
#define HARDWARE_CONTROL_H

#include "config.h"

// អនុគមន៍បញ្ជា Relay
void controlRelay(int pin, int state)
{
    if (pin < 0 || pin >= 16)
        return;

    if (ledState[pin] == state)
        return;

    ledState[pin] = state;

    // Relay Module (Active LOW - បើ Relay ប្រើ Active HIGH ត្រូវប្តូរ)
    mcpRL1.digitalWrite(pin, state ? HIGH : LOW);

    if (Blynk.connected())
    {
        Blynk.virtualWrite(V0 + pin, state);
    }
}

// អនុគមន៍ពិនិត្យម៉ោងពី RTC
void checkRTCTimers()
{
    static unsigned long lastTimeCheck = 0;
    if (millis() - lastTimeCheck >= 1000)
    {
        lastTimeCheck = millis();
        DateTime now = rtc.now();
        int currentHour = now.hour();
        int currentMinute = now.minute();

        for (int i = 0; i < 16; i++)
        {
            if (isTimerActive[i])
            {
                if (currentHour == startHr[i] && currentMinute == startMin[i] && ledState[i] == 0)
                {
                    controlRelay(i, 1);
                    Serial.printf("Timer: បើក Relay %d\n", i);
                }
                else if (currentHour == endHr[i] && currentMinute == endMin[i] && ledState[i] == 1)
                {
                    controlRelay(i, 0);
                    Serial.printf("Timer: បិទ Relay %d\n", i);
                }
            }
        }
    }
}

// អនុគមន៍ឆែកប៊ូតុងកុងតាក់
void checkPhysicalSwitches()
{
    static unsigned long lastDebounceTime[16] = {0};
    for (int i = 0; i < 16; i++)
    {
        bool currentSwitchState = mcpSW1.digitalRead(i);
        if (currentSwitchState != lastSwitchState[i])
        {
            if (lastDebounceTime[i] == 0)
            {
                lastDebounceTime[i] = millis();
            }

            if ((millis() - lastDebounceTime[i]) > 50)
            {
                int targetState = (currentSwitchState == LOW) ? 1 : 0;
                controlRelay(i, targetState);
                lastSwitchState[i] = currentSwitchState;
                lastDebounceTime[i] = 0;
            }
        }
        else
        {
            if (lastDebounceTime[i] != 0)
            {
                lastDebounceTime[i] = 0;
            }
        }
    }
}

#endif