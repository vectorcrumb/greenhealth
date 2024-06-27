#ifndef DEFINITIONS_HPP_
#define DEFINITIONS_HPP_

#include <stdint.h>
#include <string>

const char *mqtt_broker = "greenhealth.local";
const int mqtt_port = 1883;
const char *mqtt_username = "device";
const char *mqtt_password = "goodlife";

const char *mqtt_client_id_templ = "dev-%s";
char mqtt_client_id[32];
const char *mqtt_topic_register = "register";
const char *mqtt_topic_pub_skills_templ = "%s/skills";
char mqtt_topic_pub_skills[64];
const char *mqtt_topic_sub_command_templ = "%s/command";
char mqtt_topic_sub_command[64];

#endif