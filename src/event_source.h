/* This file contains all event-related definitions */

#ifndef _EVENT_SOURCE_H_
#define _EVENT_SOURCE_H_

// Wifi event group
#define WIFI_GOT_IP_BIT BIT0
#define WIFI_LOST_IP_BIT BIT1

// MQTT event group
#define MQTT_CLIENT_TASK_NOT_RUNNING BIT0

struct MQTTMessage {
    char topicSuffix[128];    // MQTT Topic
    char payload[256];  // MQTT Payload
    int len = 0;  // Payload size in bytes. If payload is a string, this value
                  // is automatically computed from the string length if let 0.
};

#endif /* _EVENT_SOURCE_H_ */
