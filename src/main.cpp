#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"
#include "pins.h"
#include "display.h"
#include "eyes.h"
#include "network.h"
#include "telemetry.h"

static unsigned long lastHeartbeatMs = 0;
constexpr unsigned long HEARTBEAT_INTERVAL_MS = 10000;

// Buffer for serial line input
static char serialBuffer[256];
static uint16_t serialIndex = 0;

void processSerialLine(const char* line) {
    if (!line || strlen(line) == 0) return;

    // Check if JSON command
    if (line[0] == '{') {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, line);
        if (err) {
            Serial.printf("[SERIAL] JSON parse error: %s\n", err.c_str());
            return;
        }

        const char* cmd = doc["cmd"];
        if (!cmd) return;

        if (strcmp(cmd, "stats") == 0) {
            globalTelemetry.cpuPercent = doc["cpu"] | 0.0f;
            globalTelemetry.ramPercent = doc["ram"] | 0.0f;
            const char* up = doc["uptime"];
            if (up) strncpy(globalTelemetry.uptimeStr, up, sizeof(globalTelemetry.uptimeStr) - 1);
            const char* tStr = doc["time"];
            if (tStr) strncpy(globalTelemetry.timeStr, tStr, sizeof(globalTelemetry.timeStr) - 1);
            const char* dStr = doc["date"];
            if (dStr) strncpy(globalTelemetry.dateStr, dStr, sizeof(globalTelemetry.dateStr) - 1);
            globalTelemetry.netKbIn = doc["net_in"] | 0.0f;
            globalTelemetry.netKbOut = doc["net_out"] | 0.0f;
            globalTelemetry.hasData = true;
            globalTelemetry.lastReceivedMs = millis();
            // Switch display to PC stats screen only if explicitly requested during startup
            if (doc["auto_show"] | false) {
                if (deskDisplay.getMode() == DisplayMode::STARTUP) {
                    deskDisplay.setMode(DisplayMode::PC_STATS);
                }
            }
            Serial.println(F("{\"ack\":\"stats_updated\"}"));
        } else if (strcmp(cmd, "media") == 0) {
            const char* title = doc["title"];
            const char* artist = doc["artist"];
            bool isPlaying = doc["playing"] | false;
            deskDisplay.setMedia(title, artist, isPlaying);
            if (doc["auto_show"] | false) {
                deskDisplay.setMode(DisplayMode::MUSIC_PLAYER);
            }
            Serial.println(F("{\"ack\":\"media_updated\"}"));
        } else if (strcmp(cmd, "qr") == 0) {
            const char* text = doc["text"];
            if (text) deskDisplay.setQRCode(text);
            deskDisplay.setMode(DisplayMode::QR_CODE);
            Serial.println(F("{\"ack\":\"qr_set\"}"));
        } else if (strcmp(cmd, "mode") == 0) {
            const char* m = doc["val"];
            if (m) deskDisplay.setMode(m);
            Serial.printf("{\"ack\":\"mode_set\",\"mode\":\"%s\"}\n", deskDisplay.getModeString());
        } else if (strcmp(cmd, "mood") == 0) {
            const char* m = doc["val"];
            if (m) robotEyes.setMood(m);
            Serial.printf("{\"ack\":\"mood_set\",\"mood\":\"%s\"}\n", robotEyes.getMoodString());
        } else if (strcmp(cmd, "blink") == 0) {
            robotEyes.triggerBlink();
            Serial.println(F("{\"ack\":\"blink\"}"));
        } else if (strcmp(cmd, "brightness") == 0) {
            uint8_t b = doc["val"] | 255;
            deskDisplay.setBrightness(b);
            Serial.printf("{\"ack\":\"brightness_set\",\"val\":%d}\n", b);
        }
        return;
    }

    // Keyboard shortcuts for serial monitor interaction
    char c = line[0];
    switch (c) {
        case 'e':
        case 'E':
            deskDisplay.setMode(DisplayMode::ROBOT_EYES);
            Serial.println(F("[COMMAND] Switched to ROBOT EYES"));
            break;
        case 's':
        case 'S':
            deskDisplay.setMode(DisplayMode::PC_STATS);
            Serial.println(F("[COMMAND] Switched to PC STATS"));
            break;
        case 'p':
        case 'P':
            deskDisplay.setMode(DisplayMode::MUSIC_PLAYER);
            Serial.println(F("[COMMAND] Switched to MUSIC PLAYER"));
            break;
        case 'q':
        case 'Q':
            deskDisplay.setMode(DisplayMode::QR_CODE);
            Serial.println(F("[COMMAND] Switched to QR CODE"));
            break;
        case 'c':
        case 'C':
            deskDisplay.setMode(DisplayMode::CLOCK);
            Serial.println(F("[COMMAND] Switched to CLOCK"));
            break;
        case 'b':
        case 'B':
            robotEyes.triggerBlink();
            Serial.println(F("[COMMAND] Blink triggered"));
            break;
        case 'h':
        case 'H':
            robotEyes.setMood(EyeMood::HAPPY);
            Serial.println(F("[COMMAND] Mood: HAPPY"));
            break;
        case 'n':
        case 'N':
            robotEyes.setMood(EyeMood::NEUTRAL);
            Serial.println(F("[COMMAND] Mood: NEUTRAL"));
            break;
        case 'a':
        case 'A':
            robotEyes.setMood(EyeMood::ANGRY);
            Serial.println(F("[COMMAND] Mood: ANGRY"));
            break;
        case 'k':
        case 'K':
            robotEyes.setMood(EyeMood::SKEPTICAL);
            Serial.println(F("[COMMAND] Mood: SKEPTICAL"));
            break;
        case 'z':
        case 'Z':
            robotEyes.setMood(EyeMood::SLEEPY);
            Serial.println(F("[COMMAND] Mood: SLEEPY"));
            break;
        case 'l':
        case 'L':
            robotEyes.setMood(EyeMood::LOVE);
            Serial.println(F("[COMMAND] Mood: LOVE (Heart Eyes)"));
            break;
        case 'w':
        case 'W':
            robotEyes.setMood(EyeMood::WINK);
            Serial.println(F("[COMMAND] Mood: WINK"));
            break;
        case 't':
        case 'T':
            robotEyes.setMood(EyeMood::CAT);
            Serial.println(F("[COMMAND] Mood: CAT (Ears, Slit Pupils, Whiskers)"));
            break;
        case 'o':
        case 'O':
            robotEyes.setMood(EyeMood::OWL);
            Serial.println(F("[COMMAND] Mood: OWL (Round Eyes, Horns, Beak)"));
            break;
        case 'm':
        case 'M':
            robotEyes.nextMood();
            Serial.printf("[COMMAND] Next Mood: %s\n", robotEyes.getMoodString());
            break;
        case ' ':
            deskDisplay.nextMode();
            Serial.printf("[COMMAND] Next Mode: %s\n", deskDisplay.getModeString());
            break;
        default:
            Serial.printf("[COMMAND] Unknown key: '%c'. Keys: h=happy, k=skeptical, a=angry, z=sleepy, l=love, w=wink, t=cat, o=owl, m=next mood, space=next screen\n", c);
            break;
    }
}

void handleSerialInput() {
    while (Serial.available()) {
        char ch = (char)Serial.read();
        if (ch == '\r' || ch == '\n') {
            if (serialIndex > 0) {
                serialBuffer[serialIndex] = '\0';
                processSerialLine(serialBuffer);
                serialIndex = 0;
            }
        } else if (serialIndex < sizeof(serialBuffer) - 1) {
            serialBuffer[serialIndex++] = ch;
        }
    }
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(400);

    Serial.println();
    Serial.println(F("========================================"));
    Serial.println(F("           ESP32 DeskOS v0.1            "));
    Serial.println(F("  Desktop Companion & Monitor Firmware  "));
    Serial.println(F("========================================"));

#if defined(ESP32)
    Serial.printf("[SYSTEM] Chip Model: %s (Rev %d)\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("[SYSTEM] CPU Frequency: %u MHz\n", ESP.getCpuFreqMHz());
#elif defined(ESP8266)
    Serial.printf("[SYSTEM] Chip: ESP8266EX (Chip ID: 0x%06X)\n", ESP.getChipId());
    Serial.printf("[SYSTEM] CPU Frequency: %u MHz\n", ESP.getCpuFreqMHz());
#endif
    Serial.printf("[SYSTEM] Free Heap: %u bytes\n", ESP.getFreeHeap());

    // 1. Initialize Display
    Serial.println(F("[SYSTEM] Initializing display subsystem..."));
    bool displayReady = deskDisplay.begin();
    deskDisplay.scanI2CBus();

    if (displayReady) {
        deskDisplay.drawStartupScreen();
    }

    // 2. Initialize Wi-Fi and Web Server
    Serial.println(F("[SYSTEM] Initializing network subsystem..."));
    deskNetwork.begin();

    Serial.println(F("========================================"));
    Serial.println(F("[SYSTEM] DeskOS Ready!"));
    Serial.println(F("[SYSTEM] Press: 'e'=eyes, 's'=stats, 'p'=music, 'q'=qr, 'c'=clock, 'b'=blink, space=next"));
    Serial.println(F("========================================"));
}

void loop() {
    // 1. Process network clients (dashboard HTTP requests)
    deskNetwork.update();

    // 2. Process serial commands from PC Agent or keyboard
    handleSerialInput();

    // 3. Update OLED animations / current mode
    deskDisplay.update();

    // 4. Periodic heartbeat
    unsigned long now = millis();
    if (now - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
        lastHeartbeatMs = now;
        Serial.printf("[HEARTBEAT] Mode: %s | Uptime: %lu s | Free Heap: %u bytes | WiFi: %s\n",
                      deskDisplay.getModeString(),
                      now / 1000,
                      ESP.getFreeHeap(),
                      deskNetwork.isConnected() ? deskNetwork.getSSID() : "AP-Mode");
    }

    yield();
}
