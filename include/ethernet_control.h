#ifndef ETHERNET_CONTROL_H
#define ETHERNET_CONTROL_H

#include <SPI.h>
#include <Ethernet.h>
#include "config.h"

extern byte mac[];

void initW5500()
{
  // ១. Hardware Reset W5500 ឱ្យស្អាតល្អ
  pinMode(W5500_RST_PIN, OUTPUT);
  digitalWrite(W5500_RST_PIN, LOW);
  delay(50);
  digitalWrite(W5500_RST_PIN, HIGH);
  delay(200);

  // ២. កំណត់ Custom SPI Pins ឱ្យចំជើង PCB របស់អ្នក
  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, W5500_CS_PIN);

  // ៣. Start Initialize Ethernet
  Ethernet.init(W5500_CS_PIN);

  // ដាស់ឈីបឱ្យភ្ញាក់ និងស្គាល់ Hardware ជាមុនសិន ដោយប្រើ IP ទទេ (មិនទាន់ធ្វើ DHCP ទេ)
  Ethernet.begin(mac, IPAddress(0, 0, 0, 0));

  // ឆែកមើលតើមានឈីប W5500 ពិតប្រាកដមែនឬអត់
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    Serial.println("❌ W5500 hardware not found!");
    isEthernetConnected = false;
    return;
  }

  Serial.println("🔎 Waiting for Ethernet PHY Link (up to 3 seconds)...");
  // រង់ចាំប្រហែល ៣ វិនាទី ដើម្បីឱ្យ W5500 PHY ចរចា (Auto-negotiate) ជាមួយ Router
  unsigned long waitLink = millis();
  while (Ethernet.linkStatus() != LinkON)
  {
    if (millis() - waitLink > 3000)
    {
      Serial.println("⚠️ Ethernet cable is not plugged in (LinkOFF)! Skipping DHCP.");
      isEthernetConnected = false;
      return;
    }
    delay(100);
  }

  Serial.println("🌐 Cable detected. Initializing W5500 Ethernet via DHCP...");

  // ៤. ហៅទម្រង់ begin(mac) ដោយកំណត់ Timeout ត្រឹមតែ ៥ វិនាទី (ជំនួសឱ្យ ៦០ វិនាទី)
  if (Ethernet.begin(mac, 5000, 2000) == 0)
  {
    Serial.println("❌ Ethernet DHCP failed!");
    isEthernetConnected = false;
  }
  else
  {
    // បង្ខំកំណត់ DNS ទៅ Google 8.8.8.8 ដើម្បីជួយ Blynk កុំឱ្យ DNS Failed
    IPAddress googleDNS(8, 8, 8, 8);
    Ethernet.setDnsServerIP(googleDNS);

    Serial.print("✅ Ethernet connected! My IP: ");
    Serial.println(Ethernet.localIP());
    isEthernetConnected = true;
  }
}

void checkEthernetStatus()
{
  static unsigned long lastReconnect = 0;
  static int retryCount = 0;
  static unsigned long coolDownTime = 15000;

  // ឆែកមើលជានិច្ច! កុំរង់ចាំ Cooldown ពេលដកខ្សែ ព្រោះបើយើងទុកចោល Blynk វានឹងរុញ Data ចូល Socket ងាប់ ធ្វើឱ្យគាំង CPU រាប់វិនាទី
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
    return;

  if (Ethernet.linkStatus() != LinkON)
  {
    if (isEthernetConnected)
    {
      Serial.println("⚠️ Ethernet cable disconnected!");
      isEthernetConnected = false;
      // ផ្តាច់ Socket ចាស់ចោលភ្លាមៗ ដើម្បីកុំឱ្យ Blynk គាំងពេលព្យាយាមបញ្ជូន Data ទៅកាន់អ៊ិនធឺណិតដែលដាច់
      // មិនចាំបាច់ហៅ stop() ទេ ព្រោះ WiFiClient នឹងត្រូវយកមកជំនួសភ្លាមៗ។ តែបើនៅតែគាំង គឺមកពី Blynk ប្រើពេលដើម្បីប្តូរ Client
    }
    return; // ដរាបណាអត់ដោតខ្សែ មិនបាច់ចុះទៅក្រោមទៀតទេ
  }

  // ពេលនេះគឺ LinkON (មានដោតខ្សែ) តែប្រព័ន្ធអត់ទាន់មាន IP
  if (!isEthernetConnected)
  {
    if (millis() - lastReconnect > coolDownTime)
    {
      lastReconnect = millis();
      if (retryCount < 3)
      {
        retryCount++;
        Serial.printf("🔄 LAN Cable detected! Reconnecting Ethernet (Try %d/3)...\n", retryCount);

        if (Ethernet.begin(mac, 5000, 2000) == 1)
        {
          IPAddress googleDNS(8, 8, 8, 8);
          Ethernet.setDnsServerIP(googleDNS);
          Serial.print("✅ Ethernet reconnected! My IP: ");
          Serial.println(Ethernet.localIP());
          isEthernetConnected = true;
          coolDownTime = 15000;
          retryCount = 0;
        }
        else
        {
          Serial.println("❌ Ethernet Reconnect failed!");
        }
      }
      else
      {
        Serial.println("⏳ Ethernet DHCP failed 3 times. Entering 5-minute cooldown...");
        coolDownTime = 300000;
        retryCount = 0;
      }
    }
  }
  else
  {
    // Reset Cooldown ពេលដំណើរការធម្មតា
    coolDownTime = 15000;
    retryCount = 0;
  }
}

#endif