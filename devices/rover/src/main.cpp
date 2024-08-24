#include <AccelStepper.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <MQTT.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include <atomic>

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
#define STEPPER_SPEED 1

#define MUTEX_STATE_TIMEOUT 25
#define COMMAND_FEEDBACK_PERIOD_MS 200
#define COMMAND_FEEDBACK_PERIOD_TICKS pdMS_TO_TICKS(COMMAND_FEEDBACK_PERIOD_MS)
#define MAIN_LOOP_PERIOD_MS 20
#define MAIN_LOOP_PERIOD_TICKS pdMS_TO_TICKS(MAIN_LOOP_PERIOD_MS)

void mqtt_callback(MQTTClient *client, char *topic, char *payload, int length);

WiFiClient wifiClient;
MQTTClient mqttClient;
AccelStepper stepper(AccelStepper::DRIVER, STEPPER_STP_PIN, STEPPER_DIR_PIN);

StaticSemaphore_t mutex_buffer_state;
SemaphoreHandle_t mutex_state;

volatile CurrentAction current_action;
std::atomic_bool connection_established;
std::atomic_bool action_in_progress;
std::atomic_bool action_completed;
volatile std::atomic_int32_t nodes_detected;
volatile std::atomic_bool detected_hall_sensor;
long action_start_time = 0;
TickType_t action_feedback_time_marker = 0;

void IRAM_ATTR hall_isr() {
  detected_hall_sensor = true;
}

void setup() {
  connection_established = false;
  action_in_progress = false;
  action_completed = false;
  detected_hall_sensor = false;
  nodes_detected = 0;
  // Create mutex
  mutex_state = xSemaphoreCreateMutexStatic(&mutex_buffer_state);
  // Configure stepper
  stepper.setEnablePin(STEPPER_ENA_PIN);
  stepper.setMaxSpeed(STEPPER_MAX_STEPS_PER_SEC);
  stepper.setAcceleration(STEPPER_MAX_STEPS_PER_SEC_PER_SEC);
  // Configure sensor. Configure as INPUT or INPUT_PULLUP accordingly to sensor
  pinMode(HALL_SENSOR_PIN, INPUT);
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
  // Attach the hardware interrupt to the hall sensor. Configure as RISING or FALLING
  attachInterrupt(digitalPinToInterrupt(HALL_SENSOR_PIN), hall_isr, FALLING);
}

void loop() {
  TickType_t start_loop_time = xTaskGetTickCount();
  // Let's copy the current state variables so that other tasks don't overwrite these during the loop
  bool action_completed_current, action_in_progress_current;
  if (xSemaphoreTake(mutex_state, pdMS_TO_TICKS(MUTEX_STATE_TIMEOUT)) == pdTRUE) {
    action_completed_current = action_completed;
    action_in_progress_current = action_in_progress;
  } else {
    // If we can't sample the state variables, skip this iteration
    ESP_LOGE("MUTEX", "COULD NOT CAPTURE MUTEX @ %s:%d", __func__, __LINE__);
    return;
  }
  // Loop the mqtt client to handling incoming messages
  mqttClient.loop();
  // Loop the motor
  stepper.run();
  // Detect if we've just finished a previously in progress action
  if (action_completed_current && action_in_progress_current) {
    // Publish the completion to the result topic with how long it took to complete
    long total_time_ms = pdTICKS_TO_MS(xTaskGetTickCount()) - action_start_time;
    snprintf(mqtt_msg_cmd_res, sizeof(mqtt_msg_cmd_res), mqtt_msg_cmd_res_templ, btoa(true), total_time_ms);
    mqttClient.publish(mqtt_topic_cmd_res, mqtt_msg_cmd_res, false, 1);
    // Reset state. Delete the current action!
    action_completed = true;
    action_in_progress = false;
    current_action.action = RoverAction::STANDBY;
  }
  // Detect if we just started an action to effect a command on the stepper motor
  else if (current_action.action > RoverAction::STANDBY && !(action_completed_current && action_in_progress_current)) {
    // Start an action
    action_in_progress = true;
    if (stepper.isRunning()) {
      ESP_LOGI("MAIN", "Stepper is already running!");
    }
    // Decide what to do
    else if (current_action.action == RoverAction::MOVE_MM) {
      stepper.move(current_action.parameter);
    } else if (current_action.action == RoverAction::MOVE_TO_NEXT_NODE) {
      stepper.setSpeed(STEPPER_SPEED);
    } else if (current_action.action == RoverAction::MOVE_N_NODES) {
      stepper.setSpeed(STEPPER_SPEED);
    }
  }
  // Detect if we're progressing an action to publish feedback
  else if (action_in_progress_current) {
    // If enough time has elapsed for an update, compute current running time and publish
    if (xTaskGetTickCount() - action_feedback_time_marker > COMMAND_FEEDBACK_PERIOD_TICKS) {
      long current_time_ms = pdTICKS_TO_MS(xTaskGetTickCount()) - action_start_time;
      snprintf(MSG(cmd_fb), sizeof(MSG(cmd_fb)), MSG_TEMPL(cmd_fb), current_time_ms);
      mqttClient.publish(TOPIC(cmd_fb), MSG(cmd_fb), false, 1);
      action_feedback_time_marker = xTaskGetTickCount();
    }
    // Check actions one by one
    // move_mm will finish automatically, let's end the action if we're done moving
    if (current_action.action == RoverAction::MOVE_MM) {
      if (!stepper.isRunning()) {
        action_completed = true;
      }
    }
    // move_to_next_node finishes on detection of hall effect sensor
    else if (current_action.action == RoverAction::MOVE_TO_NEXT_NODE) {
      if (detected_hall_sensor) {
        stepper.stop();
        action_completed = true;
        detected_hall_sensor = false;
      }
    }
    // move_n_nodes finishes on sensor + having detected n nodes
    else if (current_action.action == RoverAction::MOVE_N_NODES) {
      if (detected_hall_sensor) {
        nodes_detected++;
        // Check if we've reached the n nodes
        if (current_action.parameter - nodes_detected == 0) {
          stepper.stop();
          action_completed = true;
          detected_hall_sensor = false;
          nodes_detected = 0;
        }
      }
    }
  }
  // We determine the remaining loop time
  TickType_t loop_time = xTaskGetTickCount() - start_loop_time;
  if (loop_time < MAIN_LOOP_PERIOD_TICKS) {
    vTaskDelay(MAIN_LOOP_PERIOD_TICKS - loop_time);
  } else {
    ESP_LOGW("MAIN", "Loop time: %d ticks. Expected loop time: %d ticks", loop_time, MAIN_LOOP_PERIOD_TICKS);
  }
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
        // Retrieve the mm and timeout_ms parameters from the move_mm payload
        JsonDocument doc;
        deserializeJson(doc, payload, length);
        current_action.parameter = doc["mm"];
        current_action.timeout_ms = doc["timeout_ms"];
        current_action.action = RoverAction::MOVE_MM;
      } else if (strcmp(token, "move_to_next_node") == 0) {
        // Retrieve the timeout_ms parameters from the move_to_next_node payload
        JsonDocument doc;
        deserializeJson(doc, payload, length);
        current_action.parameter = 0;
        current_action.timeout_ms = doc["timeout_ms"];
        current_action.action = RoverAction::MOVE_TO_NEXT_NODE;
      } else if (strcmp(token, "move_n_nodes") == 0) {
        // Retrieve the n and timeout_ms parameters from the move_n_nodes payload
        JsonDocument doc;
        deserializeJson(doc, payload, length);
        current_action.parameter = doc["n"];
        current_action.timeout_ms = doc["timeout_ms"];
        current_action.action = RoverAction::MOVE_N_NODES;
      }
      // Configure state
      action_in_progress = false;
      action_completed = false;
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