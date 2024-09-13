#ifndef _WIFI_MANAGER_H_
#define _WIFI_MANAGER_H_

#include "WiFi.h"
#include "wifi_cfg.h"

/* Class managing wifi connectivity */
class WiFiManager {
  public:
    WiFiManager();
    WiFiManager(char* ssid, char* pass);
    ~WiFiManager();

    /**
     * @brief Starts the connection procedure to the configured AP
     */
    void start();

    /**
     * @brief Returns true if WiFi is currently connected
     */
    bool is_connected();

  private:
    static const char* TAG;
    char _ssid[64] = WIFI_SSID_OVERRIDE;
    char _password[64] = WIFI_PW_OVERRIDE;
    void _initializeEventHandlers();
};

/*
 *   Event Handlers
 */

void _onWiFiConnected(WiFiEvent_t event, WiFiEventInfo_t info);
void _onWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
void _onWiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);

#endif /* _WIFI_MANAGER_H_ */
