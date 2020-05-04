#include "wificfg.h"
#include <ArduinoJson.h>
#define Max_Connects    10

const size_t capacity = JSON_OBJECT_SIZE(2) + 70;

wificonfig::wificonfig()
{
    chipId=String(ESP.getChipId());
    chipId=String("ESP_")+chipId;
}

void wificonfig::begin()
{
    load();
    if(SSID=="")
    {
        WiFi.softAP(chipId.c_str());
        ApMode=true;
    }
    else
    {
        for(int i=0;i<Max_Connects;i++)
        {
            Serial.printf("Wifi %i try...\n",i);
            WiFi.begin(SSID, Passwd);
            if (WiFi.waitForConnectResult() != WL_CONNECTED) {
                WiFi.disconnect(false);
                delay(1000);
            }
            else
            {
                break;
            }
            
        }
        if(WiFi.status() != WL_CONNECTED)
        {
            Serial.printf("Failed to connect %s and %s. Starting Soft-AP Mode",SSID.c_str(),Passwd.c_str());
            WiFi.softAP(chipId.c_str());
            ApMode=true;
        }
        
        Serial.print("Ip-Address: ");
        Serial.println(WiFi.localIP());
    }
    
}

void wificonfig::reset()
{
    SSID="";
    Passwd="";
    Name=chipId.c_str();
    save();
}

void wificonfig::load()
{
    DynamicJsonDocument doc(capacity);

    File file=SPIFFS.open("/wifi.json","r");
    if(!file)
    {
        Serial.println("wifi.json does not exist, load defaults");
        SSID="";
        Passwd="";
        Name=chipId.c_str();
    }
    else
    {
        size_t size=file.size();
        Serial.print("FileSize: ");
        Serial.println(size);
        if(size==0)
        {
            Serial.println("wifi.json is empty, load defaults");
            SSID="";
            Passwd="";
            Name=chipId.c_str();
        }
        Serial.println("Reading file...");
        DeserializationError err=deserializeJson(doc,file);
        Serial.println("Closing file...");
        file.close();
        if(err!=DeserializationError::Code::Ok)
        {
            Serial.println("Unable to read Config. load defaults");
            SSID="";
            Passwd="";
            Name=chipId.c_str();
            save();
        }
        else
        {
            Serial.println("Setup informations from file...");
            SSID=doc["SSID"] | "";
            Passwd=doc["Passwd"] | "";
            Name=doc["Name"] | chipId.c_str();
        }
    }
}

void wificonfig::save()
{
    DynamicJsonDocument doc(capacity);
    File file=SPIFFS.open("/wifi.json","c");
    if(!file)
    {
        Serial.println("wifi.json does not exist");
    }
    doc["SSID"]=SSID;
    doc["Passwd"]=Passwd;
    doc["Name"]=Name;
    serializeJson(doc,file);
    file.close();
}
