#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

enum class EyeMood {
    NEUTRAL,
    HAPPY,
    SKEPTICAL,
    ANGRY,
    SLEEPY,
    LOVE,
    WINK,
    CAT,
    OWL
};

class RobotEyes {
public:
    RobotEyes();

    // Call regularly inside loop() to advance animations
    void update();

    // Render the eyes and articulated curved eyebrows onto the U8g2 framebuffer
    void draw(U8G2& u8g2);

    // Set mood
    void setMood(EyeMood mood);
    void setMood(const char* moodStr);
    EyeMood getMood() const { return m_mood; }
    const char* getMoodString() const;
    void nextMood();

    // Trigger an immediate blink
    void triggerBlink();

private:
    EyeMood m_mood;

    // Baseline dimensions
    static constexpr int8_t DEFAULT_WIDTH = 34;
    static constexpr int8_t DEFAULT_HEIGHT = 40;
    static constexpr int8_t CORNER_RADIUS = 10;
    static constexpr int8_t EYE_SPACING = 18;

    // Dynamic interpolated eye dimensions
    float m_leftHeight;
    float m_rightHeight;
    float m_leftWidth;
    float m_rightWidth;
    float m_targetLeftHeight;
    float m_targetRightHeight;
    float m_targetLeftWidth;
    float m_targetRightWidth;

    // Expressive pupil dynamics (size & gaze offset)
    float m_pupilWidth;
    float m_pupilHeight;
    float m_targetPupilWidth;
    float m_targetPupilHeight;

    float m_pupilOffsetX;
    float m_pupilOffsetY;
    float m_targetPupilOffsetX;
    float m_targetPupilOffsetY;
    float m_pupilVelX;
    float m_pupilVelY;

    // 3-Point Articulated Curved Eyebrows (Inner, Arch Peak, Outer Tail)
    float m_leftBrowInnerY;
    float m_leftBrowMidY;
    float m_leftBrowOuterY;
    float m_rightBrowInnerY;
    float m_rightBrowMidY;
    float m_rightBrowOuterY;

    float m_targetLeftBrowInnerY;
    float m_targetLeftBrowMidY;
    float m_targetLeftBrowOuterY;
    float m_targetRightBrowInnerY;
    float m_targetRightBrowMidY;
    float m_targetRightBrowOuterY;

    // Eyebrow physics
    float m_browInertiaY;
    float m_browBounce;

    // Saccadic gaze overall head/eye wander spring physics
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

    // Special animation phases & secondary dynamics
    float m_squashX;
    float m_targetSquashX;
    float m_pupilDilation;
    float m_targetPupilDilation;
    float m_browCuriosityTwitch;
    unsigned long m_nextTwitchMs;
    float m_heartPulsePhase;

    void updateBlink(unsigned long now);
    void updateGaze(unsigned long now);
    void updateMoodDynamics(unsigned long now);

    void drawSingleEye(U8G2& u8g2, int16_t x, int16_t y, float w, float h, float pW, float pH, float pOffX, float pOffY, float blinkProgress, bool isLeft);
    void drawCurvedEyebrow(U8G2& u8g2, int16_t xIn, int16_t yIn, int16_t xMid, int16_t yMid, int16_t xOut, int16_t yOut, bool isLeft);
    void drawHeartEye(U8G2& u8g2, int16_t cx, int16_t cy, float scale);
    void drawWinkEye(U8G2& u8g2, int16_t cx, int16_t cy, float w);
    void drawCatEyes(U8G2& u8g2, int16_t xL, int16_t yL, int16_t xR, int16_t yR, float pOffX, float pOffY, float blinkProgress);
    void drawOwlEyes(U8G2& u8g2, float pOffX, float pOffY, float blinkProgress);
};

extern RobotEyes robotEyes;
