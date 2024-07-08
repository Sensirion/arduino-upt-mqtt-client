#include "MQTTManager.h"
#include <Arduino.h>

/* Shows features of the library:
    -- Connecting to WiFi AP
    -- Connecting to a MQTT broker via it's URI
    -- Dispatching MQTT Messages
*/

WiFiManager wifiMgr;
MQTTMessageDispatcher mqttMailman;
QueueHandle_t mailbox;
int count = 0;
const char ssl_cert[] =
    "------BEGIN CERTIFICATE-----\nmy-certificate\n-----END CERTIFICATE-----";

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }

    wifiMgr.start("mySSID", "myPassword");

    mqttMailman.setBrokerURI("mqtt://mymqttbroker.com:1883");
    // uncomment to configure  a certificate and enable SSL.
    // mqttMailman.setSslCertificate(ssl_cert);
    mqttMailman.start(wifiMgr.getEventGroupHandle());
    mailbox = mqttMailman.getDataEventQueueHandle();
}

// Dump data into payload queue
void loop() {
    MQTTMessage mail;

    // Set payload and topic
    sprintf(mail.topic, "myTopic/");

    switch (count % 4) {
        case 0:
            sprintf(mail.payload, "Roses are red, violets are blue,");
            break;
        case 1:
            sprintf(mail.payload, "In MQTT's dance, connections accrue.");
            break;
        case 2:
            sprintf(mail.payload, "Topics like leaves, a lively tree,");
            break;
        case 3:
            sprintf(mail.payload,
                    "Brokers stand tall, like starlings set free.\n");
            break;
    }

    // Dispatch message
    if (xQueueSend(mailbox, &mail, 0) != pdPASS) {
        Serial.println("Loop error:\tMQTT Payload queue is full, unable to add "
                       "payload. Data is lost!");
    }

    count++;
    delay(5000);
}
