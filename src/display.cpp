#include "display.h"
#include "network.h"
#include <qrcode.h>

DeskDisplay deskDisplay;
TelemetryData globalTelemetry;
MediaData globalMedia;
char globalQRCodeText[96] = "https://github.com/SYN606/ESP8266-DeskOS";

DeskDisplay::DeskDisplay()
    : m_u8g2(U8G2_R0, PIN_OLED_RST),
      m_initialized(false),
      m_mode(DisplayMode::STARTUP),
      m_brightness(255),
      m_lastDrawMs(0),
      m_startupUntilMs(0),
      m_smoothCpu(0.0f),
      m_smoothRam(0.0f),
      m_musicScrollX(0),
      m_eqPhase(0) {
}

bool DeskDisplay::begin() {
    Serial.println(F("[DISPLAY] Initializing I2C bus..."));
#if defined(ESP8266)
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(I2C_CLOCK_SPEED);
#else
    bool wireOk = Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_CLOCK_SPEED);
    if (!wireOk) {
        Serial.println(F("[DISPLAY] ERROR: Failed to configure I2C pins."));
        return false;
    }
#endif

    Serial.printf("[DISPLAY] I2C configured on SDA: GPIO%d, SCL: GPIO%d at %u kHz\n",
                  PIN_I2C_SDA, PIN_I2C_SCL, I2C_CLOCK_SPEED / 1000);

#if OLED_CONTROLLER_SH1106
    Serial.println(F("[DISPLAY] Controller configured: SH1106 (1.3\" 128x64)"));
#else
    Serial.println(F("[DISPLAY] Controller configured: SSD1306 (0.96\" 128x64)"));
#endif

    Serial.println(F("[DISPLAY] Initializing U8g2 driver..."));
    m_initialized = m_u8g2.begin();

    if (m_initialized) {
        m_u8g2.setContrast(m_brightness);
        m_startupUntilMs = millis() + 2500;
        Serial.println(F("[DISPLAY] U8g2 driver initialized successfully."));
    } else {
        Serial.println(F("[DISPLAY] WARNING: U8g2 begin returned false."));
    }

    return m_initialized;
}

void DeskDisplay::setBrightness(uint8_t level) {
    m_brightness = level;
    if (m_initialized) {
        m_u8g2.setContrast(level);
    }
}

void DeskDisplay::setMode(DisplayMode mode) {
    m_mode = mode;
}

void DeskDisplay::setMode(const char* modeStr) {
    if (!modeStr) return;
    if (strcasecmp(modeStr, "eyes") == 0 || strcasecmp(modeStr, "robot_eyes") == 0) m_mode = DisplayMode::ROBOT_EYES;
    else if (strcasecmp(modeStr, "stats") == 0 || strcasecmp(modeStr, "pc_stats") == 0) m_mode = DisplayMode::PC_STATS;
    else if (strcasecmp(modeStr, "music") == 0 || strcasecmp(modeStr, "player") == 0) m_mode = DisplayMode::MUSIC_PLAYER;
    else if (strcasecmp(modeStr, "qr") == 0 || strcasecmp(modeStr, "qrcode") == 0) m_mode = DisplayMode::QR_CODE;
    else if (strcasecmp(modeStr, "clock") == 0) m_mode = DisplayMode::CLOCK;
    else if (strcasecmp(modeStr, "startup") == 0) m_mode = DisplayMode::STARTUP;
}

const char* DeskDisplay::getModeString() const {
    switch (m_mode) {
        case DisplayMode::ROBOT_EYES: return "ROBOT_EYES";
        case DisplayMode::PC_STATS: return "PC_STATS";
        case DisplayMode::MUSIC_PLAYER: return "MUSIC_PLAYER";
        case DisplayMode::QR_CODE: return "QR_CODE";
        case DisplayMode::CLOCK: return "CLOCK";
        default: return "STARTUP";
    }
}

void DeskDisplay::nextMode() {
    switch (m_mode) {
        case DisplayMode::ROBOT_EYES: m_mode = DisplayMode::PC_STATS; break;
        case DisplayMode::PC_STATS: m_mode = DisplayMode::MUSIC_PLAYER; break;
        case DisplayMode::MUSIC_PLAYER: m_mode = DisplayMode::QR_CODE; break;
        case DisplayMode::QR_CODE: m_mode = DisplayMode::CLOCK; break;
        case DisplayMode::CLOCK: m_mode = DisplayMode::ROBOT_EYES; break;
        default: m_mode = DisplayMode::ROBOT_EYES; break;
    }
}

void DeskDisplay::setQRCode(const char* text) {
    if (text) {
        strncpy(globalQRCodeText, text, sizeof(globalQRCodeText) - 1);
        globalQRCodeText[sizeof(globalQRCodeText) - 1] = '\0';
    }
}

void DeskDisplay::setMedia(const char* title, const char* artist, bool isPlaying) {
    if (title) strncpy(globalMedia.title, title, sizeof(globalMedia.title) - 1);
    if (artist) strncpy(globalMedia.artist, artist, sizeof(globalMedia.artist) - 1);
    globalMedia.title[sizeof(globalMedia.title) - 1] = '\0';
    globalMedia.artist[sizeof(globalMedia.artist) - 1] = '\0';
    globalMedia.isPlaying = isPlaying;
    globalMedia.lastUpdateMs = millis();
}

uint8_t DeskDisplay::scanI2CBus() {
    Serial.println(F("----------------------------------------"));
    Serial.println(F("[I2C] Scanning I2C bus for devices..."));

    uint8_t count = 0;
    for (uint8_t address = 1; address < 127; ++address) {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            Serial.printf("[I2C] Found device at address 0x%02X", address);
            if (address == 0x3C || address == 0x3D) {
                Serial.print(F(" (Likely OLED display - SH1106 or SSD1306)"));
            }
            Serial.println();
            count++;
        }
    }
    Serial.printf("[I2C] Scan complete. Found %u device(s).\n", count);
    Serial.println(F("----------------------------------------"));
    return count;
}

void DeskDisplay::drawStartupScreen() {
    m_u8g2.clearBuffer();
    m_u8g2.drawRFrame(0, 0, 128, 64, 2);
    m_u8g2.drawHLine(0, 18, 128);

    m_u8g2.setFont(u8g2_font_7x14B_tr);
    const char* title = "ESP32 DeskOS";
    m_u8g2.drawStr((128 - m_u8g2.getStrWidth(title)) / 2, 14, title);

    const char* status = "SYSTEM ONLINE";
    m_u8g2.drawStr((128 - m_u8g2.getStrWidth(status)) / 2, 38, status);

    m_u8g2.setFont(u8g2_font_6x10_tr);
    const char* version = "Firmware v0.1";
    m_u8g2.drawStr((128 - m_u8g2.getStrWidth(version)) / 2, 54, version);
    m_u8g2.sendBuffer();
}

void DeskDisplay::drawRobotEyes() {
    m_u8g2.clearBuffer();
    robotEyes.draw(m_u8g2);
    m_u8g2.sendBuffer();
}

void DeskDisplay::drawPCStats(const TelemetryData& stats) {
    m_u8g2.clearBuffer();

    // Top Title Bar
    m_u8g2.setFont(u8g2_font_6x10_tr);
    m_u8g2.drawStr(2, 9, "PC TELEMETRY");
    m_u8g2.drawHLine(0, 11, 128);

    if (!stats.hasData) {
        m_u8g2.setFont(u8g2_font_6x10_tr);
        m_u8g2.drawStr(8, 28, "CONNECTING PC AGENT");
        m_u8g2.setFont(u8g2_font_5x8_tr);
        m_u8g2.drawStr(12, 42, "Run: .\\scripts\\agent.ps1");
        m_u8g2.drawStr(22, 54, "Auto-detecting...");
        m_u8g2.sendBuffer();
        return;
    }

    // Smooth gauge interpolation (lerp)
    m_smoothCpu += (stats.cpuPercent - m_smoothCpu) * 0.25f;
    m_smoothRam += (stats.ramPercent - m_smoothRam) * 0.25f;

    // CPU Bar (Y: 16 - 28)
    m_u8g2.setFont(u8g2_font_6x10_tr);
    m_u8g2.drawStr(2, 23, "CPU");
    char cpuBuf[8];
    snprintf(cpuBuf, sizeof(cpuBuf), "%d%%", (int)roundf(m_smoothCpu));
    m_u8g2.drawStr(102, 23, cpuBuf);

    m_u8g2.drawFrame(26, 16, 72, 8);
    int cpuFill = (int)(constrain(m_smoothCpu, 0.0f, 100.0f) * 68 / 100.0f);
    if (cpuFill > 0) m_u8g2.drawBox(28, 18, cpuFill, 4);

    // RAM Bar (Y: 32 - 44)
    m_u8g2.drawStr(2, 39, "RAM");
    char ramBuf[8];
    snprintf(ramBuf, sizeof(ramBuf), "%d%%", (int)roundf(m_smoothRam));
    m_u8g2.drawStr(102, 39, ramBuf);

    m_u8g2.drawFrame(26, 32, 72, 8);
    int ramFill = (int)(constrain(m_smoothRam, 0.0f, 100.0f) * 68 / 100.0f);
    if (ramFill > 0) m_u8g2.drawBox(28, 34, ramFill, 4);

    // Bottom Info Bar (Y: 48 - 64)
    m_u8g2.drawHLine(0, 48, 128);
    m_u8g2.setFont(u8g2_font_5x8_tr);

    char upBuf[40];
    snprintf(upBuf, sizeof(upBuf), "UP: %s", stats.uptimeStr);
    m_u8g2.drawStr(2, 59, upBuf);

    char netBuf[32];
    snprintf(netBuf, sizeof(netBuf), "NET: %.0fK/s", stats.netKbIn + stats.netKbOut);
    m_u8g2.drawStr(72, 59, netBuf);

    m_u8g2.sendBuffer();
}

void DeskDisplay::drawMusicPlayer(const MediaData& media) {
    m_u8g2.clearBuffer();

    // Top Header with Equalizer
    m_u8g2.setFont(u8g2_font_5x8_tr);
    if (media.isPlaying) {
        m_u8g2.drawStr(2, 8, "NOW PLAYING");
    } else {
        m_u8g2.drawStr(2, 8, "MUSIC PAUSED");
    }

    // Animated 5-bar equalizer on right side of header
    m_eqPhase++;
    for (int b = 0; b < 5; b++) {
        int h = media.isPlaying ? (2 + (int)((sin((m_eqPhase * 0.3) + b) + 1.0) * 3.0)) : 1;
        m_u8g2.drawVLine(114 + (b * 3), 9 - h, h);
    }
    m_u8g2.drawHLine(0, 11, 128);

    if (strlen(media.title) == 0) {
        m_u8g2.setFont(u8g2_font_6x10_tr);
        m_u8g2.drawStr(14, 30, "NO MUSIC PLAYING");
        m_u8g2.setFont(u8g2_font_5x8_tr);
        m_u8g2.drawStr(10, 46, "Play Spotify/YouTube/VLC");
        m_u8g2.sendBuffer();
        return;
    }

    // Vinyl spinning disc graphic on left (circle at x=14, y=36, radius=12)
    m_u8g2.drawCircle(14, 36, 12);
    m_u8g2.drawCircle(14, 36, 5);
    m_u8g2.drawDisc(14, 36, 2);
    if (media.isPlaying) {
        // Rotating needle tick
        float angle = (m_eqPhase * 0.15f);
        int8_t nx = (int8_t)(cosf(angle) * 8);
        int8_t ny = (int8_t)(sinf(angle) * 8);
        m_u8g2.drawLine(14, 36, 14 + nx, 36 + ny);
    }

    // Scrolling or centered Track Title (Y: 30)
    m_u8g2.setFont(u8g2_font_7x14B_tr);
    int16_t titleWidth = m_u8g2.getStrWidth(media.title);
    constexpr int16_t TEXT_X = 32;
    constexpr int16_t TEXT_MAX_W = 92;

    if (titleWidth > TEXT_MAX_W) {
        m_musicScrollX += 2;
        if (m_musicScrollX > titleWidth + 20) m_musicScrollX = -TEXT_MAX_W;
        m_u8g2.setClipWindow(TEXT_X, 14, 128, 38);
        m_u8g2.drawStr(TEXT_X - m_musicScrollX, 30, media.title);
        m_u8g2.setMaxClipWindow();
    } else {
        m_u8g2.drawStr(TEXT_X, 30, media.title);
    }

    // Artist Name (Y: 45)
    m_u8g2.setFont(u8g2_font_6x10_tr);
    m_u8g2.drawStr(TEXT_X, 45, media.artist);

    // Bottom decorative wave / progress bar
    m_u8g2.drawHLine(TEXT_X, 54, 90);
    m_u8g2.drawBox(TEXT_X, 53, (m_eqPhase % 90), 3);

    m_u8g2.sendBuffer();
}

void DeskDisplay::drawQRCode(const char* text) {
    m_u8g2.clearBuffer();

    if (!text || strlen(text) == 0) text = "https://github.com/SYN606/ESP8266-DeskOS";

    // Initialize QR Code Version 3 (29x29 modules, up to ~77 characters)
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, 0, text);

    // 29x29 at 2x scale = 58x58 pixels
    // Quiet zone: place at x=3, y=3
    constexpr int16_t QR_OFFSET_X = 3;
    constexpr int16_t QR_OFFSET_Y = 3;
    constexpr uint8_t SCALE = 2;

    // Draw solid white background box for perfect optical scan contrast
    m_u8g2.setDrawColor(1);
    m_u8g2.drawBox(QR_OFFSET_X - 1, QR_OFFSET_Y - 1, (qrcode.size * SCALE) + 2, (qrcode.size * SCALE) + 2);

    // Draw dark modules by clearing pixels
    m_u8g2.setDrawColor(0);
    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                m_u8g2.drawBox(QR_OFFSET_X + (x * SCALE), QR_OFFSET_Y + (y * SCALE), SCALE, SCALE);
            }
        }
    }
    m_u8g2.setDrawColor(1);

    // Side Banner (x=66 to 127)
    m_u8g2.setFont(u8g2_font_7x14B_tr);
    m_u8g2.drawStr(68, 18, "SCAN ME");
    m_u8g2.drawHLine(68, 22, 56);

    m_u8g2.setFont(u8g2_font_5x8_tr);
    m_u8g2.drawStr(68, 34, "ESP8266");
    m_u8g2.drawStr(68, 44, "DeskOS");
    m_u8g2.drawStr(68, 56, "GitHub Repo");

    m_u8g2.sendBuffer();
}

void DeskDisplay::drawClock(const char* timeStr, const char* dateStr, bool wifiOk, const char* ipStr) {
    m_u8g2.clearBuffer();

    m_u8g2.setFont(u8g2_font_5x8_tr);
    if (wifiOk) {
        m_u8g2.drawStr(2, 8, "WIFI: CONNECTED");
        if (ipStr) m_u8g2.drawStr(82, 8, ipStr);
    } else if (globalTelemetry.hasData && globalTelemetry.timeStr[0] != '\0') {
        m_u8g2.drawStr(2, 8, "SYNC: PC AGENT");
        m_u8g2.drawStr(78, 8, "USB LIVE");
    } else {
        m_u8g2.drawStr(2, 8, "AP: DeskOS-Setup");
        m_u8g2.drawStr(80, 8, "192.168.4.1");
    }
    m_u8g2.drawHLine(0, 10, 128);

    m_u8g2.setFont(u8g2_font_inb19_mn);
    int16_t xTime = (128 - m_u8g2.getStrWidth(timeStr)) / 2;
    m_u8g2.drawStr(xTime, 38, timeStr);

    m_u8g2.setFont(u8g2_font_6x10_tr);
    int16_t xDate = (128 - m_u8g2.getStrWidth(dateStr)) / 2;
    m_u8g2.drawStr(xDate, 56, dateStr);

    m_u8g2.sendBuffer();
}

void DeskDisplay::update() {
    if (!m_initialized) return;

    unsigned long now = millis();

    if (m_mode == DisplayMode::STARTUP) {
        if (now >= m_startupUntilMs) {
            m_mode = DisplayMode::ROBOT_EYES;
        } else {
            return;
        }
    }

    // Refresh rate control (60 FPS for eyes, 30 FPS for music & PC stats)
    uint16_t frameInterval = (m_mode == DisplayMode::ROBOT_EYES) ? 16 : 
                            ((m_mode == DisplayMode::PC_STATS || m_mode == DisplayMode::MUSIC_PLAYER) ? 33 : 150);
    if (now - m_lastDrawMs < frameInterval) {
        return;
    }
    m_lastDrawMs = now;

    switch (m_mode) {
        case DisplayMode::ROBOT_EYES:
            robotEyes.update();
            drawRobotEyes();
            break;
        case DisplayMode::PC_STATS:
            drawPCStats(globalTelemetry);
            break;
        case DisplayMode::MUSIC_PLAYER:
            drawMusicPlayer(globalMedia);
            break;
        case DisplayMode::QR_CODE:
            drawQRCode(globalQRCodeText);
            break;
        case DisplayMode::CLOCK: {
            char timeBuf[16];
            char dateBuf[32];
            if (globalTelemetry.hasData && globalTelemetry.timeStr[0] != '\0') {
                strncpy(timeBuf, globalTelemetry.timeStr, sizeof(timeBuf) - 1);
                timeBuf[sizeof(timeBuf) - 1] = '\0';
                strncpy(dateBuf, globalTelemetry.dateStr, sizeof(dateBuf) - 1);
                dateBuf[sizeof(dateBuf) - 1] = '\0';
            } else {
                deskNetwork.getTimeString(timeBuf, sizeof(timeBuf));
                deskNetwork.getDateString(dateBuf, sizeof(dateBuf));
            }
            drawClock(timeBuf, dateBuf, deskNetwork.isConnected(), deskNetwork.getIPString());
            break;
        }
        default:
            break;
    }
}
