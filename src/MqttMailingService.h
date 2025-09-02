#ifndef UPT_MQTT_MAILING_SERVICE_H
#define UPT_MQTT_MAILING_SERVICE_H

#include "mqtt_cfg.h"
#include "mqtt_client.h"
#include <Arduino.h>
#include <Sensirion_UPT_Core.h>

namespace sensirion::upt::mqtt{

using MeasurementFormatterType = std::function<std::string(const sensirion::upt::core::Measurement&)>;

enum MqttMailingServiceState {
    UNINITIALIZED = 0,
    INITIALIZED,
    CONNECTING,
    CONNECTED,
    DISCONNECTED,
};

/* Class managing MQTT message dispatch. Optionally manages Wi-Fi connection. */
class MqttMailingService {
  public:

    MqttMailingService();
    ~MqttMailingService();

    // we must not copy the MqttMailingService
    const MqttMailingService& operator()(MqttMailingService&&) = delete;

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
    [[maybe_unused]] void
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
    [[maybe_unused]] void startWithDelegatedWiFi(const char* ssid,
                                                        const char* pass);

    /**
     * @brief Starts the MqttMailing Service without an established
     *        Wi-Fi connection and uses the environment variables
     *        WIFI_SSID_OVERRIDE and WIFI_PW_OVERRIDE to configure the
     *        Wi-Fi ssid and password.
     */
    [[maybe_unused]] void startWithDelegatedWiFi();

    /**
     * @brief Sets the broker URI used to send the MQTT messages.
     *
     * @note Must be called before start()
     *
     * @param uri: The chosen broker URI (e.g. mqtt://mybroker.com:1883), max 63 characters long
     * @return True if broker URI is set successfuly, False if provided uri is too long
     */
    [[maybe_unused]] bool setBrokerURI(std::string&& uri);

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
    [[maybe_unused]] bool setBroker(std::string&& brokerDomain, 
                                    const bool hasSsl);


    /**
     * @brief Sets the Last Will Testament (LWT) topic.
     *
     * @note Must be called before start()
     *
     * @param topic: The chosen LWT topic
     */
    [[maybe_unused]] void setLWTTopic(std::string&& topic);

    /**
     * @brief Sets the Last Will Testament (LWT) message.
     *
     * @note Must be called before start()
     *
     * @param topic: The chosen LWT message
     */
    [[maybe_unused]] void setLWTMessage(std::string&& msg);

    /**
     * @brief Set the Ssl Certificate of the MQTT broker and enables SSL
     *
     * @note The memory for the certificate needs to be managed by
     *       the callee.
     *
     * @param sslCert: The root certificate of the MQTT broker.
     */
    [[maybe_unused]] void setSslCertificate(std::string&& sslCert);

        /**
     * @brief Enable Ssl and set the Ssl Certificate of the MQTT broker
     *        to the one stored in MQTT_BROKER_CERTIFICATE_OVERRIDE
     *
     * @note Setting MQTT_BROKER_CERTIFICATE_OVERRIDE needs to be managed by
     *       the callee.
     *
     */
    [[maybe_unused]] void enableSsl();


    /**
     * @brief Set the Quality of Service (QoS) for the sent MQTT messages
     *
     * @param qos: chosen QoS as integer
     */
    [[maybe_unused]] void setQOS(int qos);

    /**
     * @brief Set the topic prefix that will automatically be added to all sent messages.
     * Resulting topic will be: globalPrefix + messageTopicSuffix
     * 
     * @note Max length is MQTT_TOPIC_PREFIX_MAX_LENGTH
     *
     * @param topicPrefix: chosen topic prefix string
     */
    [[maybe_unused]] void setGlobalTopicPrefix(std::string&& topicPrefix);

    /**
     * @brief Set the retain flag for the sent MQTT messages
     *
     * @param flag: the chosen flag as integer
     */
    [[maybe_unused]] void setRetainFlag(int flag);

    /**
     * @brief returns the QueueHandle_t to the mailbox
     *
     * @note: The mailbox is only available once initialized
     */
    [[maybe_unused]] QueueHandle_t getMailbox() const;

    /**
     * @brief returns the state of the service
     */
    [[maybe_unused]] MqttMailingServiceState getServiceState();

    /**
     * @brief returns whether the service has successfully established
     * a connection and is ready to forward messages to the broker
     */
    [[maybe_unused]] bool isReady();

    /**
     * @brief Set a measurement formatting function
     *
     * @param fFmt: the function formatting a Measurement into a string
     */
    [[maybe_unused]] void setMeasurementMessageFormatterFn(MeasurementFormatterType formatterFunction);

    /**
     * @brief Set the function used to define the topic suffix from the Measurement
     *
     * @param fFmt: the function transforming a Measurement into a topic suffix
     */
    [[maybe_unused]] void setMeasurementToTopicSuffixFn(MeasurementFormatterType formatterFunction);

    /**
     * @brief Send a message to a given topic.
     *
     * @param message: the message as a string. Maximum 256 characters.
     * @param topicSuffix the topic suffix (will be combined with the global prefix) 
     * 
     * @return true is message was successfully sent
     */
    [[maybe_unused]] bool sendTextMessage(const std::string& message, const std::string& topicSuffix);

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
    [[maybe_unused]] bool sendMeasurement(const core::Measurement measurement, const std::string& topicSuffix);

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
    [[maybe_unused]] bool sendMeasurement(const sensirion::upt::core::Measurement measurement);

  private:
    MqttMailingServiceState mState;
    std::string mBrokerFullURI{};
    std::string mLwtTopic{};
    std::string mLwtMessage{};
    bool mUseSsl;
    std::string mSslCert{};
    int mQos;
    int mRetainFlag;
    std::string mGlobalTopicPrefix{};
    TaskHandle_t mWifiCheckTaskHandle = nullptr;
    // Pointer to formatting function
    MeasurementFormatterType mMeasurementFormatterFn{};
    MeasurementFormatterType mTopicSuffixFn{};

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
} // end namespace

#endif /* UPT_MQTT_MAILING_SERVICE_H */
