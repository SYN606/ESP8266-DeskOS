#pragma once

#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <WebServer.h>
#endif

class DeskNetwork {
public:
    DeskNetwork();

    void begin();
    void update();

    bool isConnected() const { return m_connected; }
    const char* getIPString() const { return m_ipString; }
    const char* getSSID() const { return m_ssid; }
    int getRSSI() const;

    void connectWiFi(const char* ssid, const char* password);

    // Formatted time & date for clock mode
    void getTimeString(char* buf, size_t len);
    void getDateString(char* buf, size_t len);

private:
#if defined(ESP8266)
    ESP8266WebServer m_server;
#elif defined(ESP32)
    WebServer m_server;
#endif

    bool m_connected;
    bool m_isAP;
    char m_ipString[20];
    char m_ssid[33];
    unsigned long m_lastWiFiCheckMs;

    void setupServerRoutes();
    void setupNTP();
};

extern DeskNetwork deskNetwork;
