#include "MqttMailingService.h"
#include "event_source.h"
#include <WiFi.h>

const char* MqttMailingService::TAG = "MQTT Mail";
esp_mqtt_client_handle_t MqttMailingService::_espMqttClient = nullptr;

const uint8_t ssl_cert[] =
    "------BEGIN CERTIFICATE-----\n" MQTT_BROKER_CERTIFICATE_OVERRIDE
    "\n-----END CERTIFICATE-----";

MqttMailingService::MqttMailingService() {
    _state = MqttMailingServiceState::UNINITIALIZED;
    // Apply default parameters
    strncpy(_brokerFullURI, MQTT_BROKER_FULL_URI_OVERRIDE, 64);
    strncpy(_lwtTopic, MQTT_LWT_TOPIC_OVERRIDE, 127);
    strncpy(_lwtMessage, MQTT_LWT_MSG_OVERRIDE, 255);
    _sslCert = (char*)ssl_cert;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "Simplify"
#pragma ide diagnostic ignored "UnreachableCode"
    _useSsl = MQTT_USE_SSL_OVERRIDE == 0 ? false : true;
#pragma clang diagnostic pop
    _qos = MQTT_QOS_OVERRIDE;
    _retainFlag = MQTT_RETAIN_FLAG_OVERRIDE;
    // Create the mailbox
    _mailbox = xQueueCreate(MQTT_DATAQUEUE_LEN, sizeof(MQTTMessage));
}

MqttMailingService::~MqttMailingService() {
    if (_mailmanTaskHandle == nullptr) {
        xTaskAbortDelay(_mailmanTaskHandle);
        vTaskDelete(_mailmanTaskHandle);
        _mailmanTaskHandle = nullptr;
    }
    _destroyEspMqttClient();
    vQueueDelete(_mailbox);

    if (_should_manage_wifi_connection) {
        xTaskAbortDelay(_wifiCheckTaskHandle);
        vTaskDelete(_wifiCheckTaskHandle);
        _wifiCheckTaskHandle = nullptr;
    }
}

void MqttMailingService::start() {
    if (_state == MqttMailingServiceState::UNINITIALIZED) {
        _initEspMqttClient();
    }
    _startEspMqttClient();
}

__attribute__((unused)) void
MqttMailingService::startWithDelegatedWiFi(const char* ssid, const char* pass) {
    _should_manage_wifi_connection = true;
    WiFi.begin(ssid, pass);
    start();

    if (_wifiCheckTaskHandle == nullptr) {
        xTaskCreate(MqttMailingService::_wifiCheckTask, "Wi-Fi Check", 1 * 1024,
                    nullptr, tskIDLE_PRIORITY + 1, &(_wifiCheckTaskHandle));
        ESP_LOGI(MqttMailingService::TAG, "WiFi check task launched.");
    }
}

__attribute__((unused)) void MqttMailingService::startWithDelegatedWiFi() {
    startWithDelegatedWiFi(WIFI_SSID_OVERRIDE, WIFI_PW_OVERRIDE);
}

__attribute__((unused)) void
MqttMailingService::setBrokerURI(const char* brokerURI) {
    if (strlen(brokerURI) > 64) {
        ESP_LOGW(TAG,
                 "Warning: requested broker URI \"%s\" is too long: %i "
                 "characters (max %i). It got truncated.",
                 brokerURI, strlen(brokerURI), 63);
    }
    strncpy(_brokerFullURI, brokerURI, 63);
}

__attribute__((unused)) void
MqttMailingService::setLWTTopic(const char* lwtTopic) {
    if (strlen(lwtTopic) > 128) {
        ESP_LOGW(TAG,
                 "Warning: requested LWT topic \"%s\" is too long: %i "
                 "characters (max %i). It got truncated.",
                 lwtTopic, strlen(lwtTopic), 128);
    }
    strncpy(_lwtTopic, lwtTopic, 127);
}

__attribute__((unused)) void
MqttMailingService::setLWTMessage(const char* lwtMessage) {
    if (strlen(lwtMessage) > 256) {
        ESP_LOGW(TAG,
                 "Warning: requested LWT message \"%s\" is too long: %i "
                 "characters (max %i). It got truncated.",
                 lwtMessage, strlen(lwtMessage), 256);
    }
    strncpy(_lwtMessage, lwtMessage, 255);
}

__attribute__((unused)) void
MqttMailingService::setSslCertificate(const char* sslCert) {
    _sslCert = sslCert;
    _useSsl = true;
}

__attribute__((unused)) void MqttMailingService::setQOS(int qos) {
    _qos = qos;
}

__attribute__((unused)) void MqttMailingService::setRetainFlag(int retainFlag) {
    if (retainFlag < 0 || retainFlag > 1) {
        ESP_LOGW(
            TAG,
            "Warning: attempt to set illegal retain flag %i. Reverting to 0.",
            retainFlag);
        _retainFlag = 0;
    } else {
        _retainFlag = retainFlag;
    }
}

__attribute__((unused)) QueueHandle_t MqttMailingService::getMailbox() const {
    return _mailbox;
}

__attribute__((unused)) MqttMailingServiceState
MqttMailingService::getServiceState() {
    return _state;
}

/*
 *   Private
 */

void MqttMailingService::_initEspMqttClient() {
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
                                         .uri = _brokerFullURI,
                                         .lwt_topic = _lwtTopic,
                                         .lwt_msg = _lwtMessage,
                                         .disable_auto_reconnect = false};
    if (_useSsl) {
        mqtt_cfg.cert_pem = _sslCert;
    }
    _espMqttClient = esp_mqtt_client_init(&mqtt_cfg);
    if (!_espMqttClient) {
        ESP_LOGE(TAG, "Fatal error: Could not initiate MQTT Client. Aborting.");
        assert(0);
    }

    esp_mqtt_client_register_event(
        _espMqttClient, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID),
        _espMqttEventHandler, this);

    _state = MqttMailingServiceState::INITIALIZED;
    ESP_LOGI(TAG, "ESP MQTT client initialized.");
}

void MqttMailingService::_startEspMqttClient() {
    esp_err_t ret;

    if (_state == MqttMailingServiceState::CONNECTED ||
        _state == MqttMailingServiceState::CONNECTING) {
        // nothing to do
        return;
    }

    ret = esp_mqtt_client_start(_espMqttClient);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client");
        return;
    }
    _state = MqttMailingServiceState::CONNECTING;
    ESP_LOGI(TAG, "MQTT client connecting...");
}

void MqttMailingService::_destroyEspMqttClient() {
    _state = MqttMailingServiceState::UNINITIALIZED;
    esp_mqtt_client_stop(_espMqttClient);
    esp_mqtt_client_destroy(_espMqttClient);
    _espMqttClient = nullptr;
    ESP_LOGI(TAG, "MQTT client has been destroyed.");
}

/*
 *   MQTT Mailman Task
 */
[[noreturn]] void MqttMailingService::_mailmanTask(void* arg) {
    auto* pMailingService = reinterpret_cast<MqttMailingService*>(arg);
    MQTTMessage outMail;

    while (true) {
        while (pMailingService->_state == MqttMailingServiceState::CONNECTED &&
               xQueueReceive(pMailingService->_mailbox, &outMail, 0) ==
                   pdTRUE) {

            // Forward message in mailbox to the ESP MQTT client
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mqtt_client_publish(
                pMailingService->_espMqttClient, (char*)outMail.topic,
                (char*)outMail.payload, outMail.len, pMailingService->_qos,
                pMailingService->_retainFlag));

            ESP_LOGD(
                pMailingService->TAG,
                "Forwarded a msg to the MQTT client:\n\tTopic: %s\n\tMsg: %s",
                outMail.topic, outMail.payload);
            vTaskDelay(pdMS_TO_TICKS(20));
        }

        vTaskDelay(pdMS_TO_TICKS(MQTT_MAILBOX_COLLECTION_INTERVAL_MS));
    }
}

void MqttMailingService::_espMqttEventHandler(
    void* handler_args, __attribute__((unused)) esp_event_base_t base,
    int32_t event_id, void* event_data) {
    auto* pMailingService = reinterpret_cast<MqttMailingService*>(handler_args);
    auto event = reinterpret_cast<esp_mqtt_event_handle_t>(event_data);

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(pMailingService->TAG, "ESP MQTT client connected");
            pMailingService->_state = MqttMailingServiceState::CONNECTED;

            // Start running task
            if (pMailingService->_mailmanTaskHandle == nullptr) {
                xTaskCreate(MqttMailingService::_mailmanTask,
                            "MQTT MsgDispatcher", 5 * 1024,
                            reinterpret_cast<void*>(pMailingService),
                            tskIDLE_PRIORITY + 1,
                            &(pMailingService->_mailmanTaskHandle));
                ESP_LOGI(pMailingService->TAG, "Mailman task launched.");
            }
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(pMailingService->TAG, "ESP MQTT client disconnected");
            pMailingService->_state = MqttMailingServiceState::DISCONNECTED;
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGW(pMailingService->TAG,
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
void MqttMailingService::_wifiCheckTask(__attribute__((unused)) void* arg) {
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(WIFI_CHECK_INTERVAL_MS));

        if (!WiFi.isConnected()) {
            WiFi.reconnect();
        }
    }
}
