# ALL-ESP8266-IoT-Relais
New Firmware that supports MQTT for ALL-ESP8266-IoT-Relais

This is a alternative Firmware for the 4duino IoT WLAN Relais ESP8266-UP-Relais provided by ALLNET.

Based on ESP8266 and can easy used for other actuators, configurable by integrated Web-Server

Support fallowing mechanism for switching the onboard Relais

- switching by URL  http://[IP-Address]/html/switch.html?cmd=1 for ON or
                    http://[IP-Address]/html/switch.html?cmd=0 for OFF
- using Web-Sockets on http://[IP-Address]/ws possible Values 1/0 for ON/OFF
- connect to a MQTT-Broker, 
        Status will be published to Topic [DeviceId]/state
        Commands will subscribed at [DeviceId]/cmd, possible Values 1/0 for ON/OFF
        the current IP-Addrress will published to Topic [DeviceId]/ip
