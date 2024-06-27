#include <Arduino.h>

#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

#include "definitions.hpp"

void mqtt_callback(char *topic, byte *payload, unsigned int length);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void setup()
{
  // Setup Serial
  Serial.begin(115200);
  // Setup WiFi
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  if (!wm.autoConnect("Goodhealth-Rover", "goodlife"))
  {
    Serial.println(F("Failed to connect. Restarting."));
    ESP.restart();
  }
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println(F("Connecting to WiFi.."));
  }
  // Setup variables
  String mac_addr = WiFi.macAddress();
  mac_addr.replace(":", "");
  snprintf(mqtt_client_id, sizeof(mqtt_client_id), mqtt_client_id_templ, mac_addr.c_str());
  snprintf(mqtt_topic_pub_skills, sizeof(mqtt_topic_pub_skills), mqtt_topic_pub_skills_templ, mqtt_client_id);
  snprintf(mqtt_topic_sub_command, sizeof(mqtt_topic_sub_command), mqtt_topic_sub_command_templ, mqtt_client_id);
  // Setup MQTT
  mqttClient.setServer(mqtt_broker, mqtt_port);
  mqttClient.setCallback(mqtt_callback);
  while (!mqttClient.connected())
  {
    Serial.printf("The client %s connects to the public MQTT broker\n", mqtt_client_id);
    if (mqttClient.connect(mqtt_client_id, mqtt_username, mqtt_password))
    {
      Serial.println(F("MQTT broker connected"));
    }
    else
    {
      Serial.print(F("failed with state "));
      Serial.print(mqttClient.state());
      delay(2000);
    }
  }
  // Publish to registration topic
  mqttClient.publish(mqtt_topic_register, mqtt_client_id);
  mqttClient.subscribe(mqtt_topic_sub_command, 1);
}

void loop()
{
  // put your main code here, to run repeatedly:
  mqttClient.loop();
}

void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print(F("Message arrived in topic: "));
  Serial.println(topic);
  Serial.print(F("Message:"));
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
}