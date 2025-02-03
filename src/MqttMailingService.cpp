#include "MqttMailingService.h"
#include "event_source.h"
#include <WiFi.h>

const char* TAG = "MQTT Mail";
esp_mqtt_client_handle_t MqttMailingService::mEspMqttClient = nullptr;

const uint8_t ssl_cert[] =
    "------BEGIN CERTIFICATE-----\n" MQTT_BROKER_CERTIFICATE_OVERRIDE
    "\n-----END CERTIFICATE-----";

MqttMailingService::MqttMailingService() {
    mState = MqttMailingServiceState::UNINITIALIZED;
    // Apply default parameters
    strncpy(mBrokerFullURI, MQTT_BROKER_FULL_URI_OVERRIDE, 64);
    strncpy(mLwtTopic, MQTT_LWT_TOPIC_OVERRIDE, 127);
    strncpy(mLwtMessage, MQTT_LWT_MSG_OVERRIDE, 255);
    mSslCert = (char*)ssl_cert;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "Simplify"
#pragma ide diagnostic ignored "UnreachableCode"
    mUseSsl = MQTT_USE_SSL_OVERRIDE == 0 ? false : true;
#pragma clang diagnostic pop
    mQos = MQTT_QOS_OVERRIDE;
    mRetainFlag = MQTT_RETAIN_FLAG_OVERRIDE;
    // Create the mailbox
    mMailbox = xQueueCreate(MQTT_DATAQUEUE_LEN, sizeof(MQTTMessage));
}

MqttMailingService::~MqttMailingService() {
    if (mMailmanTaskHandle == nullptr) {
        xTaskAbortDelay(mMailmanTaskHandle);
        vTaskDelete(mMailmanTaskHandle);
        mMailmanTaskHandle = nullptr;
    }
    destroyEspMqttClient();
    vQueueDelete(mMailbox);

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

__attribute__((unused)) void
MqttMailingService::setBrokerURI(const char* brokerURI) {
    if (strlen(brokerURI) > 64) {
        ESP_LOGW(TAG,
                 "Warning: requested broker URI \"%s\" is too long: %i "
                 "characters (max %i). It got truncated.",
                 brokerURI, strlen(brokerURI), 63);
    }
    strncpy(mBrokerFullURI, brokerURI, 63);
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
MqttMailingService::setSslCertificate(const char* sslCert) {
    mSslCert = sslCert;
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

__attribute__((unused)) QueueHandle_t MqttMailingService::getMailbox() const {
    return mMailbox;
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

/*
 *   Private
 */

void MqttMailingService::initEspMqttClient() {
    // Setup mqtt event loop
    esp_event_loop_args_t mqtt_loop_args = {.queue_size = MQTT_EVENT_LOOP_LEN,
                                            .task_name = "MQTT Event Loop",
                                            .task_priority =
                                                uxTaskPriorityGet(nullptr),
                                            .task_stack_size = 2 * 1024,
                                            .task_core_id = tskNO_AFFINITY};
    esp_event_loop_handle_t mqtt_loop_handle;
    esp_event_loop_create(&mqtt_loop_args, &mqtt_loop_handle);

    // Config
    esp_mqtt_client_config_t mqtt_cfg = {.event_loop_handle = mqtt_loop_handle,
                                         .uri = mBrokerFullURI,
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

/*
 *   MQTT Mailman Task
 */
[[noreturn]] void MqttMailingService::mailmanTaskCode(void* arg) {
    auto* pMailingService = reinterpret_cast<MqttMailingService*>(arg);
    MQTTMessage outMail;

    while (true) {
        while (pMailingService->mState == MqttMailingServiceState::CONNECTED &&
               xQueueReceive(pMailingService->mMailbox, &outMail, 0) ==
                   pdTRUE) {

            // Concatenate topic
            char topic[256];
            strcpy(topic, pMailingService->mGlobalTopicPrefix);
            strcat(topic, outMail.topicSuffix);

            ESP_LOGI(TAG, "Sending to topic: %s", topic);

            // Forward message in mailbox to the ESP MQTT client
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mqtt_client_publish(
                pMailingService->mEspMqttClient, topic,
                (char*)outMail.payload, outMail.len, pMailingService->mQos,
                pMailingService->mRetainFlag));

            

            ESP_LOGD(TAG,
                "Forwarded a msg to the MQTT client:\n\tTopic: %s\n\tMsg: %s",
                outMail.topicSuffix, outMail.payload);
            vTaskDelay(pdMS_TO_TICKS(20));
        }

        vTaskDelay(pdMS_TO_TICKS(MQTT_MAILBOX_COLLECTION_INTERVAL_MS));
    }
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

            // Start running task if not existing yet
            if (pMailingService->mMailmanTaskHandle == nullptr) {
                xTaskCreate(MqttMailingService::mailmanTaskCode,
                            "MQTT MsgDispatcher", 5 * 1024,
                            reinterpret_cast<void*>(pMailingService),
                            tskIDLE_PRIORITY + 1,
                            &(pMailingService->mMailmanTaskHandle));
                ESP_LOGI(TAG, "Mailman task launched.");
            }
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
