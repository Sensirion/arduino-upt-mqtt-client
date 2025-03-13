#include "MqttMailingService.h"
#include <Arduino.h>
#include <MeasurementFormatting.cpp>

/*
    In this usage example, the MqttMailingService is managing the Wi-Fi itself.
*/

MqttMailingService mqttMailingService;
int count = 0;

// Configuration
const char* ssid = "ap-name";
const char* password = "ap-pass.";
const char* broker_uri = "mqtt://mqtt.yourserver.com:1883";
const char ssl_cert[] =
    "------BEGIN CERTIFICATE-----\nmy-certificate\n-----END CERTIFICATE-----";

Measurement getSampleMeasurement();
Measurement dummyMeasurement = getSampleMeasurement();

void setup() {
    Serial.begin(115200);
    sleep(1);

    // Configure the MQTT mailing service
    mqttMailingService.setBrokerURI(broker_uri);
    // [Optional] set a prefix that will be prepended to the topic defined in the messages
    mqttMailingService.setGlobalTopicPrefix("myPrefix/deviceID2345/");

    // [Optional] Uncomment next line for SSL connection
    // mqttMailingService.setSslCertificate(ssl_cert);

    // Set a formatting function to be able to send Measurements
    mqttMailingService.setMeasurementMessageFormatterFn(&defaultMeasurementToMessage);

    // [Optional] Set a function to automatically define the topic based on passed Measurement
    mqttMailingService.setMeasurementToTopicSuffixFn(&defaultMeasurementToTopicSuffix);

    Serial.println("MQTT Mailing Service starting and connecting ....");

    // A blocking start ensure that connection is established before starting
    // sending messages.
    bool blockingStart = true;
    mqttMailingService.startWithDelegatedWiFi(ssid, password, blockingStart);

    Serial.println("MQTT Mailing Service started and connected !");
    mqttMailingService.sendTextMessage("Device connected !", "statusTopic/");
}

void loop() {
    // Update measurement time and send to MQTT.
    dummyMeasurement.dataPoint.t_offset = count*10;
    bool successful = mqttMailingService.sendMeasurement(dummyMeasurement);

    if(!successful){
        Serial.println("Failed to send measurement");
    }

    count++;
    delay(100);
}

/**
 * Returns a dummy measurement filled with realistic data combination
 */
Measurement getSampleMeasurement(){
    Measurement m;
    m.dataPoint.t_offset = 0;
    m.dataPoint.value = 100.0;
    m.signalType = SignalType::CO2_PARTS_PER_MILLION;
    m.metaData.deviceID = 932780134865341212;
    m.metaData.deviceType.sensorType = SensorType::SCD4X;
    m.metaData.platform = DevicePlatform::WIRED;
    return m;
}