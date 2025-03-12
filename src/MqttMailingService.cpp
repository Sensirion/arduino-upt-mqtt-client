#include "MqttMailingService.h"
#include <WiFi.h>

#ifndef WIFI_CHECK_INTERVAL_MS
#define WIFI_CHECK_INTERVAL_MS 10000
#endif


const char* TAG = "MQTT Mail";
esp_mqtt_client_handle_t MqttMailingService::mEspMqttClient = nullptr;

const uint8_t ssl_cert[] =
    "------BEGIN CERTIFICATE-----\n" MQTT_BROKER_CERTIFICATE_OVERRIDE
    "\n-----END CERTIFICATE-----";

const uint8_t DEFAULT_MQTT_EVENT_LOOP_SIZE = 20;


MqttMailingService::MqttMailingService() {
    mState = MqttMailingServiceState::UNINITIALIZED;
    // Apply default parameters
    mBrokerFullURI[0] = '\0';
    strncpy(mLwtTopic, "defaultTopic/", 127);
    strncpy(mLwtMessage, "The MQTT Mailman unexpectedly disconnected.", 255);
    mSslCert = "";
    mUseSsl = false;
    mQos = 0;
    mRetainFlag = 0;
    mMqttEventLoopSize = DEFAULT_MQTT_EVENT_LOOP_SIZE;
}

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

__attribute__((unused)) void
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

__attribute__((unused)) void
MqttMailingService::startWithDelegatedWiFi(const char* ssid, const char* pass) {
    startWithDelegatedWiFi(ssid, pass, false);
}

__attribute__((unused)) void MqttMailingService::startWithDelegatedWiFi() {
    startWithDelegatedWiFi(WIFI_SSID_OVERRIDE, WIFI_PW_OVERRIDE, false);
}

__attribute__((unused)) bool
MqttMailingService::setBrokerURI(const char* brokerURI) {
    if (strlen(brokerURI) > MAX_URI_LENGTH) {
        ESP_LOGW(TAG,
                 "Error: requested broker URI \"%s\" is too long: %i "
                 "characters (max %i).",
                 brokerURI, strlen(brokerURI), MAX_URI_LENGTH);
        return false;
    }
    strncpy(mBrokerFullURI, brokerURI, MAX_URI_LENGTH); 
    return true;
}

__attribute__((unused)) bool MqttMailingService::setBroker(const char* brokerDomain, const bool hasSsl) {
        const int maxDomainLength = MAX_URI_LENGTH - 8 - 5;
        if (strlen(brokerDomain) > maxDomainLength) {
            ESP_LOGW(TAG,
                "Error: requested broker Domain \"%s\" is too long: %i "
                 "characters (max %i).",
                brokerDomain, strlen(brokerDomain), maxDomainLength);
            return false;
        }
        
        if(hasSsl) {
            strcpy(mBrokerFullURI, "mqtts://");
            strcat(mBrokerFullURI, brokerDomain);
            strcat(mBrokerFullURI, ":8883");
        } else {
            strcpy(mBrokerFullURI, "mqtt://");
            strcat(mBrokerFullURI, brokerDomain);
            strcat(mBrokerFullURI, ":1883");
        }
        return true;
    }

__attribute__((unused)) void
MqttMailingService::setLWTTopic(const char* lwtTopic) {
    if (strlen(lwtTopic) > 128) {
        ESP_LOGW(TAG,
                 "Warning: requested LWT topic \"%s\" is too long: %i "
                 "characters (max %i). It got truncated.",
                 lwtTopic, strlen(lwtTopic), 128);
    }
    strncpy(mLwtTopic, lwtTopic, 127);
}

__attribute__((unused)) void
MqttMailingService::setLWTMessage(const char* lwtMessage) {
    if (strlen(lwtMessage) > 256) {
        ESP_LOGW(TAG,
                 "Warning: requested LWT message \"%s\" is too long: %i "
                 "characters (max %i). It got truncated.",
                 lwtMessage, strlen(lwtMessage), 256);
    }
    strncpy(mLwtMessage, lwtMessage, 255);
}

__attribute__((unused)) void 
    MqttMailingService::setMqttEventLoopSize(int loopSize) {
        mMqttEventLoopSize = loopSize;
    }

__attribute__((unused)) void
MqttMailingService::setSslCertificate(const char* sslCert) {
    mSslCert = sslCert;
    mUseSsl = true;
}

__attribute__((unused)) void
MqttMailingService::enableSsl() {
    mSslCert = (char*)ssl_cert;
    mUseSsl = true;
}

__attribute__((unused)) void MqttMailingService::setQOS(int qos) {
    mQos = qos;
}

__attribute__((unused)) void
MqttMailingService::setGlobalTopicPrefix(const char* topicPrefix) {
    if (strlen(topicPrefix) > 128) {
        ESP_LOGW(TAG,
                 "Warning: requested global prefix message \"%s\" is too long: %i "
                 "characters (max %i). It got truncated.",
                 topicPrefix, strlen(topicPrefix), 128);
    }
    strncpy(mGlobalTopicPrefix, topicPrefix, 127);
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

__attribute__((unused)) MqttMailingServiceState
MqttMailingService::getServiceState() {
    return mState;
}

__attribute__((unused)) bool MqttMailingService::isReady() {
    if (mShouldManageWifiConnection) {
        return WiFi.isConnected() &&
               getServiceState() == MqttMailingServiceState::CONNECTED;
    }
    return getServiceState() == MqttMailingServiceState::CONNECTED;
}

void MqttMailingService::setMeasurementMessageFormatterFn(void (*fFmt)(Measurement, char*)) {
    mMeasurementFormatterFn = fFmt;
}

void MqttMailingService::setMeasurementToTopicSuffixFn(void (*fFmt)(Measurement, char*)) {
    mTopicSuffixFn = fFmt;
}

bool MqttMailingService::fwdMqttMessage(const char* topic, const char* message){
    // Forward message in mailbox to the ESP MQTT client
    return esp_mqtt_client_publish(mEspMqttClient, topic, message, 0, mQos, mRetainFlag) != -1;
}

bool MqttMailingService::sendTextMessage(const char* message,
                                        const char* topicSuffix) {
    if (strlen(topicSuffix) > MQTT_TOPIC_SUFFIX_MAX_LENGTH) {
        ESP_LOGW(TAG,"topicSuffix \"%s\" is too long, message not sent",topicSuffix);
        return false;
    }

    // Assemble topic: prefix + suffix
    char topic[MQTT_TOPIC_PREFIX_MAX_LENGTH + MQTT_TOPIC_SUFFIX_MAX_LENGTH];
    strcpy(topic, mGlobalTopicPrefix);
    strcat(topic, topicSuffix);

    return fwdMqttMessage(topic, message);
}

bool MqttMailingService::sendMeasurement(const Measurement measurement, const char* topicSuffix) {
    if (mMeasurementFormatterFn == nullptr){
        ESP_LOGE(TAG, "Formatter not set, message not sent");
        return false;
    }
    char msgBuffer[MQTT_MEASUREMENT_MESSAGE_MAX_LENGTH]; 
    mMeasurementFormatterFn(measurement, msgBuffer);
    return sendTextMessage(msgBuffer,topicSuffix);
}

bool MqttMailingService::sendMeasurement(const Measurement measurement) {
    if (mTopicSuffixFn == nullptr){
        ESP_LOGE(TAG, "TopicSuffixFunction is not set, message not sent");
        return false;
    }
    char topicSuffix[MQTT_TOPIC_SUFFIX_MAX_LENGTH]; 
    mTopicSuffixFn(measurement, topicSuffix);

    return sendMeasurement(measurement,topicSuffix);
}

/*
 *   Private
 */

void MqttMailingService::initEspMqttClient() {
    esp_mqtt_client_config_t mqtt_cfg = {.uri = mBrokerFullURI,
                                         .lwt_topic = mLwtTopic,
                                         .lwt_msg = mLwtMessage,
                                         .disable_auto_reconnect = false};
    if (mUseSsl) {
        mqtt_cfg.cert_pem = mSslCert;
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
    void* handler_args, __attribute__((unused)) esp_event_base_t base,
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
void MqttMailingService::wifiCheckTaskCode(__attribute__((unused)) void* arg) {
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(WIFI_CHECK_INTERVAL_MS));

        if (!WiFi.isConnected()) {
            WiFi.reconnect();
        }
    }
}
