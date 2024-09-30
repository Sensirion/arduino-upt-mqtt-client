/**
 * Defines macros for MQTT configuration, setting default values if not
 * previously specified during project environment setup (detected via
 * PIO_ADVANCED_SCRIPTING).
 *
 * For more details, refer to the library's README and the configuration script,
 * set_env_vars_from_json.py, in the library's root directory.
 *
 * Values are sourced from mqtt_config.json if configured by the post_script.
 */

#ifndef _MQTT_CONFIG_H_
#define _MQTT_CONFIG_H_

#ifndef PIO_ADVANCED_SCRIPTING
#define MQTT_LWT_TOPIC_OVERRIDE "myTopic/"
#define MQTT_LWT_MSG_OVERRIDE "The MQTT Mailman unexpectedly disconnected."
#define MQTT_QOS_OVERRIDE 0
#define MQTT_RETAIN_FLAG_OVERRIDE 0
#define MQTT_BROKER_FULL_URI_OVERRIDE "mqtt://mymqttbroker.com:1883"
#define MQTT_BROKER_CERTIFICATE_OVERRIDE ""
#define MQTT_USE_SSL_OVERRIDE 0
#endif /* PIO_ADVANCED_SCRIPTING */

#define MQTT_DATAQUEUE_LEN 10
#define MQTT_EVENT_LOOP_LEN 20
#define MQTT_MAILBOX_COLLECTION_INTERVAL_MS 100

#define WIFI_CHECK_INTERVAL_MS 10000

#endif /* _MQTT_CONFIG_H_ */
