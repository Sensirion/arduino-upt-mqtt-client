#ifndef _WIFI_MANAGER_H_
#define _WIFI_MANAGER_H_

#include "WiFi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "event_source.h"
#include "wifi_cfg.h"

/* Class managing wifi connectivity */
class WiFiManager {
  public:
    WiFiManager();
    ~WiFiManager();

    /**
     * @brief Starts a WiFi manager with the credentials from configuration
     */
    void start();

    /**
     * @brief Starts a WiFi manager with the provided credentials
     *
     * @param SSID: The SSID of the WiFi network
     * @param pass: The password of the WiFi network
     */
    void start(const char* SSID, const char* pass);

    /**
     * @brief Starts a WiFi manager with the provided credentials and scan
     *        method.
     *
     * @param SSID: The SSID of the WiFi network
     * @param pass: The password of the WiFi network
     * @param scanMethod: the wifi_scan_method_t to use
     */
    void start(const char* SSID, const char* pass,
               wifi_scan_method_t scanMethod);

    /**
     * @brief Get the handle of the WiFi event group
     */
    EventGroupHandle_t getEventGroupHandle();

    WiFiClass* operator->() {
        return &WiFi;
    }

  private:
    static const char* TAG;
    static EventGroupHandle_t _wifiEventGroup;
    static void _eventHandler(WiFiEvent_t);
    char _ssid[64];
    char _password[64];
};

#endif /* _WIFI_MANAGER_H_ */
