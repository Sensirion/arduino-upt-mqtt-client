#include "MQTTManager.h"
#include "event_source.h"

const char* MqttMailingService::TAG = "MQTT Mail";
esp_mqtt_client_handle_t MqttMailingService::_mqttClient = nullptr;

const uint8_t ssl_cert[] =
    "------BEGIN CERTIFICATE-----\n" MQTT_BROKER_CERTIFICATE_OVERRIDE
    "\n-----END CERTIFICATE-----";

MqttMailingService::MqttMailingService() {
    strncpy(_brokerFullURI, MQTT_BROKER_FULL_URI_OVERRIDE, 64);
    strncpy(_lwtTopic, MQTT_LWT_TOPIC_OVERRIDE, 127);
    strncpy(_lwtMessage, MQTT_LWT_MSG_OVERRIDE, 255);
    _sslCert = (char*)ssl_cert;
    _useSsl = (bool)MQTT_USE_SSL_OVERRIDE;
    _qos = MQTT_QOS_OVERRIDE;
    _retainFlag = MQTT_RETAIN_FLAG_OVERRIDE;
}

MqttMailingService::~MqttMailingService() {
    if (_mailmanTaskHandle == nullptr) {
        _mailmanIsRunning = false;
        xTaskAbortDelay(_mailmanTaskHandle);
        vTaskDelete(_mailmanTaskHandle);
        _mailmanTaskHandle = nullptr;
    }
    destroyMqttClient();
    vQueueDelete(_mailbox);
}

void MqttMailingService::start() {
    return start(_brokerFullURI);
}

void MqttMailingService::start(const char* fullURI) {
    // _wifiEventGroup = wifiEventGroup;
    _mailbox = xQueueCreate(MQTT_DATAQUEUE_LEN, sizeof(MQTTMessage));
    setBrokerURI(fullURI);

    initMqttClient();
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

void MqttMailingService::startMqttClient() {
    esp_err_t ret;

    if (_clientState == MQTT_STATE_CONNECTED ||
        _clientState == MQTT_STATE_STARTING) {
        // nothing to do
        return;
    }

    if (_clientState == MQTT_STATE_DISCONNECTED) {
        /* got disconnected, try to reconnect */
        ret = esp_mqtt_client_reconnect(_mqttClient);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to reconnect to MQTT broker.");
        }
        _clientState = MQTT_STATE_STARTING;
        return;
    }

    ret = esp_mqtt_client_start(_mqttClient);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client");
        return;
    }
    _clientState = MQTT_STATE_STARTING;
    ESP_LOGI(TAG, "MQTT client started.");
}

void MqttMailingService::stopMqttClient() {
    esp_err_t ret;

    if (_clientState == MQTT_STATE_INIT) {
        // nothing to do
        return;
    }

    ret = esp_mqtt_client_stop(_mqttClient);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop MQTT client");
        return;
    }
    _clientState = MQTT_STATE_INIT;
    ESP_LOGI(TAG, "MQTT client stopped.");
}

void MqttMailingService::initMqttClient() {
    // Setup mqtt event loop
    esp_event_loop_args_t mqtt_loop_args = {.queue_size = MQTT_EVENT_LOOP_LEN,
                                            .task_name = "MQTT Event Loop",
                                            .task_priority =
                                                uxTaskPriorityGet(NULL),
                                            .task_stack_size = 2 * 1024,
                                            .task_core_id = tskNO_AFFINITY};
    esp_event_loop_handle_t mqtt_loop_handle;
    esp_event_loop_create(&mqtt_loop_args, &mqtt_loop_handle);

    // Config
    esp_mqtt_client_config_t mqtt_cfg = {.event_loop_handle = mqtt_loop_handle,
                                         .uri = _brokerFullURI,
                                         .lwt_topic = _lwtTopic,
                                         .lwt_msg = _lwtMessage};
    if (_useSsl) {
        mqtt_cfg.cert_pem = _sslCert;
    }
    _mqttClient = esp_mqtt_client_init(&mqtt_cfg);
    if (!_mqttClient) {
        ESP_LOGE(TAG, "Fatal error: Could not initiate MQTT Client. Aborting.");
        assert(0);
    }
    _clientState = MQTT_STATE_INIT;
    esp_mqtt_client_register_event(
        _mqttClient, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID),
        _mqttEventHandler, this);
    ESP_LOGI(TAG, "MQTT client initialized.");
}

void MqttMailingService::destroyMqttClient() {
    _clientState = MQTT_STATE_UNINIT;
    esp_mqtt_client_stop(_mqttClient);
    esp_mqtt_client_destroy(_mqttClient);
    _mqttClient = nullptr;
    ESP_LOGI(TAG, "MQTT client has been destroyed.");
}

QueueHandle_t MqttMailingService::getDataEventQueueHandle() const {
    return _mailbox;
}

esp_mqtt_client_handle_t MqttMailingService::getMQTTClientHandle() const {
    return _mqttClient;
}

/*
 *   Private
 */

void MqttMailingService::_mailmanTask(void* arg) {
    MqttMailingService* mqttMgr = reinterpret_cast<MqttMailingService*>(arg);

    while (1) {
        MQTTMessage outMail;
        while (mqttMgr->_clientState == MQTT_STATE_CONNECTED &&
               mqttMgr->_mailmanIsRunning &&
               xQueueReceive(mqttMgr->_mailbox, &outMail, 0) == pdTRUE) {
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mqtt_client_publish(
                mqttMgr->_mqttClient, (char*)outMail.topic,
                (char*)outMail.payload, outMail.len, mqttMgr->_qos,
                mqttMgr->_retainFlag));
            ESP_LOGI(mqttMgr->TAG, "Posted a message:\n\tTopic: %s\n\tMsg: %s",
                     outMail.topic, outMail.payload);
        }

        vTaskDelay(pdMS_TO_TICKS(MQTT_MAILBOX_COLLECTION_INTERVAL_MS));
    }
}

/**
 * See definition of esp_mqtt_event_id_t for list and description of events
 * thrown by MQTT client
 */
void MqttMailingService::_mqttEventHandler(void* handler_args,
                                           esp_event_base_t base,
                                           int32_t event_id, void* event_data) {
    MqttMailingService* mqttMgr =
        reinterpret_cast<MqttMailingService*>(handler_args);

    ESP_LOGD(mqttMgr->TAG,
             "Event dispatched from event loop base=%s, event_id=%" PRIi32 "",
             base, event_id);
    esp_mqtt_event_handle_t event =
        reinterpret_cast<esp_mqtt_event_handle_t>(event_data);
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(mqttMgr->TAG, "MQTT_EVENT_CONNECTED");
            if (mqttMgr->_mailmanTaskHandle == nullptr) {
                xTaskCreate(mqttMgr->_mailmanTask, "MQTT MsgDispatcher",
                            5 * 1024, reinterpret_cast<void*>(mqttMgr),
                            tskIDLE_PRIORITY + 1,
                            &(mqttMgr->_mailmanTaskHandle));
                ESP_LOGI(mqttMgr->TAG, "Mailman task launched.");
            }
            mqttMgr->_clientState = MQTT_STATE_CONNECTED;
            mqttMgr->_mailmanIsRunning = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(mqttMgr->TAG, "MQTT_EVENT_DISCONNECTED");
            mqttMgr->_mailmanIsRunning = false;
            mqttMgr->_clientState = MQTT_STATE_DISCONNECTED;
            xTaskAbortDelay(mqttMgr->_mailmanTaskHandle);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGW(mqttMgr->TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            break;
    }
}
