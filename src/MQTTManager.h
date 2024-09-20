#ifndef _MQTT_MANAGER_H_
#define _MQTT_MANAGER_H_

#include "WiFiManager.h"
#include "event_source.h"
#include "mqtt_cfg.h"
#include "mqtt_client.h"
#include <Arduino.h>
#include <WiFi.h>

enum MqttMailingServiceState {
    UNINITIALIZED = 0,
    INITIALIZED,
    CONNECTING,
    CONNECTED,
    DISCONNECTED,
    RECONNECTING,
};

/* Class managing MQTT message dispatch. Uses a WiFi client. */
class MqttMailingService {
  public:
    MqttMailingService();
    ~MqttMailingService();

    /**
     * @brief Starts the MqttMailingService
     */
    void start();

    /**
     * @brief Sets the broker URI used to send the MQTT messages. It will
     *        overwrite the broker URI taken from the configuration.
     *
     * @note Must be called before start()
     *
     * @param uri: The chosen broker URI
     */
    void setBrokerURI(const char* uri);

    /**
     * @brief Sets the Last Will Testament (LWT) topic.It will
     *        overwrite the LWT topic taken from the configuration.
     *
     * @note Must be called before start()
     *
     * @param topic: The chosen LWT topic
     */
    void setLWTTopic(const char* topic);

    /**
     * @brief Sets the Last Will Testament (LWT) message.It will
     *        overwrite the LWT message taken from the configuration.
     *
     * @note Must be called before start()
     *
     * @param topic: The chosen LWT message
     */
    void setLWTMessage(const char* msg);

    /**
     * @brief Set the Ssl Certificate of the MQTT broker
     *
     * @note The memory for the certificate needs to be managed by
     *       the callee.
     *
     * @param sslCert: The root certificate of the MQTT broker.
     */
    void setSslCertificate(const char* sslCert);

    /**
     * @brief Set the Quality of Service (QoS) for the sent MQTT messages
     *
     * @param qos: chosen QoS as integer
     */
    void setQOS(int qos);

    /**
     * @brief Set the retain flag for the sent MQTT messages
     *
     * @param flag: the chosen flag as integer
     */
    void setRetainFlag(int flag);

    /**
     * @brief returns the QueueHandle_t to the mailbox
     *
     * @note: The mailbox is only available once initialized
     */
    QueueHandle_t getMailbox() const;

    /**
     * @brief returns the state of the service
     */
    MqttMailingServiceState getServiceState();

  private:
    static const char* TAG;
    MqttMailingServiceState _state;
    char _brokerFullURI[64];
    char _lwtTopic[128];
    char _lwtMessage[256];
    bool _useSsl;
    const char* _sslCert;
    int _qos;
    int _retainFlag;
    TaskHandle_t _mailmanTaskHandle = nullptr;
    static void _mqttEventHandler(void*, esp_event_base_t, int32_t, void*);

    // ESP MQTT client
    static esp_mqtt_client_handle_t _espMqttClient;
    void _initEspMqttClient();
    void _startEspMqttClient();
    void _stopEspMqttClient();
    void _destroyEspMqttClient();

    // Mailbox
    QueueHandle_t _mailbox = nullptr;

    // Wifi related
    bool _should_manage_wifi_connection = false;
    WiFiManager* _pWifiMgr = nullptr;

    // Running task definition
    static void _mailmanTask(void* pMQTTMessageDispatcherInstance);

    // Event handler
    static void _espMqttEventHandler(void* handler_args, esp_event_base_t base,
                                     int32_t event_id, void* event_data);
};

#endif /* _MQTT_MANAGER_H_ */
