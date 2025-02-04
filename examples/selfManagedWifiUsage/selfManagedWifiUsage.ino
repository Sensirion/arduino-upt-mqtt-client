#include "MqttMailingService.h"
#include <Arduino.h>
#include <WiFi.h>

/*
    In this usage example, the main application is managing the Wi-Fi connection
    itself.
    Once connection is established, mqttMailingService.start() can be called.
*/

MqttMailingService mqttMailingService;
QueueHandle_t mailbox;
MQTTMessage mail;
int count = 0;

// Configuration
const char* ssid = "ap-name";
const char* password = "ap-pass.";
const char* broker_uri = "mqtt://mqtt.someserver.com:1883";

const char ssl_cert[] =
    "------BEGIN CERTIFICATE-----\nmy-certificate\n-----END CERTIFICATE-----";

void setup() {
    Serial.begin(115200);
    sleep(1);

    // Configure the MQTT mailing service
    mqttMailingService.setBrokerURI(broker_uri);
    // [Optional] set a prefix that will be prepended to the topic defined in the messages
    mqttMailingService.setGlobalTopicPrefix("myPrefix/id2345/");

    // [Optional] Uncomment next line for SSL connection
    // mqttMailingService.setSslCertificate(ssl_cert);

    mailbox = mqttMailingService.getMailbox();

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (!WiFi.isConnected()) {
        Serial.println("Waiting on the Wi-Fi connection...");
        sleep(1);
    }
    Serial.println("Connected to Wi-Fi AP !");

    // Wait until MQTT service initialized
    mqttMailingService.start();

    Serial.println("setup() complete.");
}

void loop() {
    // Send a message
    char msg[16];
    sprintf(msg, "Message #%i", count);
    mqttMailingService.sendTextMessage(msg, "topic/something/");

    count++;
    delay(1000);
}