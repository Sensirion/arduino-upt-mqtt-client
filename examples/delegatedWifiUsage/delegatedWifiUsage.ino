#include "MQTTManager.h"
#include <Arduino.h>

/*
    In this usage example, the MqttMailingService is managing the Wi-Fi itself.
*/

MqttMailingService mqttMailman;
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

    // Configure the MQTT Mailman service
    mqttMailman.setBrokerURI(broker_uri);
    mailbox = mqttMailman.getMailbox();
    // mqttMailman.setSslCertificate(ssl_cert); // Uncomment for SSL connection

    mqttMailman.startWithDelegatedWiFi(ssid, password);
    while (!WiFi.isConnected()) {
        // Wait for Wi-Fi connection, since MQTT mailing service
        // can't establish a connection without Wi-Fi.
        Serial.println("Waiting for Wi-Fi connection...");
        sleep(1);
    }

    // Wait until MQTT service connects to broker
    while (mqttMailman.getServiceState() !=
           MqttMailingServiceState::CONNECTED) {
        Serial.println("MQTT mailing service is connecting...");
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