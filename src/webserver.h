#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>

extern AsyncWebServer server;
extern AsyncWebSocket ws;
extern AsyncEventSource events;

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);

void setupOTA()
{
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

}

void registerWebEvents()
{
        ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    events.onConnect([](AsyncEventSourceClient *client){
      client->send("hello!",NULL,millis(),1000);
    });
    server.addHandler(&events);

}
//
/*  HELPER   */
//

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

#endif