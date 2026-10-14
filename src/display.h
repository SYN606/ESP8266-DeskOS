#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "config.h"
#include "pins.h"
#include "eyes.h"
#include "telemetry.h"

enum class DisplayMode {
    STARTUP,
    ROBOT_EYES,
    PC_STATS,
    MUSIC_PLAYER,
    QR_CODE,
    CLOCK
};

class DeskDisplay {
public:
    DeskDisplay();

    bool begin();
    void update();

    // Mode management
    void setMode(DisplayMode mode);
    void setMode(const char* modeStr);
    DisplayMode getMode() const { return m_mode; }
    const char* getModeString() const;
    void nextMode();

    // Specific screen renderers
    void drawStartupScreen();
    void drawRobotEyes();
    void drawPCStats(const TelemetryData& stats);
    void drawMusicPlayer(const MediaData& media);
    void drawQRCode(const char* text);
    void drawClock(const char* timeStr, const char* dateStr, bool wifiOk, const char* ipStr);

    // Dynamic payloads
    void setQRCode(const char* text);
    void setMedia(const char* title, const char* artist, bool isPlaying);

    // Display contrast / brightness (0 - 255)
    void setBrightness(uint8_t level);
    uint8_t getBrightness() const { return m_brightness; }

    static uint8_t scanI2CBus();
    bool isReady() const { return m_initialized; }
    U8G2& getDriver() { return m_u8g2; }

private:
#if OLED_CONTROLLER_SH1106
    U8G2_SH1106_128X64_NONAME_F_HW_I2C m_u8g2;
#else
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C m_u8g2;
#endif
    bool m_initialized;
    DisplayMode m_mode;
    uint8_t m_brightness;

    unsigned long m_lastDrawMs;
    unsigned long m_startupUntilMs;
    float m_smoothCpu;
    float m_smoothRam;
    int16_t m_musicScrollX;
    uint8_t m_eqPhase;
};

extern DeskDisplay deskDisplay;
