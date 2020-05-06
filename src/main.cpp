//
// A simple server implementation showing how to:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
//

#include <Arduino.h>
#ifdef ESP32
#include <FS.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <wificfg.h>
#include <mqttconfig.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include "webserver.h"
#include "mqtthelper.h"

#define SPIFFS_USE_MAGIC

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");
wificonfig wifiConfig;

String SubscribePath;
String PublishPath;

const char* PARAM_MESSAGE = "cmd";



#define MQTT_HOST IPAddress(192, 168, 1, 10)
#define MQTT_PORT 1883

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;
mqttconfig mqttcfg;

void switchCoil(bool on);
bool GetState();
const char* StateValue();


String indexProcessor(const String& var);

void switchCoil(bool on)
{
  if(on)
  {
    digitalWrite(D6,LOW);
    digitalWrite(D5,HIGH);
  }
  else
  {
    digitalWrite(D5,LOW);
    digitalWrite(D6,HIGH);
  }
  mqttClient.publish(PublishPath.c_str(),1,false,StateValue());
}

bool GetState()
{
  if(digitalRead(D5)==0)
  {
    return false;
  }
  else
  {
    return true;
  }
  
}

const char* StateValue()
{
  if(GetState())
    return "1";
  else
    return "0";
}

void setup() {

    Serial.begin(115200);
    pinMode(12,OUTPUT);
    pinMode(14,OUTPUT);
    switchCoil(false);
    // WiFi.mode(WIFI_STA);
    // WiFi.begin(ssid, password);
    // if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    //     Serial.printf("WiFi Failed!\n");
    //     return;
    // }

    // Serial.print("IP Address: ");
    // Serial.println(WiFi.localIP());

    registerMqttEvents();

    setupOTA();

    Serial.println("Mounting FS...");
    if (!SPIFFS.begin()) {
      Serial.println("Failed to mount file system");
    return;
    }
    //wifiConfig.begin();
    mqttcfg.load();
    mqttcfg.Enabled=true;
    connectToWifi();

    MDNS.addService("http","tcp",80);
    ArduinoOTA.setHostname(wifiConfig.Name.c_str());
    ArduinoOTA.begin();

    registerWebEvents();

    // Register Wepages and Handler

    //Move client to /html/index.html
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->redirect("/html/index.html");
    });

    server.on("/html/index.html",HTTP_GET,[](AsyncWebServerRequest *request){
      request->send(SPIFFS,"/html/index.html",String(),false,indexProcessor);
    });

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // Send a GET request to <IP>/html/switch?message=<message>
    server.on("/html/switch", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam(PARAM_MESSAGE)) {
            message = request->getParam(PARAM_MESSAGE)->value();
        } else {
            message = "";
        }
        if(message!="0" && message!="1")
        {
          request->send(403,"text/plain","unknown or malformed parameter. ?cmd=1 or ?cmd=0");
        }
        else
        {
          if(message=="1" && GetState()==false)
          {
            switchCoil(true);
            request->send(200,"text/plain","new State ON");
          }
          else if(message=="0" && GetState()==true)
          {
            switchCoil(false);
            request->send(200,"text/plain","New State OFF");
          }
          else
          {
            request->send(201);
          }
          
        }
        
        request->send(200, "text/plain", "Hello, GET: " + message);
    });

    // Send a POST request to <IP>/post with a form field message set to <message>
    server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request){
        String message;
        if (request->hasParam(PARAM_MESSAGE, true)) {
            message = request->getParam(PARAM_MESSAGE, true)->value();
        } else {
            message = "No message sent";
        }
        request->send(200, "text/plain", "Hello, POST: " + message);
    });

    server.onNotFound(notFound);

    server.begin();
}

void loop() {
  ArduinoOTA.handle();
  ws.cleanupClients();

}


String indexProcessor(const String& var)
{
  if(var == "TemplateDeviceId")
  {
    return wifiConfig.Name;
  }
  if(var == "TemplateIpAddress")
  {
    return WiFi.localIP().toString();
  }
  if(var == "TemplateMacAddress")
  {
    return WiFi.macAddress();
  }
  return String("");
}

