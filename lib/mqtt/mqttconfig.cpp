#include "mqttconfig.h"
#include <ArduinoJson.h>

const size_t capacity = JSON_OBJECT_SIZE(3) + 60;

mqttconfig::mqttconfig(/* args */)
{
}

mqttconfig::~mqttconfig()
{
}

void mqttconfig::load()
{
    DynamicJsonDocument doc(capacity);
    File file=SPIFFS.open("/mqtt.json","r");
    if(!file)
    {
        Serial.println("mqtt.json does not exist, load defaults");
        ServerAddress="192.168.178.10";
        ServerPort=1883;
        Enabled=false;
    }
    else
    {
        size_t size=file.size();
        Serial.print("FileSize: ");
        Serial.println(size);
        if(size==0)
        {
            Serial.println("mqtt.json is empty, load defaults");
        ServerAddress="192.168.178.10";
        ServerPort=1883;
        Enabled=false;
        }
        Serial.println("Reading file...");
        DeserializationError err=deserializeJson(doc,file);
        Serial.println("Closing file...");
        file.close();
        if(err!=DeserializationError::Code::Ok)
        {
            Serial.println("Unable to read Config. load defaults");
            ServerAddress="192.168.178.10";
            ServerPort=1883;
            Enabled=false;
            save();
        }
        else
        {
            Serial.println("Setup informations from file...");
            ServerAddress=doc["ServerAddress"] | "192.168.178.10";
            ServerPort=doc["ServerPort"] | 1883;
            Enabled=doc["Enabled"] | false;
        }
    }

}

void mqttconfig::save()
{
    DynamicJsonDocument doc(capacity);
    File file=SPIFFS.open("/mqtt.json","c");
    if(!file)
    {
        Serial.println("mqtt.json does not exist");
    }
    doc["ServerAddress"]=ServerAddress;
    doc["ServerPort"]=ServerPort;
    doc["Enabled"]=Enabled;
    serializeJson(doc,file);
    file.close();

}

void mqttconfig::reset()
{
    ServerAddress="192.168.178.10";
    ServerPort=1883;
    Enabled=false;
    save();
}
