#ifndef MQTT_HELPER
#define MQTT_HELPER
#include <Arduino.h>
#ifdef ESP32
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

class mqttconfig
{
private:
    /* data */
public:
    String ServerAddress="192.168.1.10";
    uint16_t ServerPort=1883;
    bool Enabled=false;

mqttconfig(/* args */);
    ~
mqttconfig();
    void load();
    void save();
    void reset();
};




#endif