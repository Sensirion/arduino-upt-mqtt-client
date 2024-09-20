#include "MQTTManager.h"
#include <Arduino.h>

/*
    In this usage example the main application is managing the WiFi connection
    itself. Once connection is established mqttMailman.start() can be called.
*/

MqttMailingService mqttMailman;
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

    // Configure the MQTT Mailman service
    mqttMailman.setBrokerURI(broker_uri);
    // mqttMailman.setSslCertificate(ssl_cert); // Uncomment for SSL connection
    mailbox = mqttMailman.getMailbox();

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("Waiting on the WiFi connection");
        sleep(1);
    }
    Serial.println("Connected to WiFi AP !");

    // Wait until MQTT service initialized
    mqttMailman.start();
    while (mqttMailman.getServiceState() ==
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