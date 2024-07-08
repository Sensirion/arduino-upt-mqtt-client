#ifndef _MQTT_MANAGER_H_
#define _MQTT_MANAGER_H_

#include "WiFiManager.h"
#include "mqtt_cfg.h"
#include "mqtt_client.h"
#include <Arduino.h>
#include <WiFi.h>

/* Class managing MQTT message dispatch. Uses a WiFi client. */
class MQTTMessageDispatcher {
  public:
    MQTTMessageDispatcher();
    ~MQTTMessageDispatcher();

    /**
     * @brief Starts the MQTTMessageDispatcher with WiFi informations
     *
     * @param ssid: The WiFi netwtwork SSID
     * @param pass: The WiFi network password
     */
    void startWithWifiConnection(const char* ssid, const char* pass);

    /**
     * @brief Starts the MQTTMessageDispatcher with WiFi informations and the
     *        MQTT broker URI
     *
     * @param ssid: The WiFi netwtwork SSID
     * @param pass: The WiFi network password
     * @param uri: The broker URI
     */
    void startWithWifiConnection(const char* ssid, const char* pass,
                                 const char* uri);

    /**
     * @brief Starts the MQTTMessageDispatcher with a provided
     *        EventGroupHandle_t of WiFi events.
     *
     * @param wifiEventGroup: The EventGroupHandle_t of WiFi events
     */
    void start(EventGroupHandle_t wifiEventGroup);

    /**
     * @brief Starts the MQTTMessageDispatcher with a provided
     *        EventGroupHandle_t of WiFi events.
     *
     * @param wifiEventGroup: The EventGroupHandle_t of WiFi events
     * @param uri: The broker URI
     */
    void start(EventGroupHandle_t wifiEventGroup, const char* uri);

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
     * @brief Starts the ESP MQTT client
     */
    void startMqttClient();

    /**
     * @brief Stops the ESP MQTT client
     */
    void stopMqttClient();

    /**
     * @brief Initializes the ESP MQTT client
     */
    void initMqttClient();

    /**
     * @brief Destroys the ESP MQTT client
     */
    void destroyMqttClient();

    /* Utilities */

    /**
     * @brief returns the handle of the data envent queue.
     */
    QueueHandle_t getDataEventQueueHandle() const;

    /**
     * @brief returns the event group containing internet connectivity flags.
     * Useful if WiFi client was started using startWithWifiConnection() and
     * information is required elsewhere
     *
     * @note See event_source.h for flag definitions, and
     * WiFiManager::_eventHandler for flag usage
     *
     * @param[out] wifiEventGroupHandle containing flags with the status of the
     * current internet connection
     */
    EventGroupHandle_t getWifiEventGroupHandle() const;

    /**
     * @brief returns the handle to the ESP MQTT client usied by the MQTT
     * Manager
     *
     * @return esp_mqtt_client_handle_t
     */
    esp_mqtt_client_handle_t getMQTTClientHandle() const;

  private:
    static const char* TAG;
    // Wifi related
    WiFiManager* _pWifiMgr = nullptr;
    EventGroupHandle_t _wifiEventGroup = nullptr;
    void _onWiFiConnected();
    void _onWiFiDisconnected();
    static void _wifiConnectionMonitor(void* pMQTTMessageDispatcherInstance);

    // MQTT related
    static esp_mqtt_client_handle_t _mqttClient;
    enum MqttClientState {
        MQTT_STATE_UNINIT = 0,
        MQTT_STATE_INIT,
        MQTT_STATE_DISCONNECTED,
        MQTT_STATE_STARTING,
        MQTT_STATE_CONNECTED,
    } _clientState = MQTT_STATE_UNINIT;
    bool _mailmanIsRunning = false;
    char _brokerFullURI[64];
    char _lwtTopic[128];
    char _lwtMessage[256];
    bool _useSsl;
    const char* _sslCert;
    int _qos;
    int _retainFlag;
    TaskHandle_t _mailmanTaskHandle = nullptr;
    static void _mailmanTask(void* pMQTTMessageDispatcherInstance);
    static void _mqttEventHandler(void*, esp_event_base_t, int32_t, void*);

    // Mailbox
    QueueHandle_t _mailbox = nullptr;
};

#endif /* _MQTT_MANAGER_H_ */
