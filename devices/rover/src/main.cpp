#include <Arduino.h>

#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <AccelStepper.h>

#include "definitions.hpp"

#define STEPPER_STP_PIN 1
#define STEPPER_DIR_PIN 1
#define STEPPER_ENA_PIN 1
#define HALL_SENSOR_PIN 1

#define STEPPER_STEPS_PER_REV 1
#define STEPPER_MAX_STEPS_PER_SEC 1
#define STEPPER_MAX_STEPS_PER_SEC_PER_SEC 1

void mqtt_callback(char *topic, byte *payload, unsigned int length);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
AccelStepper stepper(AccelStepper::DRIVER, STEPPER_STP_PIN, STEPPER_DIR_PIN);

void setup()
{
  // Configure stepper
  stepper.setEnablePin(STEPPER_ENA_PIN);
  stepper.setMaxSpeed(STEPPER_MAX_STEPS_PER_SEC);
  stepper.setAcceleration(STEPPER_MAX_STEPS_PER_SEC_PER_SEC);
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
  // Setup topic variables
  // TODO: Reimplement function directly to avoid call to String class
  String mac_addr = WiFi.macAddress();
  mac_addr.replace(":", "");
  snprintf(mqtt_client_id, sizeof(mqtt_client_id), mqtt_client_id_templ, mac_addr.c_str());
  snprintf(mqtt_topic_pub_skills, sizeof(mqtt_topic_pub_skills), mqtt_topic_pub_skills_templ, mqtt_client_id);
  snprintf(mqtt_topic_sub_command, sizeof(mqtt_topic_sub_command), mqtt_topic_sub_command_templ, mqtt_client_id);
  snprintf(mqtt_topic_pub_stage, sizeof(mqtt_topic_pub_stage), mqtt_topic_pub_stage_templ, mqtt_client_id);
  // Setup MQTT
  mqttClient.setServer(mqtt_broker, mqtt_port);
  mqttClient.setCallback(mqtt_callback);
  while (!mqttClient.connected())
  {
    Serial.printf("Connecting to MQTT broker with id %s", mqtt_client_id);
    if (mqttClient.connect(mqtt_client_id, mqtt_username, mqtt_password))
    {
      Serial.println(F("MQTT connected"));
    }
    else
    {
      Serial.print(F("MQTT connection failed with "));
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
  // Establish subscriptions
  mqttClient.subscribe(mqtt_topic_sub_command, 1);
  // Publish to registration topic
  mqttClient.publish(mqtt_topic_register, mqtt_client_id);
}

void loop()
{
  // put your main code here, to run repeatedly:
  mqttClient.loop();
}

void mqtt_callback(char *topic, byte *payload, unsigned int length)
{

  ESP_LOGI("MQTT", "CB T:%s", topic);
  ESP_LOGD("MQTT", "CB MSG: %s", (char *)payload);

  char *token;
  // Call strtok twice to skip device ID and obtain actual topic
  token = strtok(topic, "/");
  token = strtok(topic, "/");

  if (strcmp(token, mqtt_topic_sub_command) == 0)
  {
    // Implement COMMAND call
  }
  else
  {
    ESP_LOGW("MQTT", "CB unrecognized topic %s with msg %s", topic, (char *)payload);
  }
}