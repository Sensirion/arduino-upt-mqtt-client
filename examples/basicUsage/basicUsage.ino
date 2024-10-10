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
const char* broker_uri = "mqtt://mqtt.some-server.com:1883";

const char ssl_cert[] =
    "------BEGIN CERTIFICATE-----\nmy-certificate\n-----END CERTIFICATE-----";

void setup() {
    Serial.begin(115200);
    sleep(1);

    // Configure the MQTT mailing service
    mqttMailingService.setBrokerURI(broker_uri);

    // Uncomment next line for SSL connection
    // mqttMailingService.setSslCertificate(ssl_cert);

    mailbox = mqttMailingService.getMailbox();

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (!WiFi.isConnected()) {
        Serial.println("Waiting on the Wi-Fi connection");
        sleep(1);
    }
    Serial.println("Connected to Wi-Fi AP !");

    // Wait until MQTT service initialized
    mqttMailingService.start();
    while (mqttMailingService.getServiceState() ==
           MqttMailingServiceState::UNINITIALIZED) {
        Serial.println("MQTT mailing service is initializing...");
        sleep(1);
    }

    Serial.println("setup() complete.");
}

void loop() {
    // Set payload and topic
    sprintf(mail.topic, "testtopic/");

    sprintf(mail.payload, "Msg #%i", count);

    // Dispatch message
    if (xQueueSend(mailbox, &mail, 0) != pdPASS) {
        Serial.println("ERROR:\tMailbox is full, mail is lost.");
    } else {
        Serial.println("Message successfully added to mailbox");
    }

    count++;
    delay(1000);
}