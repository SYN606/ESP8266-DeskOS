#include "eyes.h"
#include <math.h>

RobotEyes robotEyes;

RobotEyes::RobotEyes()
    : m_mood(EyeMood::NEUTRAL),
      m_leftHeight(DEFAULT_HEIGHT),
      m_rightHeight(DEFAULT_HEIGHT),
      m_offsetX(0.0f),
      m_offsetY(0.0f),
      m_targetOffsetX(0.0f),
      m_targetOffsetY(0.0f),
      m_velX(0.0f),
      m_velY(0.0f),
      m_breathOffset(0.0f),
      m_isBlinking(false),
      m_blinkStage(0),
      m_lastBlinkMs(0),
      m_nextBlinkIntervalMs(3200),
      m_lastGazeMs(0),
      m_nextGazeIntervalMs(2200),
      m_lastFrameMs(0) {
}

void RobotEyes::setMood(EyeMood mood) {
    m_mood = mood;
}

void RobotEyes::setMood(const char* moodStr) {
    if (!moodStr) return;
    if (strcasecmp(moodStr, "happy") == 0) m_mood = EyeMood::HAPPY;
    else if (strcasecmp(moodStr, "skeptical") == 0) m_mood = EyeMood::SKEPTICAL;
    else if (strcasecmp(moodStr, "angry") == 0) m_mood = EyeMood::ANGRY;
    else if (strcasecmp(moodStr, "sleepy") == 0) m_mood = EyeMood::SLEEPY;
    else m_mood = EyeMood::NEUTRAL;
}

const char* RobotEyes::getMoodString() const {
    switch (m_mood) {
        case EyeMood::HAPPY: return "HAPPY";
        case EyeMood::SKEPTICAL: return "SKEPTICAL";
        case EyeMood::ANGRY: return "ANGRY";
        case EyeMood::SLEEPY: return "SLEEPY";
        default: return "NEUTRAL";
    }
}

void RobotEyes::triggerBlink() {
    m_isBlinking = true;
    m_blinkStage = 1;
}

void RobotEyes::look(int8_t dirX, int8_t dirY) {
    m_targetOffsetX = constrain(dirX * 9, -9, 9);
    m_targetOffsetY = constrain(dirY * 6, -6, 6);
}

void RobotEyes::updateBlink(unsigned long now) {
    if (!m_isBlinking) {
        if (now - m_lastBlinkMs >= m_nextBlinkIntervalMs) {
            m_isBlinking = true;
            m_blinkStage = 1;
            m_lastBlinkMs = now;
            m_nextBlinkIntervalMs = random(2200, 5000);
        }
        return;
    }

    // High frame rate blink velocity
    constexpr float BLINK_CLOSE_RATE = 11.0f;
    constexpr float BLINK_OPEN_RATE  = 9.5f;

    switch (m_blinkStage) {
        case 1: // Rapid snap close
            m_leftHeight -= BLINK_CLOSE_RATE;
            m_rightHeight -= BLINK_CLOSE_RATE;
            if (m_leftHeight <= 3.0f) {
                m_leftHeight = 2.0f;
                m_rightHeight = 2.0f;
                m_blinkStage = 2;
            }
            break;
        case 2: // Quick bottom pause
            m_blinkStage = 3;
            break;
        case 3: // Spring pop open
            m_leftHeight += BLINK_OPEN_RATE;
            m_rightHeight += BLINK_OPEN_RATE;
            if (m_leftHeight >= DEFAULT_HEIGHT) {
                m_leftHeight = DEFAULT_HEIGHT;
                m_rightHeight = DEFAULT_HEIGHT;
                m_isBlinking = false;
                m_blinkStage = 0;
            }
            break;
    }
}

void RobotEyes::updateGaze(unsigned long now) {
    // Saccadic gaze shift timer
    if (now - m_lastGazeMs >= m_nextGazeIntervalMs) {
        m_lastGazeMs = now;
        m_nextGazeIntervalMs = random(1600, 4200);

        int r = random(0, 10);
        if (r < 4) {
            // Center gaze
            m_targetOffsetX = 0.0f;
            m_targetOffsetY = 0.0f;
        } else if (r < 6) {
            // Quick glance left
            m_targetOffsetX = -8.5f;
            m_targetOffsetY = random(-2, 3);
        } else if (r < 8) {
            // Quick glance right
            m_targetOffsetX = 8.5f;
            m_targetOffsetY = random(-2, 3);
        } else {
            // Look slightly up or down
            m_targetOffsetX = random(-3, 4);
            m_targetOffsetY = (random(0, 2) == 0) ? -5.0f : 4.0f;
        }
    }

    // True damped spring physics (Hooke's Law: F = -k*x - c*v)
    constexpr float STIFFNESS = 0.28f;
    constexpr float DAMPING   = 0.62f;

    float forceX = (m_targetOffsetX - m_offsetX) * STIFFNESS;
    m_velX = (m_velX + forceX) * DAMPING;
    m_offsetX += m_velX;

    float forceY = (m_targetOffsetY - m_offsetY) * STIFFNESS;
    m_velY = (m_velY + forceY) * DAMPING;
    m_offsetY += m_velY;

    // Organic breathing micro-drift
    m_breathOffset = sinf(now * 0.0022f) * 1.5f;
}

void RobotEyes::update() {
    unsigned long now = millis();
    // 60 FPS tick rate (~16ms delta)
    if (now - m_lastFrameMs < 16) {
        return;
    }
    m_lastFrameMs = now;

    updateBlink(now);
    updateGaze(now);
}

void RobotEyes::drawHighlights(U8G2& u8g2, int16_t xL, int16_t yL, int16_t xR, int16_t yR, float hL, float hR) {
    if (hL < 18.0f || hR < 18.0f) return; // Don't draw specular glare while blinking

    // Invert pixels for circular specular glare spots in upper corner
    u8g2.setDrawColor(0);

    // Primary highlight glint (3x3 rounded)
    int16_t glintXL = xL + 7;
    int16_t glintYL = yL + 7;
    int16_t glintXR = xR + 7;
    int16_t glintYR = yR + 7;

    u8g2.drawDisc(glintXL, glintYL, 2);
    u8g2.drawDisc(glintXR, glintYR, 2);

    // Secondary tiny micro-glint for depth
    u8g2.drawPixel(glintXL + 4, glintYL + 4);
    u8g2.drawPixel(glintXR + 4, glintYR + 4);

    u8g2.setDrawColor(1);
}

void RobotEyes::draw(U8G2& u8g2) {
    constexpr int16_t BASE_X = 21;
    constexpr int16_t BASE_Y = 10;

    int16_t xL = BASE_X + (int16_t)m_offsetX;
    int16_t xR = BASE_X + DEFAULT_WIDTH + EYE_SPACING + (int16_t)m_offsetX;

    float currentLH = m_leftHeight;
    float currentRH = m_rightHeight;

    // Eye squishing / asymmetric facial distortion when glancing
    if (!m_isBlinking) {
        if (m_offsetX < -4.0f) {
            currentLH = DEFAULT_HEIGHT * 0.90f; // Leading eye compresses slightly
        } else if (m_offsetX > 4.0f) {
            currentRH = DEFAULT_HEIGHT * 0.90f;
        }

        if (m_mood == EyeMood::SKEPTICAL) {
            currentRH = DEFAULT_HEIGHT * 0.35f; // Squint right eye
        } else if (m_mood == EyeMood::SLEEPY) {
            currentLH = DEFAULT_HEIGHT * 0.42f;
            currentRH = DEFAULT_HEIGHT * 0.42f;
        }
    }

    int16_t yL = BASE_Y + (int16_t)(m_offsetY + m_breathOffset) + (int16_t)((DEFAULT_HEIGHT - currentLH) / 2);
    int16_t yR = BASE_Y + (int16_t)(m_offsetY + m_breathOffset) + (int16_t)((DEFAULT_HEIGHT - currentRH) / 2);

    int16_t radL = min((int16_t)CORNER_RADIUS, (int16_t)(currentLH / 2));
    int16_t radR = min((int16_t)CORNER_RADIUS, (int16_t)(currentRH / 2));

    // Render left & right eye bodies
    u8g2.drawRBox(xL, yL, DEFAULT_WIDTH, (int16_t)currentLH, radL);
    u8g2.drawRBox(xR, yR, DEFAULT_WIDTH, (int16_t)currentRH, radR);

    // Apply mood mask cutouts
    if (!m_isBlinking) {
        if (m_mood == EyeMood::HAPPY) {
            // Cut off lower half for cheerful upward crescent
            u8g2.setDrawColor(0);
            u8g2.drawBox(xL - 2, yL + currentLH - 16, DEFAULT_WIDTH + 4, 18);
            u8g2.drawBox(xR - 2, yR + currentRH - 16, DEFAULT_WIDTH + 4, 18);
            u8g2.setDrawColor(1);
            u8g2.drawRBox(xL, yL, DEFAULT_WIDTH, (int16_t)(currentLH - 10), 10);
            u8g2.drawRBox(xR, yR, DEFAULT_WIDTH, (int16_t)(currentRH - 10), 10);
        } else if (m_mood == EyeMood::ANGRY) {
            // Slant inner top corners
            u8g2.setDrawColor(0);
            for (int i = 0; i < 14; ++i) {
                u8g2.drawHLine(xL + DEFAULT_WIDTH - 14 + i, yL, 14 - i);
                u8g2.drawHLine(xR, yR, 14 - i);
            }
            u8g2.setDrawColor(1);
        } else {
            // Render 3D glassy specular highlights in neutral, sleepy, and skeptical modes
            drawHighlights(u8g2, xL, yL, xR, yR, currentLH, currentRH);
        }
    }
}
