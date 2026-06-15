# 1 "C:\\Users\\leeta\\AppData\\Local\\Temp\\tmpdlqdeipo"
#include <Arduino.h>
# 1 "C:/Users/leeta/.platformio/CommandStation/CommandStation-EX.ino"
# 19 "C:/Users/leeta/.platformio/CommandStation/CommandStation-EX.ino"
#if __has_include ( "config.h")
  #include "config.h"
  #ifndef MOTOR_SHIELD_TYPE
  #error Your config.h must include a MOTOR_SHIELD_TYPE definition. If you see this warning in spite not having a config.h, you have a buggy preprocessor and must copy config.example.h to config.h
  #endif
#else
  #warning config.h not found. Using defaults from config.example.h
  #include "config.example.h"
#endif
# 52 "C:/Users/leeta/.platformio/CommandStation/CommandStation-EX.ino"
#include "DCCEX.h"
#include "Display_Implementation.h"

#ifdef CPU_TYPE_ERROR
#error CANNOT COMPILE - DCC++ EX ONLY WORKS WITH THE ARCHITECTURES LISTED IN defines.h
#endif

#ifdef WIFI_WARNING
#warning You have defined that you want WiFi but your hardware has not enough memory to do that, so WiFi DISABLED
#endif
#ifdef ETHERNET_WARNING
#warning You have defined that you want Ethernet but your hardware has not enough memory to do that, so Ethernet DISABLED
#endif
#ifdef EXRAIL_WARNING
#warning You have myAutomation.h but your hardware has not enough memory to do that, so EX-RAIL DISABLED
#endif
void setup();
void loop();
#line 69 "C:/Users/leeta/.platformio/CommandStation/CommandStation-EX.ino"
void setup()
{




  SerialManager::init();

  DIAG(F("License GPLv3 fsf.org (c) dcc-ex.com"));


  IODevice::begin();



  ADCee::begin();

  TrackManager::Setup(MOTOR_SHIELD_TYPE);

  DISPLAY_START (

    LCD(0,F("DCC-EX v%S"),F(VERSION));
    LCD(1,F("Lic GPLv3"));
  );




#ifndef ARDUINO_ARCH_ESP32
#if WIFI_ON
  WifiInterface::setup(WIFI_SERIAL_LINK_SPEED, F(WIFI_SSID), F(WIFI_PASSWORD), F(WIFI_HOSTNAME), IP_PORT, WIFI_CHANNEL, WIFI_FORCE_AP);
#endif
#else

  WifiESP::setup(WIFI_SSID, WIFI_PASSWORD, WIFI_HOSTNAME, IP_PORT, WIFI_CHANNEL, WIFI_FORCE_AP);
#endif

#if ETHERNET_ON
  EthernetInterface::setup();
#endif


  DCC::begin();


  RMFT::begin();




  #if __has_include ( "mySetup.h")
    #define SETUP(cmd) DCCEXParser::parse(F(cmd))
    #include "mySetup.h"
    #undef SETUP
  #endif

  #if defined(LCN_SERIAL)
  LCN_SERIAL.begin(115200);
  LCN::init(LCN_SERIAL);
  #endif
  LCD(3, F("Ready"));
  CommandDistributor::broadcastPower();
}

void loop()
{




  #if !nanoLite
  DCC::loop();


 SerialManager::loop();
#endif

#ifndef ARDUINO_ARCH_ESP32
#if WIFI_ON
  WifiInterface::loop();
#endif
#else
#ifndef WIFI_TASK_ON_CORE0
  WifiESP::loop();
#endif
#endif
#if ETHERNET_ON
  EthernetInterface::loop();
#endif

  RMFT::loop();

  #if defined(LCN_SERIAL)
  LCN::loop();
  #endif


  DisplayInterface::loop();


  IODevice::loop();

  Sensor::checkAll();


  static int ramLowWatermark = __INT_MAX__;

  int freeNow = DCCTimer::getMinimumFreeMemory();
  if (freeNow < ramLowWatermark) {
    ramLowWatermark = freeNow;
    LCD(3,F("Free RAM=%5db"), ramLowWatermark);
  }
}