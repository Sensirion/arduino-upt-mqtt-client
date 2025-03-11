/**
 * Defines macros for MQTT configuration, setting default values if not
 * previously specified.
 * 
 * For more details, refer to the library's README.
 */

#ifndef MQTT_CONFIG_H_
#define MQTT_CONFIG_H_


/** 
 * 
 * make sure to call `enableWifi and setBroker with parameter hasSsl set to true
 * to enabled Ssl 
 */ 
#ifndef MQTT_BROKER_CERTIFICATE_OVERRIDE
#define MQTT_BROKER_CERTIFICATE_OVERRIDE ""
#endif


/** WiFi SSID and PW and check interval are only used in case you use the `startWithDelegatedWiFi` API */
#ifndef WIFI_SSID_OVERRIDE
#define WIFI_SSID_OVERRIDE "Obi-WLAN Kenobi"
#endif

#ifndef WIFI_PW_OVERRIDE
#define WIFI_PW_OVERRIDE "ConnectToTheAP,IWill"
#endif

#endif /* MQTT_CONFIG_H_ */
