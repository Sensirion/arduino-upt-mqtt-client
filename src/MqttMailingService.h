#ifndef UPT_MQTT_MAILING_SERVICE_H
#define UPT_MQTT_MAILING_SERVICE_H

#include "event_source.h"
#include "mqtt_cfg.h"
#include "mqtt_client.h"
#include <Arduino.h>

enum MqttMailingServiceState {
    UNINITIALIZED = 0,
    INITIALIZED,
    CONNECTING,
    CONNECTED,
    DISCONNECTED,
};

/* Class managing MQTT message dispatch. Optionally manages Wi-Fi connection. */
class __attribute__((unused)) MqttMailingService {
  public:
    MqttMailingService();
    ~MqttMailingService();

    /**
     * @brief Starts the MqttMailingService
     *
     * @note If Wi-Fi connection is not established when calling start, the
     *       mqtt client will print out connection errors. It will however
     *       automatically recover once a connection is established.
     */
    void start();

    /**
     * @brief Starts the MqttMailingService without an established Wi-Fi
     * connection. The MqttMailingService will handle connectivity  by itself.
     *
     * @param ssid: The SSID of the WiFi AP
     * @param pass: the password of the WiFi AP
     * @param should_be_blocking: Specify if the call should be blocking until
     * connection is established with broker
     */
    __attribute__((unused)) void
    startWithDelegatedWiFi(const char* ssid, const char* pass,
                           const bool shouldBeBlocking);

    /**
     * @brief Starts the MqttMailingService without an established Wi-Fi
     * connection. The MqttMailingService will handle connectivity  by itself.
     *
     * @param ssid: The SSID of the WiFi AP
     *
     * @param pass: the password of the WiFi AP
     *
     */
    __attribute__((unused)) void startWithDelegatedWiFi(const char* ssid,
                                                        const char* pass);

    /**
     * @brief Starts the MqttMailing Service without an established
     *        Wi-Fi connection and uses the environment variables
     *        WIFI_SSID_OVERRIDE and WIFI_PW_OVERRIDE to configure the
     *        Wi-Fi ssid and password.
     */
    __attribute__((unused)) void startWithDelegatedWiFi();

    /**
     * @brief Sets the broker URI used to send the MQTT messages. It will
     *        overwrite the broker URI taken from the configuration.
     *
     * @note Must be called before start()
     *
     * @param uri: The chosen broker URI
     */
    __attribute__((unused)) void setBrokerURI(const char* uri);

    /**
     * @brief Sets the Last Will Testament (LWT) topic.It will
     *        overwrite the LWT topic taken from the configuration.
     *
     * @note Must be called before start()
     *
     * @param topic: The chosen LWT topic
     */
    __attribute__((unused)) void setLWTTopic(const char* topic);

    /**
     * @brief Sets the Last Will Testament (LWT) message.It will
     *        overwrite the LWT message taken from the configuration.
     *
     * @note Must be called before start()
     *
     * @param topic: The chosen LWT message
     */
    __attribute__((unused)) void setLWTMessage(const char* msg);

    /**
     * @brief Set the Ssl Certificate of the MQTT broker
     *
     * @note The memory for the certificate needs to be managed by
     *       the callee.
     *
     * @param sslCert: The root certificate of the MQTT broker.
     */
    __attribute__((unused)) void setSslCertificate(const char* sslCert);

    /**
     * @brief Set the Quality of Service (QoS) for the sent MQTT messages
     *
     * @param qos: chosen QoS as integer
     */
    __attribute__((unused)) void setQOS(int qos);

    /**
     * @brief Set the retain flag for the sent MQTT messages
     *
     * @param flag: the chosen flag as integer
     */
    __attribute__((unused)) void setRetainFlag(int flag);

    /**
     * @brief returns the QueueHandle_t to the mailbox
     *
     * @note: The mailbox is only available once initialized
     */
    __attribute__((unused)) QueueHandle_t getMailbox() const;

    /**
     * @brief returns the state of the service
     */
    __attribute__((unused)) MqttMailingServiceState getServiceState();

    /**
     * @brief returns whether the service has successfully established
     * a connection and is ready to forward messages to the broker
     */
    __attribute__((unused)) bool isReady();

  private:
    static const char* TAG;
    MqttMailingServiceState _state;
    char _brokerFullURI[64]{};
    char _lwtTopic[128]{};
    char _lwtMessage[256]{};
    bool _useSsl;
    const char* _sslCert;
    int _qos;
    int _retainFlag;
    TaskHandle_t _mailmanTaskHandle = nullptr;
    TaskHandle_t _wifiCheckTaskHandle = nullptr;

    // ESP MQTT client
    static esp_mqtt_client_handle_t _espMqttClient;
    void _initEspMqttClient();
    void _startEspMqttClient();
    void _destroyEspMqttClient();

    // Mailbox
    QueueHandle_t _mailbox = nullptr;

    // Wi-fi related
    bool _should_manage_wifi_connection = false;
    [[noreturn]] static void _wifiCheckTask(__attribute__((unused)) void* arg);

    // Running task definition
    [[noreturn]] static void _mailmanTask(void* pMqttMailingService);
    // Event handler
    static void
    _espMqttEventHandler(void* handler_args,
                         __attribute__((unused)) esp_event_base_t base,
                         int32_t event_id, void* event_data);
};

#endif /* UPT_MQTT_MAILING_SERVICE_H */
