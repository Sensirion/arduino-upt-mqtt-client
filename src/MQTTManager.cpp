#include "MQTTManager.h"
#include "event_source.h"

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
    // _useSsl = (bool)MQTT_USE_SSL_OVERRIDE;
    _useSsl = false;
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
}

void MqttMailingService::start() {
    if (_state == MqttMailingServiceState::UNINITIALIZED) {
        _initEspMqttClient();
    }
    _startEspMqttClient();
}

void MqttMailingService::setBrokerURI(const char* brokerURI) {
    if (strlen(brokerURI) > 64) {
        ESP_LOGW(TAG,
                 "Warning: requested broker URI \"%s\" is too long: %i "
                 "characters (max %i). It got truncated.",
                 brokerURI, strlen(brokerURI), 63);
    }
    strncpy(_brokerFullURI, brokerURI, 63);
}

void MqttMailingService::setLWTTopic(const char* lwtTopic) {
    if (strlen(lwtTopic) > 128) {
        ESP_LOGW(TAG,
                 "Warning: requested LWT topic \"%s\" is too long: %i "
                 "characters (max %i). It got truncated.",
                 lwtTopic, strlen(lwtTopic), 128);
    }
    strncpy(_lwtTopic, lwtTopic, 127);
}

void MqttMailingService::setLWTMessage(const char* lwtMessage) {
    if (strlen(lwtMessage) > 256) {
        ESP_LOGW(TAG,
                 "Warning: requested LWT message \"%s\" is too long: %i "
                 "characters (max %i). It got truncated.",
                 lwtMessage, strlen(lwtMessage), 256);
    }
    strncpy(_lwtMessage, lwtMessage, 255);
}

void MqttMailingService::setSslCertificate(const char* sslCert) {
    _sslCert = sslCert;
    _useSsl = true;
}

void MqttMailingService::setQOS(int qos) {
    _qos = qos;
}

void MqttMailingService::setRetainFlag(int retainFlag) {
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

QueueHandle_t MqttMailingService::getMailbox() const {
    return _mailbox;
}

MqttMailingServiceState MqttMailingService::getServiceState() {
    return _state;
}

/*
 *   Private
 */

void MqttMailingService::_initEspMqttClient() {
    // Setup mqtt event loop
    esp_event_loop_args_t mqtt_loop_args = {
        .queue_size = 2 * MQTT_EVENT_LOOP_LEN,
        .task_name = "MQTT Event Loop",
        .task_priority = uxTaskPriorityGet(NULL),
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

void MqttMailingService::_stopEspMqttClient() {
    esp_err_t ret;

    if (_state == MqttMailingServiceState::INITIALIZED) {
        // nothing to do
        return;
    }

    ret = esp_mqtt_client_stop(_espMqttClient);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop MQTT client");
        return;
    }
    _state = MqttMailingServiceState::INITIALIZED;
    ESP_LOGI(TAG, "MQTT client stopped.");
}

void MqttMailingService::_destroyEspMqttClient() {
    _state = MqttMailingServiceState::UNINITIALIZED;
    esp_mqtt_client_stop(_espMqttClient);
    esp_mqtt_client_destroy(_espMqttClient);
    _espMqttClient = nullptr;
    ESP_LOGI(TAG, "MQTT client has been destroyed.");
}

/*
 *   Task
 */
void MqttMailingService::_mailmanTask(void* arg) {
    MqttMailingService* mqttMgr = reinterpret_cast<MqttMailingService*>(arg);
    MQTTMessage outMail;

    while (1) {
        while (mqttMgr->_state == MqttMailingServiceState::CONNECTED &&
               xQueueReceive(mqttMgr->_mailbox, &outMail, 0) == pdTRUE) {

            // Forward message in mailbox to ESP MQTT client
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mqtt_client_publish(
                mqttMgr->_espMqttClient, (char*)outMail.topic,
                (char*)outMail.payload, outMail.len, mqttMgr->_qos,
                mqttMgr->_retainFlag));

            ESP_LOGD(
                mqttMgr->TAG,
                "Forwarded a msg to the MQTT client:\n\tTopic: %s\n\tMsg: %s",
                outMail.topic, outMail.payload);
            vTaskDelay(pdMS_TO_TICKS(20));
        }

        vTaskDelay(pdMS_TO_TICKS(MQTT_MAILBOX_COLLECTION_INTERVAL_MS));
    }
}

void MqttMailingService::_espMqttEventHandler(void* handler_args,
                                              esp_event_base_t base,
                                              int32_t event_id,
                                              void* event_data) {
    MqttMailingService* mqttMgr =
        reinterpret_cast<MqttMailingService*>(handler_args);
    esp_mqtt_event_handle_t event =
        reinterpret_cast<esp_mqtt_event_handle_t>(event_data);

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(mqttMgr->TAG, "ESP MQTT client connected");
            mqttMgr->_state = MqttMailingServiceState::CONNECTED;

            // Start running task
            if (mqttMgr->_mailmanTaskHandle == nullptr) {
                xTaskCreate(mqttMgr->_mailmanTask, "MQTT MsgDispatcher",
                            5 * 1024, reinterpret_cast<void*>(mqttMgr),
                            tskIDLE_PRIORITY + 1,
                            &(mqttMgr->_mailmanTaskHandle));
                ESP_LOGI(mqttMgr->TAG, "Mailman task launched.");
            }
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(mqttMgr->TAG, "ESP MQTT client disconnected");
            mqttMgr->_state = MqttMailingServiceState::DISCONNECTED;
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGW(mqttMgr->TAG,
                     "ESP MQTT client encountered an error (code %i)",
                     (*event).error_handle->error_type);
            break;
        default:
            break;
    }
}