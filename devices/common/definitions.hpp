#ifndef DEFINITIONS_HPP_
#define DEFINITIONS_HPP_

#include <stdint.h>
#include <string>

#define btoa(x) ((x) ? "true" : "false")
#define MQTT_TEMPL(name) name##_templ

#define TOPIC(name) mqtt_topic_##name
#define TOPIC_TEMPL(name) mqtt_topic_##name##_templ

#define MSG(name) mqtt_msg_##name
#define MSG_TEMPL(name) mqtt_msg_##name##_templ

#define TOPIC_MAX_LEN 64
#define MSG_MAX_LEN 128

const char *mqtt_broker = "greenhealth.local";
const int mqtt_port = 1883;
const char *mqtt_username = "device";
const char *mqtt_password = "goodlife";

const char *mqtt_client_id_templ = "dev-%s";
char mqtt_client_id[32];
const char *mqtt_topic_register = "discovery";
// SUB topics
const char *mqtt_topic_ack_templ = "%s/ack";
const char *mqtt_topic_cmd_templ = "%s/cmd";

char mqtt_topic_ack[TOPIC_MAX_LEN];
char mqtt_topic_cmd[TOPIC_MAX_LEN];
// PUB topics
const char *mqtt_topic_skills_templ = "%s/skills";
const char *mqtt_topic_cmd_fb_templ = "%s/cmd/fb";
const char *mqtt_topic_cmd_res_templ = "%s/cmd/res";

char mqtt_topic_skills[TOPIC_MAX_LEN];
char mqtt_topic_cmd_fb[TOPIC_MAX_LEN];
char mqtt_topic_cmd_res[TOPIC_MAX_LEN];
// Messages
const char *mqtt_msg_cmd_fb_templ = "{\"elapsed_ms\":%lu}";
const char *mqtt_msg_cmd_res_templ = "{\"success\":%s\n\"total_time_ms\":%lu}";

char mqtt_msg_cmd_fb[MSG_MAX_LEN];
char mqtt_msg_cmd_res[MSG_MAX_LEN];

#endif