#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

enum class EyeMood {
    NEUTRAL,
    HAPPY,
    SKEPTICAL,
    ANGRY,
    SLEEPY,
    SURPRISED
};

class RobotEyes {
public:
    RobotEyes();

    // Call regularly inside loop() to advance animations
    void update();

    // Render the eyes and articulated eyebrows onto the U8g2 framebuffer
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
    static constexpr int8_t DEFAULT_HEIGHT = 40;
    static constexpr int8_t CORNER_RADIUS = 10;
    static constexpr int8_t EYE_SPACING = 18;

    // Dynamic interpolated dimensions & squish
    float m_leftHeight;
    float m_rightHeight;
    float m_leftWidth;
    float m_rightWidth;
    float m_targetLeftHeight;
    float m_targetRightHeight;
    float m_targetLeftWidth;
    float m_targetRightWidth;

    // Articulated animated eyebrows (inner & outer Y endpoints)
    float m_leftBrowInnerY;
    float m_leftBrowOuterY;
    float m_rightBrowInnerY;
    float m_rightBrowOuterY;
    float m_targetLeftBrowInnerY;
    float m_targetLeftBrowOuterY;
    float m_targetRightBrowInnerY;
    float m_targetRightBrowOuterY;
    float m_browBounce;

    // Saccadic gaze spring physics & positions
    float m_offsetX;
    float m_offsetY;
    float m_targetOffsetX;
    float m_targetOffsetY;
    float m_velX;
    float m_velY;

    // Ocular micro-tremor & breathing
    float m_tremorX;
    float m_tremorY;
    float m_breathOffset;
    float m_breathScale;

    // Realistic blinking state machine with double-blink support
    bool m_isBlinking;
    uint8_t m_blinkStage; // 0=idle, 1=closing, 2=closed, 3=opening
    unsigned long m_blinkStartMs;
    float m_blinkProgress;
    bool m_doubleBlinkPending;
    unsigned long m_doubleBlinkTriggerMs;
    unsigned long m_lastBlinkMs;
    unsigned long m_nextBlinkIntervalMs;

    // Gaze wander state machine
    unsigned long m_lastGazeMs;
    unsigned long m_nextGazeIntervalMs;
    unsigned long m_lastFrameMs;

    void updateBlink(unsigned long now);
    void updateGaze(unsigned long now);
    void updateMoodDynamics(unsigned long now);
    void drawCorneaGlint(U8G2& u8g2, int16_t xCenter, int16_t yCenter, float curW, float curH, bool isLeft);
    void drawEyebrow(U8G2& u8g2, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t thickness);
};

extern RobotEyes robotEyes;
