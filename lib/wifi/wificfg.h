#ifndef WIFI_HELPER
#define WIFI_HELPER

#include <Arduino.h>
#ifdef ESP32
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

class wificonfig
{
    public:
    String SSID;
    String Passwd;
    String Name=String("ESP-");
    bool ApMode=false;
    String ChipId;
    void begin();
    void reset();
    void load();
    void save();
    wificonfig();
    private:
    char * buff;
};

#endif