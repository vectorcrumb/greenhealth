#include <AccelStepper.h>
#include <Arduino.h>
#include <MQTT.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "definitions.hpp"
#include "rover_defs.hpp"

#define STEPPER_STP_PIN 32
#define STEPPER_DIR_PIN 26
#define STEPPER_ENA_PIN 33
#define STEPPER_ALARM_PIN 27
#define HALL_SENSOR_PIN 34

#define STEPPER_STEPS_PER_REV 1
#define STEPPER_MAX_STEPS_PER_SEC 1
#define STEPPER_MAX_STEPS_PER_SEC_PER_SEC 1

#define MUTEX_STATE_TIMEOUT 25
#define COMMAND_FEEDBACK_PERIOD 200
#define COMMAND_FEEDBACK_PERIOD_TICKS pdMS_TO_TICKS(COMMAND_FEEDBACK_PERIOD)

void mqtt_callback(char *topic, byte *payload, unsigned int length);

WiFiClient wifiClient;
MQTTClient mqttClient;
// PubSubClient mqttClient(wifiClient);
AccelStepper stepper(AccelStepper::DRIVER, STEPPER_STP_PIN, STEPPER_DIR_PIN);
RoverAction current_action = RoverAction::NONE;
StaticSemaphore_t mutex_buffer_state;
SemaphoreHandle_t mutex_state;
bool connection_established = false;
bool action_in_progress = false;
bool action_completed = false;
long action_start_time = 0;
TickType_t action_feedback_time_marker = 0;

void setup() {
  // Create mutex
  mutex_state = xSemaphoreCreateMutexStatic(&mutex_buffer_state);
  // Configure stepper
  stepper.setEnablePin(STEPPER_ENA_PIN);
  stepper.setMaxSpeed(STEPPER_MAX_STEPS_PER_SEC);
  stepper.setAcceleration(STEPPER_MAX_STEPS_PER_SEC_PER_SEC);
  // Setup Serial
  Serial.begin(115200);
  // Setup WiFi
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  if (!wm.autoConnect("Goodhealth-Rover", "goodlife")) {
    Serial.println(F("Failed to connect. Restarting."));
    ESP.restart();
  }
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(F("Connecting to WiFi.."));
  }
  // Setup topic variables
  // TODO: Reimplement function directly to avoid call to String class
  String mac_addr = WiFi.macAddress();
  mac_addr.replace(":", "");
  snprintf(mqtt_client_id, sizeof(mqtt_client_id), mqtt_client_id_templ, mac_addr.c_str());
  snprintf(TOPIC(skills), sizeof(TOPIC(skills)), TOPIC_TEMPL(skills), mqtt_client_id);
  snprintf(TOPIC(cmd), sizeof(TOPIC(cmd)), TOPIC_TEMPL(cmd), mqtt_client_id);
  snprintf(TOPIC(cmd_fb), sizeof(TOPIC(cmd_fb)), TOPIC_TEMPL(cmd_fb), mqtt_client_id);
  snprintf(TOPIC(cmd_res), sizeof(TOPIC(cmd_res)), TOPIC_TEMPL(cmd_res), mqtt_client_id);
  snprintf(TOPIC(ack), sizeof(TOPIC(ack)), TOPIC_TEMPL(ack), mqtt_client_id);
  // Setup MQTT
  mqttClient.begin(mqtt_broker, mqtt_port, wifiClient);
  mqttClient.onMessageAdvanced(mqtt_callback);
  while (!mqttClient.connected()) {
    Serial.printf("Connecting to MQTT broker with id %s", mqtt_client_id);
    if (mqttClient.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
      Serial.println(F("MQTT connected"));
    } else {
      Serial.print(F("MQTT connection failed with "));
      Serial.println(mqttClient.lastError());
      delay(2000);
    }
  }
  // Establish subscriptions via wildcard
  mqttClient.subscribe(mqtt_client_id, 1);
  // Publish to registration topic
  mqttClient.publish(mqtt_topic_register, mqtt_client_id, false, 1);
}

void loop() {
  // put your main code here, to run repeatedly:
  mqttClient.loop();
  if (action_completed && action_in_progress) {
    long total_time_ms = pdTICKS_TO_MS(xTaskGetTickCount()) - action_start_time;
    snprintf(mqtt_msg_cmd_res, sizeof(mqtt_msg_cmd_res), mqtt_msg_cmd_res_templ, btoa(true), total_time_ms);
    mqttClient.publish(mqtt_topic_cmd_res, mqtt_msg_cmd_res, false, 1);
    action_in_progress = false;
    action_completed = true;
  } else if (action_in_progress) {
    if (xTaskGetTickCount() - action_feedback_time_marker > COMMAND_FEEDBACK_PERIOD_TICKS) {
      long current_time_ms = pdTICKS_TO_MS(xTaskGetTickCount()) - action_start_time;
      snprintf(MSG(cmd_fb), sizeof(MSG(cmd_fb)), MSG_TEMPL(cmd_fb), current_time_ms);
      mqttClient.publish(TOPIC(cmd_fb), MSG(cmd_fb), false, 1);
      action_feedback_time_marker = xTaskGetTickCount();
    }

    // if (current_action == RoverAction::MOVE_MM) {

    // }
  }

  delay(5);
}

void mqtt_callback(MQTTClient *client, char topic[], char payload[], int length) {
  ESP_LOGI("MQTT", "CB T:%s", topic);
  ESP_LOGD("MQTT", "CB MSG: %s", (char *)payload);

  char *token;
  // Call strtok twice to skip device ID and obtain actual topic
  token = strtok(topic, "/");
  token = strtok(topic, "/");

  if (strcmp(token, TOPIC(cmd)) == 0) {
    ESP_LOGD("MQTT", "Command received");
    if (xSemaphoreTake(mutex_state, pdMS_TO_TICKS(MUTEX_STATE_TIMEOUT)) == pdTRUE) {
      // If we're executing an action currently, ignore new request
      if (action_in_progress) {
        return;
      }
      token = strtok(topic, "/");
      // Check if action is registered and if so, update current action
      if (strcmp(token, "move_mm") == 0) {
        current_action = RoverAction::MOVE_MM;
      } else if (strcmp(token, "move_to_next_node") == 0) {
        current_action = RoverAction::MOVE_TO_NEXT_NODE;
      } else if (strcmp(token, "move_n_nodes") == 0) {
        current_action = RoverAction::MOVE_N_NODES;
      }
      action_in_progress = true;
      action_start_time = pdTICKS_TO_MS(xTaskGetTickCount());
      xSemaphoreGive(mutex_state);
    } else {
      ESP_LOGE("MUTEX", "COULD NOT CAPTURE MUTEX @ %s:%d", __func__, __LINE__);
    }
  } else if (strcmp(token, TOPIC(ack)) == 0) {
    connection_established = true;
    ESP_LOGD("MQTT", "Connection ack'ed");
  } else {
    ESP_LOGW("MQTT", "CB unrecognized topic %s with msg %s", topic, (char *)payload);
  }
}