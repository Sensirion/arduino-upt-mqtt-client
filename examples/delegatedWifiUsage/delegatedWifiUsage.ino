#include "MqttMailingService.h"
#include <Arduino.h>
#include <MeasurementFormatting.hpp>

/*
    In this usage example, the MqttMailingService is managing the Wi-Fi itself.
*/
using namespace sensirion::upt;

mqtt::MqttMailingService mqttMailingService;
int count = 0;

// Configuration
constexpr auto ssid = "ap-name";
constexpr auto password = "ap-pass.";
constexpr auto broker_uri = "mqtt://mqtt.yourserver.com:1883";
constexpr const auto ssl_cert =
    "------BEGIN CERTIFICATE-----\nmy-certificate\n-----END CERTIFICATE-----";

sensirion::upt::core::Measurement getSampleMeasurement();
sensirion::upt::core::Measurement dummyMeasurement = getSampleMeasurement();

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
    mqttMailingService.setMeasurementMessageFormatterFn(mqtt::DefaultMeasurementFormatter{});

    // [Optional] Set a function to automatically define the topic based on passed Measurement
    mqttMailingService.setMeasurementToTopicSuffixFn(mqtt::DefaultMeasurementToTopicSuffix{});

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
sensirion::upt::core::Measurement getSampleMeasurement(){
    sensirion::upt::core::MetaData meta{sensirion::upt::core::SCD4X()};
    meta.deviceID = 932780134865341212;
    sensirion::upt::core::Measurement m{meta, 
        sensirion::upt::core::SignalType::CO2_PARTS_PER_MILLION, 
        sensirion::upt::core::DataPoint{0, 100.0}
    };
    return m;
}