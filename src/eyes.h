#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

enum class EyeMood {
    NEUTRAL,
    HAPPY,
    SKEPTICAL,
    ANGRY,
    SLEEPY
};

class RobotEyes {
public:
    RobotEyes();

    // Call regularly inside loop() to advance animations
    void update();

    // Render the eyes onto the U8g2 framebuffer
    void draw(U8G2& u8g2);

    // Set mood
    void setMood(EyeMood mood);
    void setMood(const char* moodStr);
    EyeMood getMood() const { return m_mood; }
    const char* getMoodString() const;

    // Trigger an immediate blink
    void triggerBlink();

    // Force glance direction (-1=left/up, 0=center, 1=right/down)
    void look(int8_t dirX, int8_t dirY);

private:
    EyeMood m_mood;

    // Baseline dimensions
    static constexpr int8_t DEFAULT_WIDTH = 34;
    static constexpr int8_t DEFAULT_HEIGHT = 44;
    static constexpr int8_t CORNER_RADIUS = 10;
    static constexpr int8_t EYE_SPACING = 18;

    // Spring physics & positions
    float m_leftHeight;
    float m_rightHeight;
    float m_offsetX;
    float m_offsetY;
    float m_targetOffsetX;
    float m_targetOffsetY;
    float m_velX;
    float m_velY;

    // Idle organic breathing micro-drift
    float m_breathOffset;

    // Blinking state machine
    bool m_isBlinking;
    uint8_t m_blinkStage; // 0=idle, 1=closing, 2=closed, 3=opening
    unsigned long m_lastBlinkMs;
    unsigned long m_nextBlinkIntervalMs;

    // Gaze wander state machine
    unsigned long m_lastGazeMs;
    unsigned long m_nextGazeIntervalMs;
    unsigned long m_lastFrameMs;

    void updateBlink(unsigned long now);
    void updateGaze(unsigned long now);
    void drawHighlights(U8G2& u8g2, int16_t xL, int16_t yL, int16_t xR, int16_t yR, float hL, float hR);
};

extern RobotEyes robotEyes;
