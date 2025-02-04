#include "MqttMailingService.h"
#include <Arduino.h>

/*
    In this usage example, the MqttMailingService is managing the Wi-Fi itself.
*/

MqttMailingService mqttMailingService;
QueueHandle_t mailbox;
MQTTMessage mail;
int count = 0;

// Configuration
const char* ssid = "ap-name";
const char* password = "ap-pass.";
const char* broker_uri = "mqtt://mqtt.yourserver.com:1883";
const char ssl_cert[] =
    "------BEGIN CERTIFICATE-----\nmy-certificate\n-----END CERTIFICATE-----";

void setup() {
    Serial.begin(115200);
    sleep(1);

    // Configure the MQTT mailing service
    mqttMailingService.setBrokerURI(broker_uri);
    // [Optional] set a prefix that will be prepended to the topic defined in the messages
    mqttMailingService.setGlobalTopicPrefix("myPrefix/id2345/");
    mailbox = mqttMailingService.getMailbox();

    // [Optional] Uncomment next line for SSL connection
    // mqttMailingService.setSslCertificate(ssl_cert);

    Serial.println("MQTT Mailing Service starting and connecting ....");

    // A blocking start ensure that connection is established before starting
    // sending messages.
    bool blockingStart = true;
    mqttMailingService.startWithDelegatedWiFi(ssid, password, blockingStart);

    Serial.println("MQTT Mailing Service started and connected !");
}

void loop() {
    // Send a message
    char msg[16];
    sprintf(msg, "Message #%i", count);
    mqttMailingService.sendTextMessage(msg, "topic/something/");

    count++;
    delay(1000);
}