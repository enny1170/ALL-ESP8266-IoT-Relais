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

#define SPIFFS_USE_MAGIC

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");
wificonfig wifiConfig;

String SubscribePath;
String PublishPath;

const char* PARAM_MESSAGE = "message";

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

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

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  if(mqttcfg.Enabled)
  {
    mqttClient.connect();
  }
}

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
//  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  wifiConfig.begin();
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  SubscribePath=wifiConfig.Name+String("/cmd");
  PublishPath=wifiConfig.Name+String("/state");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
//  uint16_t packetIdSub = mqttClient.subscribe("test/lol", 2);
  Serial.print("Subscribe to: ");
  Serial.println(SubscribePath);
  mqttClient.publish(SubscribePath.c_str(),1,true, StateValue());
  uint16_t packetIdSub = mqttClient.subscribe(SubscribePath.c_str(), 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
  Serial.println(packetIdSub);
//  mqttClient.publish("test/lol", 0, true, "test 1");
  Serial.print("Publish at: ");
  Serial.println(PublishPath);
  mqttClient.publish(PublishPath.c_str(), 1, true, StateValue());
  // Serial.println("Publishing at QoS 0");
  // uint16_t packetIdPub1 = mqttClient.publish("test/lol", 1, true, "test 2");
  // Serial.print("Publishing at QoS 1, packetId: ");
  // Serial.println(packetIdPub1);
  // uint16_t packetIdPub2 = mqttClient.publish("test/lol", 2, true, "test 3");
  // Serial.print("Publishing at QoS 2, packetId: ");
  // Serial.println(packetIdPub2);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  qos: ");
  Serial.println(properties.qos);
  Serial.print("  dup: ");
  Serial.println(properties.dup);
  Serial.print("  retain: ");
  Serial.println(properties.retain);
  Serial.print("  len: ");
  Serial.println(len);
  Serial.print("  index: ");
  Serial.println(index);
  Serial.print("  total: ");
  Serial.println(total);
  Serial.print(" load: ");
  Serial.println(payload);
  Serial.printf("Command received: %c",payload[index]);
  if(payload[index]=='0' && GetState()!=false)
  {
    Serial.println("Switch to Off (0)");
    switchCoil(false);
  }
  else if(payload[index]=='1' && GetState()!=true)
  {
    Serial.println("Switch to On (1)");
    switchCoil(true);
  }
  //mqttClient.publish(PublishPath.c_str(), 1, true, StateValue());
}

void onMqttPublish(uint16_t packetId) {
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

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

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(mqttcfg.ServerAddress.c_str(), mqttcfg.ServerPort);

    //Send OTA events to the browser
    ArduinoOTA.onStart([]() { events.send("Update Start", "ota"); });
    ArduinoOTA.onEnd([]() { events.send("Update End", "ota"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      char p[32];
      sprintf(p, "Progress: %u%%\n", (progress/(total/100)));
      events.send(p, "ota");
    });
    ArduinoOTA.onError([](ota_error_t error) {
      if(error == OTA_AUTH_ERROR) events.send("Auth Failed", "ota");
      else if(error == OTA_BEGIN_ERROR) events.send("Begin Failed", "ota");
      else if(error == OTA_CONNECT_ERROR) events.send("Connect Failed", "ota");
      else if(error == OTA_RECEIVE_ERROR) events.send("Recieve Failed", "ota");
      else if(error == OTA_END_ERROR) events.send("End Failed", "ota");
    });


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
    // ArduinoOTA.setHostname(wifiConfig.Name.c_str());
    // ArduinoOTA.begin();

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    events.onConnect([](AsyncEventSourceClient *client){
      client->send("hello!",NULL,millis(),1000);
    });
    server.addHandler(&events);

    //Move client to /html/index.html
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->redirect("/html/index.html");
    });

    server.on("/html/index.html",HTTP_GET,[](AsyncWebServerRequest *request){
      request->send(SPIFFS,"/html/index.html",String(),false,indexProcessor);
    });

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // Send a GET request to <IP>/get?message=<message>
    server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam(PARAM_MESSAGE)) {
            message = request->getParam(PARAM_MESSAGE)->value();
        } else {
            message = "No message sent";
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

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n",msg.c_str());

      if(info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n",msg.c_str());

      if((info->index + len) == info->len){
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
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

