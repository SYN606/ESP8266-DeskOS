#pragma once

#include <Arduino.h>

struct MediaData {
    char title[48] = "";
    char artist[32] = "";
    bool isPlaying = false;
    unsigned long lastUpdateMs = 0;
};

struct TelemetryData {
    float cpuPercent = 0.0f;
    float ramPercent = 0.0f;
    float tempC = 0.0f;
    float netKbIn = 0.0f;
    float netKbOut = 0.0f;
    char uptimeStr[32] = "00h 00m";
    unsigned long lastReceivedMs = 0;
    bool hasData = false;
};

extern TelemetryData globalTelemetry;
extern MediaData globalMedia;
extern char globalQRCodeText[96];
