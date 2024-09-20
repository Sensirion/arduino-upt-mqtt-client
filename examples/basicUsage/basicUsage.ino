#include "MQTTManager.h"
#include <Arduino.h>

/* Shows features of the library:
    -- Connecting to WiFi AP
    -- Connecting to a MQTT broker via it's URI
    -- Dispatching MQTT Messages
*/

MqttMailingService mqttMailman;
QueueHandle_t mailbox;
MQTTMessage mail;
int count = 0;

const char* ssid = "ap-name";
const char* password = "ap-pass.";
const char* broker_uri = "mqtt://mqtt.some-server.com:1883";

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

    mqttMailman.start();
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("Disconnected from WiFi access point");
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.wifi_sta_disconnected.reason);
    Serial.println("Trying to Reconnect");
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
    delay(2000);

    // Configure the MQTT Mailman service
    mqttMailman.setBrokerURI(broker_uri);
    mailbox = mqttMailman.getMailbox();

    // Initialize and connect to WiFi
    initializeWiFi();
    WiFi.begin(ssid, password);

    // Wait until service initialized
    while (mqttMailman.getServiceState() ==
           MqttMailingServiceState::UNINITIALIZED) {
        sleep(1);
    }

    Serial.println("setup() completed");
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