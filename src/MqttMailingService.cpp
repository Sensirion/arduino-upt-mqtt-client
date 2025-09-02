#include "MqttMailingService.h"
#include <WiFi.h>

namespace sensirion::upt::mqtt{

#ifndef WIFI_CHECK_INTERVAL_MS
#define WIFI_CHECK_INTERVAL_MS 10000
#endif


const char* TAG = "MQTT Mail";
esp_mqtt_client_handle_t MqttMailingService::mEspMqttClient = nullptr;

constexpr auto ssl_cert =
    "------BEGIN CERTIFICATE-----\n" MQTT_BROKER_CERTIFICATE_OVERRIDE
    "\n-----END CERTIFICATE-----";


MqttMailingService::MqttMailingService():  
    mState{MqttMailingServiceState::UNINITIALIZED},
    mBrokerFullURI{0}, 
    mLwtTopic{"defaultTopic/"},
    mLwtMessage{"The MQTT Mailman unexpectedly disconnected."},
    mSslCert{""},
    mUseSsl{false},
    mQos{0},
    mRetainFlag{0} {}

MqttMailingService::~MqttMailingService() {
    destroyEspMqttClient();

    if (mShouldManageWifiConnection) {
        xTaskAbortDelay(mWifiCheckTaskHandle);
        vTaskDelete(mWifiCheckTaskHandle);
        mWifiCheckTaskHandle = nullptr;
    }
}

void MqttMailingService::start() {
    if (mState == MqttMailingServiceState::UNINITIALIZED) {
        initEspMqttClient();
    }
    startEspMqttClient();
}

[[maybe_unused]] void
MqttMailingService::startWithDelegatedWiFi(const char* ssid, const char* pass,
                                           const bool shouldBeBlocking) {

    mShouldManageWifiConnection = true;
    WiFi.begin(ssid, pass);

    if (shouldBeBlocking) {
        ESP_LOGI(TAG, "Waiting for Wi-Fi connection...");
        // Wait for Wi-Fi connection, since MQTT mailing service
        // can't establish a connection without Wi-Fi.
        while (!WiFi.isConnected()) {
            sleep(1);
        }
        ESP_LOGD(TAG, "Wi-Fi connected");
    }

    start();

    // Create Wi-Fi monitoring task to ensure reconnection
    if (mWifiCheckTaskHandle == nullptr) {
        xTaskCreate(MqttMailingService::wifiCheckTaskCode, "Wi-Fi Check", 1 * 1024,
                    nullptr, tskIDLE_PRIORITY + 1, &(mWifiCheckTaskHandle));
        ESP_LOGI(TAG, "WiFi check task launched.");
    }

    if (shouldBeBlocking) {
        // Wait until MQTT service connects to broker
        ESP_LOGD(TAG,
                 "MQTT mailing service is connecting...");
        while (getServiceState() != MqttMailingServiceState::CONNECTED) {
            sleep(1);
        }
        ESP_LOGD(TAG, "MQTT mailing service is connected.");
    }
}

[[maybe_unused]] void
MqttMailingService::startWithDelegatedWiFi(const char* ssid, const char* pass) {
    startWithDelegatedWiFi(ssid, pass, false);
}

[[maybe_unused]] void MqttMailingService::startWithDelegatedWiFi() {
    startWithDelegatedWiFi(WIFI_SSID_OVERRIDE, WIFI_PW_OVERRIDE, false);
}

[[maybe_unused]] bool
MqttMailingService::setBrokerURI(std::string&& brokerURI) {
    mBrokerFullURI = brokerURI;
    return true;
}

[[maybe_unused]] bool MqttMailingService::setBroker(std::string&& brokerDomain, const bool hasSsl) {
    std::string protocol{"mqtt://"};
    std::string port{":1883"};
    if (hasSsl){
        protocol = "mqtts://";
        port = ":8883";
    }
    mBrokerFullURI = protocol + brokerDomain + port;
    return true;
}

[[maybe_unused]] void
MqttMailingService::setLWTTopic(std::string&& lwtTopic) {
    mLwtTopic = lwtTopic;
}

[[maybe_unused]] void
MqttMailingService::setLWTMessage(std::string&& lwtMessage) {
    mLwtMessage = lwtMessage;
}

[[maybe_unused]] void
MqttMailingService::setSslCertificate(std::string&& sslCert) {
    mSslCert = sslCert;
    mUseSsl = true;
}

[[maybe_unused]] void
MqttMailingService::enableSsl() {
    mSslCert = ssl_cert;
    mUseSsl = true;
}

[[maybe_unused]] void MqttMailingService::setQOS(int qos) {
    mQos = qos;
}

[[maybe_unused]] void
MqttMailingService::setGlobalTopicPrefix(std::string&& topicPrefix) {
    mGlobalTopicPrefix = topicPrefix;
}

void MqttMailingService::setRetainFlag(int retainFlag) {
    if (retainFlag < 0 || retainFlag > 1) {
        ESP_LOGW(
            TAG,
            "Warning: attempt to set illegal retain flag %i. Reverting to 0.",
            retainFlag);
        mRetainFlag = 0;
    } else {
        mRetainFlag = retainFlag;
    }
}

[[maybe_unused]] MqttMailingServiceState
MqttMailingService::getServiceState() {
    return mState;
}

[[maybe_unused]] bool MqttMailingService::isReady() {
    if (mShouldManageWifiConnection) {
        return WiFi.isConnected() &&
               getServiceState() == MqttMailingServiceState::CONNECTED;
    }
    return getServiceState() == MqttMailingServiceState::CONNECTED;
}

void MqttMailingService::setMeasurementMessageFormatterFn(MeasurementFormatterType fFmt) {
    mMeasurementFormatterFn = fFmt;
}

void MqttMailingService::setMeasurementToTopicSuffixFn(MeasurementFormatterType fFmt) {
    mTopicSuffixFn = fFmt;
}

bool MqttMailingService::fwdMqttMessage(const char* topic, const char* message){
    // Forward message in mailbox to the ESP MQTT client
    return esp_mqtt_client_publish(mEspMqttClient, topic, message, 0, mQos, mRetainFlag) != -1;
}

[[maybe_unused]]
bool MqttMailingService::sendTextMessage(const std::string& message,
                                         const std::string& topicSuffix) {


    const auto topic{mGlobalTopicPrefix + topicSuffix};                                        
    return fwdMqttMessage(topic.c_str(), message.c_str());
}

bool MqttMailingService::sendMeasurement(const sensirion::upt::core::Measurement measurement, 
                                         const std::string& topicSuffix) {
    if (!mMeasurementFormatterFn){
        ESP_LOGE(TAG, "Formatter not set, message not sent");
        return false;
    }
    auto message = mMeasurementFormatterFn(measurement);
    return sendTextMessage(message, topicSuffix);
}

bool MqttMailingService::sendMeasurement(const sensirion::upt::core::Measurement measurement) {
    if (!mTopicSuffixFn){
        ESP_LOGE(TAG, "TopicSuffixFunction is not set, message not sent");
        return false;
    }
    
    auto suffix = mTopicSuffixFn(measurement);

    return sendMeasurement(measurement, suffix);
}

/*
 *   Private
 */

void MqttMailingService::initEspMqttClient() {
    esp_mqtt_client_config_t mqtt_cfg = {.uri = mBrokerFullURI.data(),
                                         .lwt_topic = mLwtTopic.data(),
                                         .lwt_msg = mLwtMessage.data(),
                                         .disable_auto_reconnect = false};
    if (mUseSsl) {
        mqtt_cfg.cert_pem = mSslCert.c_str();
    }

    mEspMqttClient = esp_mqtt_client_init(&mqtt_cfg);
    if (!mEspMqttClient) {
        ESP_LOGE(TAG, "Fatal error: Could not initiate MQTT Client. Aborting.");
        assert(0);
    }

    esp_mqtt_client_register_event(
        mEspMqttClient, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID),
        espMqttEventHandler, this);

    mState = MqttMailingServiceState::INITIALIZED;
    ESP_LOGI(TAG, "ESP MQTT client initialized.");
}

void MqttMailingService::startEspMqttClient() {
    esp_err_t ret;

    if (mState == MqttMailingServiceState::CONNECTED ||
        mState == MqttMailingServiceState::CONNECTING) {
        // nothing to do
        return;
    }

    ret = esp_mqtt_client_start(mEspMqttClient);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client");
        return;
    }
    mState = MqttMailingServiceState::CONNECTING;
    ESP_LOGI(TAG, "MQTT client connecting...");
}

void MqttMailingService::destroyEspMqttClient() {
    mState = MqttMailingServiceState::UNINITIALIZED;
    esp_mqtt_client_stop(mEspMqttClient);
    esp_mqtt_client_destroy(mEspMqttClient);
    mEspMqttClient = nullptr;
    ESP_LOGI(TAG, "MQTT client has been destroyed.");
}

void MqttMailingService::espMqttEventHandler(
    void* handler_args, 
    [[maybe_unused]] esp_event_base_t base,
    int32_t event_id, void* event_data) {
    auto* pMailingService = reinterpret_cast<MqttMailingService*>(handler_args);
    auto event = reinterpret_cast<esp_mqtt_event_handle_t>(event_data);

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "ESP MQTT client connected");
            pMailingService->mState = MqttMailingServiceState::CONNECTED;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "ESP MQTT client disconnected");
            pMailingService->mState = MqttMailingServiceState::DISCONNECTED;
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGW(TAG,
                     "ESP MQTT client encountered an error (code %i)",
                     (*event).error_handle->error_type);
            break;
        default:
            break;
    }
}

/**
 * Wi-Fi check task
 * This tasks checks if Wi-Fi is still connected every 10 second
 * and tries to reconnect if Wi-Fi connection is not established.
 */
void MqttMailingService::wifiCheckTaskCode(
    [[maybe_unused]] void* arg) {
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(WIFI_CHECK_INTERVAL_MS));

        if (!WiFi.isConnected()) {
            WiFi.reconnect();
        }
    }
}
} // end namespace

