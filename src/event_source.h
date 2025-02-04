/* This file contains all event-related definitions */

#ifndef _EVENT_SOURCE_H_
#define _EVENT_SOURCE_H_

// Wifi event group
#define WIFI_GOT_IP_BIT BIT0
#define WIFI_LOST_IP_BIT BIT1

// MQTT event group
#define MQTT_CLIENT_TASK_NOT_RUNNING BIT0

// MQTT Message sizes
#define MQTT_MESSAGE_TOPIC_SUFFIX_SIZE 128
#define MQTT_MESSAGE_PAYLOAD_SIZE 256


struct MQTTMessage {
    char topicSuffix[MQTT_MESSAGE_TOPIC_SUFFIX_SIZE];    // MQTT Topic
    char payload[MQTT_MESSAGE_PAYLOAD_SIZE];  // MQTT Payload
    int len = 0;  // Payload size in bytes. If payload is a string, this value
                  // is automatically computed from the string length if let 0.
};

#endif /* _EVENT_SOURCE_H_ */
