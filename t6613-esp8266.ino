#include <stdio.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

WiFiClient wifi;
PubSubClient mqtt(wifi);

char id[9];
char debugtopic[128];
char sensortopic[128];

void setup() {
  Serial.begin(19200);
  WiFi.begin("revspace-pub-2.4ghz", "");
  mqtt.setServer("mosquitto.space.revspace.nl", 1883);
  sprintf(id, "%06x", ESP.getChipId());
  sprintf(debugtopic, "revdebug/co2/%06x", ESP.getChipId());
  sprintf(sensortopic, "revspace/sensors/co2/%06x", ESP.getChipId());
}

void loop() {
  int co2 = 0;
  
  while (!mqtt.connected()) {
    mqtt.connect(id);
    mqtt.publish(debugtopic, "connected");
  }
  mqtt.loop();
  if (WiFi.status() != WL_CONNECTED) ESP.restart();

  Serial.write("\xff\xfe\x02\x02\x03");
  delay(400);

  char buf[5];
  for (int i = 0; i < 5; i++) {
    unsigned int timeout = millis() + 1000;
    while (! Serial.available()) {
      if (millis() > timeout) {
        mqtt.publish(debugtopic, "No response from sensor, restarting");
        ESP.reset();  // XXX this sometimes hangs :(
      }
    }
    buf[i] = Serial.read();
  }

  if (buf[0] != 0xFF || buf[1] != 0xFA || buf[2] != 2) {
    mqtt.publish(debugtopic, "Weird output from sensor, restarting");
    ESP.reset();
  }
  
  co2 = 256 * buf[3] + buf[4];
  
  char message[128];
  sprintf(message, "%d PPM", co2);
  
  if (co2) {
    mqtt.publish(sensortopic, message, 1);
  } else {
    mqtt.publish(debugtopic, "No reading yet");
  }
  delay(4000);
}
