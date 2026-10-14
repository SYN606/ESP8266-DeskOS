#include "eyes.h"

RobotEyes robotEyes;

RobotEyes::RobotEyes()
    : m_mood(EyeMood::NEUTRAL),
      m_leftHeight(DEFAULT_HEIGHT),
      m_rightHeight(DEFAULT_HEIGHT),
      m_offsetX(0),
      m_offsetY(0),
      m_targetOffsetX(0),
      m_targetOffsetY(0),
      m_isBlinking(false),
      m_blinkStage(0),
      m_lastBlinkMs(0),
      m_nextBlinkIntervalMs(3000),
      m_lastGazeMs(0),
      m_nextGazeIntervalMs(2500),
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
    m_targetOffsetX = constrain(dirX * 8, -8, 8);
    m_targetOffsetY = constrain(dirY * 5, -5, 5);
}

void RobotEyes::updateBlink(unsigned long now) {
    if (!m_isBlinking) {
        if (now - m_lastBlinkMs >= m_nextBlinkIntervalMs) {
            m_isBlinking = true;
            m_blinkStage = 1;
            m_lastBlinkMs = now;
            m_nextBlinkIntervalMs = random(2500, 6000);
        }
        return;
    }

    // Blinking animation steps
    constexpr float BLINK_SPEED = 8.0f;

    switch (m_blinkStage) {
        case 1: // Closing
            m_leftHeight -= BLINK_SPEED;
            m_rightHeight -= BLINK_SPEED;
            if (m_leftHeight <= 4.0f) {
                m_leftHeight = 2.0f;
                m_rightHeight = 2.0f;
                m_blinkStage = 2;
            }
            break;
        case 2: // Closed momentary pause
            m_blinkStage = 3;
            break;
        case 3: // Opening
            m_leftHeight += BLINK_SPEED;
            m_rightHeight += BLINK_SPEED;
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
    // Saccadic glance shift
    if (now - m_lastGazeMs >= m_nextGazeIntervalMs) {
        m_lastGazeMs = now;
        m_nextGazeIntervalMs = random(1800, 4500);

        int r = random(0, 10);
        if (r < 4) {
            // Center
            m_targetOffsetX = 0;
            m_targetOffsetY = 0;
        } else if (r < 6) {
            // Look left
            m_targetOffsetX = -7;
            m_targetOffsetY = random(-3, 4);
        } else if (r < 8) {
            // Look right
            m_targetOffsetX = 7;
            m_targetOffsetY = random(-3, 4);
        } else {
            // Look slightly up or down
            m_targetOffsetX = random(-4, 5);
            m_targetOffsetY = (random(0, 2) == 0) ? -4 : 4;
        }
    }

    // Smooth easing toward target offset
    m_offsetX += (m_targetOffsetX - m_offsetX) * 0.25f;
    m_offsetY += (m_targetOffsetY - m_offsetY) * 0.25f;
}

void RobotEyes::update() {
    unsigned long now = millis();
    if (now - m_lastFrameMs < 25) { // Cap internal physics tick at ~40 FPS
        return;
    }
    m_lastFrameMs = now;

    updateBlink(now);
    updateGaze(now);
}

void RobotEyes::draw(U8G2& u8g2) {
    // Screen center reference
    constexpr int16_t BASE_X = 21;
    constexpr int16_t BASE_Y = 10;

    int16_t xL = BASE_X + (int16_t)m_offsetX;
    int16_t xR = BASE_X + DEFAULT_WIDTH + EYE_SPACING + (int16_t)m_offsetX;

    float currentLH = m_leftHeight;
    float currentRH = m_rightHeight;

    // Mood adjustments when not blinking
    if (!m_isBlinking) {
        if (m_mood == EyeMood::SKEPTICAL) {
            currentRH = DEFAULT_HEIGHT * 0.35f; // Squint right eye
        } else if (m_mood == EyeMood::SLEEPY) {
            currentLH = DEFAULT_HEIGHT * 0.40f;
            currentRH = DEFAULT_HEIGHT * 0.40f;
        }
    }

    int16_t yL = BASE_Y + (int16_t)m_offsetY + (int16_t)((DEFAULT_HEIGHT - currentLH) / 2);
    int16_t yR = BASE_Y + (int16_t)m_offsetY + (int16_t)((DEFAULT_HEIGHT - currentRH) / 2);

    int16_t radL = min((int16_t)CORNER_RADIUS, (int16_t)(currentLH / 2));
    int16_t radR = min((int16_t)CORNER_RADIUS, (int16_t)(currentRH / 2));

    // Render left & right eye bodies
    u8g2.drawRBox(xL, yL, DEFAULT_WIDTH, (int16_t)currentLH, radL);
    u8g2.drawRBox(xR, yR, DEFAULT_WIDTH, (int16_t)currentRH, radR);

    // Apply mood mask cutouts using XOR / erase mode
    if (!m_isBlinking) {
        if (m_mood == EyeMood::HAPPY) {
            // Cut off lower half with inverted curve for smiling happy eyes
            u8g2.setDrawColor(0);
            u8g2.drawBox(xL - 2, yL + currentLH - 16, DEFAULT_WIDTH + 4, 18);
            u8g2.drawBox(xR - 2, yR + currentRH - 16, DEFAULT_WIDTH + 4, 18);
            u8g2.setDrawColor(1);
            // Draw cute curved lower border
            u8g2.drawRBox(xL, yL, DEFAULT_WIDTH, (int16_t)(currentLH - 10), 10);
            u8g2.drawRBox(xR, yR, DEFAULT_WIDTH, (int16_t)(currentRH - 10), 10);
        } else if (m_mood == EyeMood::ANGRY) {
            // Slant inner top corners
            u8g2.setDrawColor(0);
            // Left eye top-right corner cut
            for (int i = 0; i < 14; ++i) {
                u8g2.drawHLine(xL + DEFAULT_WIDTH - 14 + i, yL, 14 - i);
            }
            // Right eye top-left corner cut
            for (int i = 0; i < 14; ++i) {
                u8g2.drawHLine(xR, yR, 14 - i);
            }
            u8g2.setDrawColor(1);
        }
    }
}
