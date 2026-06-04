#ifndef HARDWARE_CONTROL_H
#define HARDWARE_CONTROL_H

#include "config.h"

// អនុគមន៍បញ្ជា Relay
void controlRelay(int pin, int state)
{
    if (pin < 0 || pin >= 16)
        return;

    // ឆែកមើល៖ បើស្ថានភាពថ្មីដូចស្ថានភាពចាស់ដែលមានស្រាប់ មិនបាច់ធ្វើអ្វីទាំងអស់ (ការពារ Loop)
    if (ledState[pin] == state)
        return;

    // កត់ត្រាស្ថានភាពថ្មីចូលក្នុង Array
    ledState[pin] = state;

    // បញ្ជាទៅជើង Hardware MCP23X17
    mcpRL1.digitalWrite(pin, state ? HIGH : LOW); // បើ Relay ជា Active LOW ត្រូវដូរទៅ (state ? LOW : HIGH)

    // ផ្ញើទៅ Update លើ App តែជើងណាដែលមិនមែនជាទិន្នន័យបានមកពីការចុច App ផ្ទាល់
    // ឬ Update ទៅដើម្បីឲ្យប្រាកដថា App បង្ហាញស្ថានភាពត្រូវនឹង Hardware
    if (Blynk.connected())
    {
        Blynk.virtualWrite(V0 + pin, state);
    }
}

// អនុគមន៍ពិនិត្យម៉ោងពី RTC (ហៅទៅប្រើក្នុង loop)
void checkRTCTimers()
{
    static unsigned long lastTimeCheck = 0;
    if (millis() - lastTimeCheck >= 1000)
    {
        lastTimeCheck = millis();
        DateTime now = rtc.now();
        int currentHour = now.hour();
        int currentMinute = now.minute();

        for (int i = 0; i < 4; i++)
        {
            if (isTimerActive[i])
            {
                int relayPin = 12 + i;
                if (currentHour == startHr[i] && currentMinute == startMin[i] && ledState[relayPin] == 0)
                {
                    controlRelay(relayPin, 1);
                    Serial.printf("Timer: បើកភ្លើងជើងទី %d\n", relayPin);
                }
                else if (currentHour == endHr[i] && currentMinute == endMin[i] && ledState[relayPin] == 1)
                {
                    controlRelay(relayPin, 0);
                    Serial.printf("Timer: បិទភ្លើងជើងទី %d\n", relayPin);
                }
            }
        }
    }
}

// អនុគមន៍ឆែកប៊ូតុងកុងតាក់ (ហៅទៅប្រើក្នុង loop)
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