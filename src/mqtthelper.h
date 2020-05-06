#ifndef MQTT_HELPER_H
#define MQTT_HELPER_H

#include <mqttconfig.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

// external Variables defined in main.cpp
extern AsyncMqttClient mqttClient;
extern Ticker mqttReconnectTimer;

extern WiFiEventHandler wifiConnectHandler;
extern WiFiEventHandler wifiDisconnectHandler;
extern Ticker wifiReconnectTimer;
extern mqttconfig mqttcfg;

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

void registerMqttEvents()
{
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(mqttcfg.ServerAddress.c_str(), mqttcfg.ServerPort);

}
#endif
