#pragma once

#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <DNSServer.h>

#include <cstdint>
#include <string>

struct WifiStatusSnapshot {
    IPAddress ap_ip;
    IPAddress sta_ip;
    bool sta_connected = false;
    std::string sta_ssid;
};

class WebServerManager {
public:
    static WebServerManager& instance();

    void begin();
    void loop();
    void notifyConfigChanged();
    void disableAP();
    WifiStatusSnapshot getStatusSnapshot() const;
    
    // Access to web server for plugin registration
    AsyncWebServer& getServer() { return server_; }

private:
    WebServerManager();

    void setupRoutes();
    void configureWifi();

    AsyncWebServer server_;
    DNSServer dns_server_;
    bool sta_connected_ = false;
    bool events_registered_ = false;
    IPAddress ap_ip_{0, 0, 0, 0};
    IPAddress sta_ip_{0, 0, 0, 0};
    std::string sta_ssid_;
    bool wifi_reconfigure_pending_ = false;
    std::uint32_t wifi_reconfigure_request_ms_ = 0;
    bool ap_suppressed_ = false;
    bool dns_active_ = false;
};
