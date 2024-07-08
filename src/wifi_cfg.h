/**
 * Defines macros for WiFi configuration, setting default values if not
 * previously specified during project environment setup (detected via
 * PIO_ADVANCED_SCRIPTING).
 *
 * For more details, refer to the library's README and the configuration script,
 * set_env_vars_from_json.py, in the library's root directory.
 *
 * Values are sourced from mqtt_config.json if configured by the post_script.
 */
#ifndef _WIFI_CONFIG_H_
#define _WIFI_CONFIG_H_

// Access default values.
#ifndef PIO_ADVANCED_SCRIPTING
#define WIFI_SSID_OVERRIDE "Obi-WLAN Kenobi"
#define WIFI_PW_OVERRIDE "ConnectToTheAP,IWill"
#endif /* PIO_ADVANCED_SCRIPTING */

// STA config
#define WIFI_SCAN_METHOD WIFI_ALL_CHANNEL_SCAN

#endif /* _WIFI_CONFIG_H_ */
