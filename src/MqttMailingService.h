#ifndef UPT_MQTT_MAILING_SERVICE_H
#define UPT_MQTT_MAILING_SERVICE_H

#include "mqtt_cfg.h"
#include "mqtt_client.h"
#include <Arduino.h>
#include <Sensirion_UPT_Core.h>

// MQTT Message sizes
#define MQTT_TOPIC_SUFFIX_MAX_LENGTH 128
#define MQTT_TOPIC_PREFIX_MAX_LENGTH 128

#define MQTT_MEASUREMENT_MESSAGE_MAX_LENGTH 128

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
    static const uint8_t MAX_URI_LENGTH = 63;

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
     * @brief Sets the broker URI used to send the MQTT messages.
     *
     * @note Must be called before start()
     *
     * @param uri: The chosen broker URI (e.g. mqtt://mybroker.com:1883), max 63 characters long
     * @return True if broker URI is set successfuly, False if provided uri is too long
     */
    __attribute__((unused)) bool setBrokerURI(const char* uri);

    /**
     * @brief Sets the broker URI used to send the MQTT messages.
     *        If you set the parameter hasSsl to True, you 
     *        need to configure the Ssl certificate by either calling
     *        setSslCertificate() or setting the environment variable and calling enableSsl().
     *        If you need to configure a special server port other than the MQTT default ports
     *        (1883 without Ssl, 8883 with Ssl) use setBrokerURI().
     *  
     * @note Must be called before start()
     *
     * @param brokerDomain: domain name of your MQTT broker (e.g. mymqttbroker.com)
     * @param hasSsl: True if your broker uses SSL, False otherwise 
     * @return True if broker URI is set successfuly, False if provided brokerDomain is too long
     */
    __attribute__((unused)) bool setBroker(const char* brokerDomain, 
                                           const bool hasSsl);


    /**
     * @brief Sets the Last Will Testament (LWT) topic.
     *
     * @note Must be called before start()
     *
     * @param topic: The chosen LWT topic
     */
    __attribute__((unused)) void setLWTTopic(const char* topic);

    /**
     * @brief Sets the Last Will Testament (LWT) message.
     *
     * @note Must be called before start()
     *
     * @param topic: The chosen LWT message
     */
    __attribute__((unused)) void setLWTMessage(const char* msg);

    /**
     * @brief Set the Ssl Certificate of the MQTT broker and enables SSL
     *
     * @note The memory for the certificate needs to be managed by
     *       the callee.
     *
     * @param sslCert: The root certificate of the MQTT broker.
     */
    __attribute__((unused)) void setSslCertificate(const char* sslCert);

        /**
     * @brief Enable Ssl and set the Ssl Certificate of the MQTT broker
     *        to the one stored in MQTT_BROKER_CERTIFICATE_OVERRIDE
     *
     * @note Setting MQTT_BROKER_CERTIFICATE_OVERRIDE needs to be managed by
     *       the callee.
     *
     */
    __attribute__((unused)) void enableSsl();


    /**
     * @brief Set the Quality of Service (QoS) for the sent MQTT messages
     *
     * @param qos: chosen QoS as integer
     */
    __attribute__((unused)) void setQOS(int qos);

    /**
     * @brief Set the topic prefix that will automatically be added to all sent messages.
     * Resulting topic will be: globalPrefix + messageTopicSuffix
     * 
     * @note Max length is MQTT_TOPIC_PREFIX_MAX_LENGTH
     *
     * @param topicPrefix: chosen topic prefix string
     */
    __attribute__((unused)) void setGlobalTopicPrefix(const char* topicPrefix);

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

    /**
     * @brief Set a measurement formatting function
     *
     * @param fFmt: the function formatting a Measurement into a string
     */
    __attribute__((unused)) void setMeasurementMessageFormatterFn(void (*fFmt)(Measurement, char*));

    /**
     * @brief Set the function used to define the topic suffix from the Measurement
     *
     * @param fFmt: the function transforming a Measurement into a topic suffix
     */
    __attribute__((unused)) void setMeasurementToTopicSuffixFn(void (*fFmt)(Measurement, char*));

    /**
     * @brief Send a message to a given topic.
     *
     * @param message: the message as a string. Maximum 256 characters.
     * @param topicSuffix the topic suffix (will be combined with the global prefix) 
     * 
     * @return true is message was successfully sent
     */
    __attribute__((unused)) bool sendTextMessage(const char* message, const char* topicSuffix);

    /**
     * @brief Send a measurement to a given topic.
     * 
     * @note Formatter needs to be set first (using setMeasurementMessageFormatterFn)
     *
     * @param measurement: the Measurement to send
     * @param topicSuffix the topic suffix (will be combined with the global prefix) 
     * 
     * @return true is message was successfully sent
     */
    __attribute__((unused)) bool sendMeasurement(const Measurement measurement, const char* topicSuffix);

    /**
     * @brief Send a measurement to a topic defined using the registered function.
     * 
     * @note Formatter needs to be set first (using setMeasurementMessageFormatterFn)
     * @note Topic suffix function needs to be set first (using setMeasurementToTopicSuffixFn)
     *
     * @param measurement: the Measurement to send
     * 
     * @return true is message was successfully sent
     */
    __attribute__((unused)) bool sendMeasurement(const Measurement measurement);

  private:
    MqttMailingServiceState mState;
    char mBrokerFullURI[MAX_URI_LENGTH + 1]{};
    char mLwtTopic[128]{};
    char mLwtMessage[256]{};
    bool mUseSsl;
    const char* mSslCert;
    int mQos;
    int mRetainFlag;
    char mGlobalTopicPrefix[MQTT_TOPIC_PREFIX_MAX_LENGTH] = "";
    TaskHandle_t mWifiCheckTaskHandle = nullptr;
    // Pointer to formatting function
    void (*mMeasurementFormatterFn)(Measurement, char*) = nullptr;
    void (*mTopicSuffixFn)(Measurement, char*) = nullptr;

    // ESP MQTT client
    static esp_mqtt_client_handle_t mEspMqttClient;
    void initEspMqttClient();
    void startEspMqttClient();
    void destroyEspMqttClient();

    //  Forward function
    bool fwdMqttMessage(const char* topic, const char* message);

    // Wi-fi related
    bool mShouldManageWifiConnection = false;
    [[noreturn]] static void wifiCheckTaskCode(__attribute__((unused)) void* arg);

    // Event handler
    static void
    espMqttEventHandler(void* handler_args,
                         __attribute__((unused)) esp_event_base_t base,
                         int32_t event_id, void* event_data);
};

#endif /* UPT_MQTT_MAILING_SERVICE_H */
