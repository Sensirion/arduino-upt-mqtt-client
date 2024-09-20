#include "WiFiManager.h"

const char* WiFiManager::TAG = "WiFi Manager";

WiFiManager::WiFiManager() {
    // Ensure previous configuration is reset
    WiFi.disconnect(true);
    delay(500);

    // Add event handlers
    _initializeEventHandlers();
}

WiFiManager::WiFiManager(char* ssid, char* pass) {
    strncpy(_ssid, ssid, 63);
    strncpy(_password, pass, 63);

    WiFiManager();
}

WiFiManager::~WiFiManager() {
    WiFi.disconnect(true);
}

void WiFiManager::start() {
    WiFi.begin(_ssid, _password);
}

bool WiFiManager::is_connected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::_initializeEventHandlers() {
    WiFi.onEvent(&_onWiFiConnected,
                 WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(&_onWiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(&_onWiFiDisconnected,
                 WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}

/*
 *   Event Handlers
 */

void _onWiFiConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    ESP_LOGI("WiFi Manager", "Connected to WiFi AP successfully!");
}

void _onWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
    ESP_LOGI("WiFi Manager", "Got an IP from WiFi AP:");
    ESP_LOGI("WiFi Manager", "%s", WiFi.localIP());
}

void _onWiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    ESP_LOGW("WiFi Manager", "Got disconnected from WiFi AP (code %i)",
             info.wifi_sta_disconnected.reason);
    ESP_LOGW("WiFi Manager", "Trying to reconnect...");
    WiFi.reconnect();
}
