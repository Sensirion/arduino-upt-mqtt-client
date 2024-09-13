#include "MQTTManager.h"
#include <Arduino.h>

/* Shows features of the library:
    -- Connecting to WiFi AP
    -- Connecting to a MQTT broker via it's URI
    -- Dispatching MQTT Messages
*/

WiFiManager wifiMgr;
MqttMailingService mqttMailman;
QueueHandle_t mailbox;
int count = 0;

const char* ssid = "yourssid";
const char* password = "yourpass";
const char ssl_cert[] =
    "------BEGIN CERTIFICATE-----\nmy-certificate\n-----END CERTIFICATE-----";

unsigned long previousMillis = 0;
unsigned long interval = 5000;

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    mqttMailman.startMqttClient();
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("Disconnected from WiFi access point");
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.wifi_sta_disconnected.reason);
    Serial.println("Trying to Reconnect");
    WiFi.disconnect();
    WiFi.reconnect();
}

void initializeWiFi() {
    WiFi.disconnect(true);

    delay(1000);

    WiFi.onEvent(WiFiStationConnected,
                 WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(WiFiStationDisconnected,
                 WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    initializeWiFi();
    WiFi.begin(ssid, password);
    mqttMailman.start("mqtt://mqtt.smartobjectserver.com:1883");
    mailbox = mqttMailman.getDataEventQueueHandle();
}

void loop() {
    MQTTMessage mail;

    // Set payload and topic
    sprintf(mail.topic, "testtopic/");

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
        Serial.println("Loop error:\tMQTT Payload queue is full, unable to "
                       "payload. Data is lost!");
    }

    count++;
    delay(1000);
}
