#include "WiFiManager.h"

const char* WiFiManager::TAG = "WiFi Station";
EventGroupHandle_t WiFiManager::_wifiEventGroup = nullptr;

WiFiManager::WiFiManager() {
    strncpy(_ssid, WIFI_SSID_OVERRIDE, 63);
    strncpy(_password, WIFI_PW_OVERRIDE, 63);
}

WiFiManager::~WiFiManager() {
    WiFi.disconnect();
    xEventGroupClearBits(_wifiEventGroup, WIFI_GOT_IP_BIT);
    xEventGroupSetBits(_wifiEventGroup, WIFI_LOST_IP_BIT);
    vEventGroupDelete(_wifiEventGroup);
}

void WiFiManager::start() {
    return start(_ssid, _password);
}

void WiFiManager::start(const char* SSID, const char* pass) {
    return start(SSID, pass, WIFI_SCAN_METHOD);
}

void WiFiManager::start(const char* SSID, const char* pass,
                        wifi_scan_method_t scanMethod) {
    _wifiEventGroup = xEventGroupCreate();
    xEventGroupClearBits(_wifiEventGroup, WIFI_GOT_IP_BIT);
    xEventGroupSetBits(_wifiEventGroup, WIFI_LOST_IP_BIT);

    WiFi.setScanMethod(scanMethod);
    WiFi.onEvent(&_eventHandler);
    WiFi.begin(SSID, pass);
}

EventGroupHandle_t WiFiManager::getEventGroupHandle() {
    return _wifiEventGroup;
}

/**
 * See arduino_event_id_t definition for list of available events
 */
void WiFiManager::_eventHandler(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_READY:
            ESP_LOGD(TAG, "WiFi interface ready");
            break;
        case ARDUINO_EVENT_WIFI_STA_START:
            ESP_LOGD(TAG, "WiFi client started");
            break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            ESP_LOGD(TAG, "WiFi clients stopped");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            ESP_LOGI(TAG, "Connected to access point %s.", WiFi.SSID());
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected from WiFi access point");
            xEventGroupClearBits(_wifiEventGroup, WIFI_GOT_IP_BIT);
            xEventGroupSetBits(_wifiEventGroup, WIFI_LOST_IP_BIT);
            break;
        case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
            ESP_LOGD(TAG, "Authentication mode of access point has changed");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            ESP_LOGI(TAG, "Obtained IP address: %s", WiFi.localIP().toString());
            xEventGroupClearBits(_wifiEventGroup, WIFI_LOST_IP_BIT);
            xEventGroupSetBits(_wifiEventGroup, WIFI_GOT_IP_BIT);
            break;
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
            ESP_LOGD(TAG, "Lost IP address and IP address is reset to 0");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
            ESP_LOGD(TAG, "STA IPv6 is preferred");
        default:
            break;
    }
}
