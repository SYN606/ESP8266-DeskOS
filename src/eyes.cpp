#include "eyes.h"
#include <math.h>

RobotEyes robotEyes;

RobotEyes::RobotEyes()
    : m_mood(EyeMood::NEUTRAL),
      m_leftHeight(DEFAULT_HEIGHT),
      m_rightHeight(DEFAULT_HEIGHT),
      m_leftWidth(DEFAULT_WIDTH),
      m_rightWidth(DEFAULT_WIDTH),
      m_targetLeftHeight(DEFAULT_HEIGHT),
      m_targetRightHeight(DEFAULT_HEIGHT),
      m_targetLeftWidth(DEFAULT_WIDTH),
      m_targetRightWidth(DEFAULT_WIDTH),
      m_offsetX(0.0f),
      m_offsetY(0.0f),
      m_targetOffsetX(0.0f),
      m_targetOffsetY(0.0f),
      m_velX(0.0f),
      m_velY(0.0f),
      m_tremorX(0.0f),
      m_tremorY(0.0f),
      m_breathOffset(0.0f),
      m_breathScale(1.0f),
      m_isBlinking(false),
      m_blinkStage(0),
      m_blinkStartMs(0),
      m_blinkProgress(1.0f),
      m_doubleBlinkPending(false),
      m_doubleBlinkTriggerMs(0),
      m_lastBlinkMs(0),
      m_nextBlinkIntervalMs(3200),
      m_lastGazeMs(0),
      m_nextGazeIntervalMs(2400),
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
    else if (strcasecmp(moodStr, "surprised") == 0 || strcasecmp(moodStr, "curious") == 0) m_mood = EyeMood::SURPRISED;
    else m_mood = EyeMood::NEUTRAL;
}

const char* RobotEyes::getMoodString() const {
    switch (m_mood) {
        case EyeMood::HAPPY: return "HAPPY";
        case EyeMood::SKEPTICAL: return "SKEPTICAL";
        case EyeMood::ANGRY: return "ANGRY";
        case EyeMood::SLEEPY: return "SLEEPY";
        case EyeMood::SURPRISED: return "SURPRISED";
        default: return "NEUTRAL";
    }
}

void RobotEyes::triggerBlink() {
    m_isBlinking = true;
    m_blinkStage = 1;
    m_blinkStartMs = millis();
}

void RobotEyes::look(int8_t dirX, int8_t dirY) {
    m_targetOffsetX = constrain(dirX * 10, -11, 11);
    m_targetOffsetY = constrain(dirY * 7, -8, 8);
}

void RobotEyes::updateBlink(unsigned long now) {
    // 1. Spontaneous Blink Trigger
    if (!m_isBlinking) {
        bool shouldBlink = false;

        // Check for double blink follow-up
        if (m_doubleBlinkPending && now >= m_doubleBlinkTriggerMs) {
            shouldBlink = true;
            m_doubleBlinkPending = false;
        } else if (now - m_lastBlinkMs >= m_nextBlinkIntervalMs) {
            shouldBlink = true;
            // 25% chance of a realistic quick double-blink
            if (random(0, 100) < 25 && m_mood != EyeMood::SLEEPY) {
                m_doubleBlinkPending = true;
                m_doubleBlinkTriggerMs = now + 340;
            }
            m_lastBlinkMs = now;
            if (m_mood == EyeMood::SLEEPY) {
                m_nextBlinkIntervalMs = random(4500, 7500);
            } else if (m_mood == EyeMood::SURPRISED) {
                m_nextBlinkIntervalMs = random(1800, 3600);
            } else {
                m_nextBlinkIntervalMs = random(2600, 5200);
            }
        }

        if (shouldBlink) {
            m_isBlinking = true;
            m_blinkStage = 1; // Closing
            m_blinkStartMs = now;
        }
        return;
    }

    // 2. High-performance non-linear asymmetric blink curve
    unsigned long elapsed = now - m_blinkStartMs;

    // Timing parameters: Closing is fast (~65ms), pause is tiny (~20ms), Opening has gentle elastic ease (~140ms)
    constexpr unsigned long CLOSE_DURATION_MS = 65;
    constexpr unsigned long CLOSED_PAUSE_MS   = 20;
    constexpr unsigned long OPEN_DURATION_MS  = 140;

    if (m_blinkStage == 1) { // Closing
        if (elapsed < CLOSE_DURATION_MS) {
            float t = (float)elapsed / CLOSE_DURATION_MS;
            // Ease-in acceleration (t^2)
            m_blinkProgress = 1.0f - (t * t);
        } else {
            m_blinkProgress = 0.05f; // Closed slit
            m_blinkStage = 2; // Bottom pause
            m_blinkStartMs = now;
        }
    } else if (m_blinkStage == 2) { // Closed slit pause
        if (elapsed >= CLOSED_PAUSE_MS) {
            m_blinkStage = 3; // Opening
            m_blinkStartMs = now;
        }
    } else if (m_blinkStage == 3) { // Opening
        if (elapsed < OPEN_DURATION_MS) {
            float t = (float)elapsed / OPEN_DURATION_MS;
            // Smooth cubic ease-out curve: 1 - (1-t)^3
            float easeOut = 1.0f - powf(1.0f - t, 3.0f);
            m_blinkProgress = 0.05f + (easeOut * 0.95f);
        } else {
            m_blinkProgress = 1.0f;
            m_isBlinking = false;
            m_blinkStage = 0;
        }
    }
}

void RobotEyes::updateGaze(unsigned long now) {
    // 1. Natural Saccadic Glance Timing
    if (now - m_lastGazeMs >= m_nextGazeIntervalMs) {
        m_lastGazeMs = now;
        m_nextGazeIntervalMs = random(1500, 4000);

        int r = random(0, 100);
        if (r < 42) {
            // Forward center gaze
            m_targetOffsetX = 0.0f;
            m_targetOffsetY = 0.0f;
        } else if (r < 62) {
            // Glance left
            m_targetOffsetX = (float)random(-11, -7);
            m_targetOffsetY = (float)random(-3, 4);
        } else if (r < 82) {
            // Glance right
            m_targetOffsetX = (float)random(7, 12);
            m_targetOffsetY = (float)random(-3, 4);
        } else if (r < 92) {
            // Curious upward glance
            m_targetOffsetX = (float)random(-4, 5);
            m_targetOffsetY = (float)random(-7, -4);
        } else {
            // Downward glance
            m_targetOffsetX = (float)random(-4, 5);
            m_targetOffsetY = (float)random(4, 7);
        }
    }

    // 2. Critically Damped Spring Physics for Saccades
    constexpr float STIFFNESS = 0.32f;
    constexpr float DAMPING   = 0.58f;

    float forceX = (m_targetOffsetX - m_offsetX) * STIFFNESS;
    m_velX = (m_velX + forceX) * DAMPING;
    m_offsetX += m_velX;

    float forceY = (m_targetOffsetY - m_offsetY) * STIFFNESS;
    m_velY = (m_velY + forceY) * DAMPING;
    m_offsetY += m_velY;

    // 3. Physiological Ocular Micro-Tremor (Sub-pixel biological life)
    m_tremorX = (sinf(now * 0.006f) * 0.45f) + (sinf(now * 0.016f) * 0.25f);
    m_tremorY = (cosf(now * 0.005f) * 0.35f);

    // 4. Organic Respiration Rhythm
    float breathPhase = now * 0.0022f; // ~0.35 Hz natural breathing
    m_breathOffset = sinf(breathPhase) * 1.5f;
    m_breathScale  = 1.0f + (sinf(breathPhase) * 0.025f);
}

void RobotEyes::updateMoodDynamics(unsigned long now) {
    // Determine baseline targets based on active mood
    switch (m_mood) {
        case EyeMood::HAPPY:
            m_targetLeftWidth   = 36.0f;
            m_targetRightWidth  = 36.0f;
            m_targetLeftHeight  = 38.0f;
            m_targetRightHeight = 38.0f;
            break;

        case EyeMood::SKEPTICAL:
            // Left eye arches inquisitively high, right eye squints
            m_targetLeftWidth   = 35.0f;
            m_targetRightWidth  = 33.0f;
            m_targetLeftHeight  = 46.0f;
            m_targetRightHeight = 22.0f;
            break;

        case EyeMood::ANGRY:
            m_targetLeftWidth   = 34.0f;
            m_targetRightWidth  = 34.0f;
            m_targetLeftHeight  = 36.0f;
            m_targetRightHeight = 36.0f;
            break;

        case EyeMood::SLEEPY:
            m_targetLeftWidth   = 34.0f;
            m_targetRightWidth  = 34.0f;
            m_targetLeftHeight  = 22.0f;
            m_targetRightHeight = 22.0f;
            break;

        case EyeMood::SURPRISED:
            // Large dilated round eyes
            m_targetLeftWidth   = 38.0f;
            m_targetRightWidth  = 38.0f;
            m_targetLeftHeight  = 49.0f;
            m_targetRightHeight = 49.0f;
            break;

        default: // NEUTRAL
            m_targetLeftWidth   = (float)DEFAULT_WIDTH;
            m_targetRightWidth  = (float)DEFAULT_WIDTH;
            m_targetLeftHeight  = (float)DEFAULT_HEIGHT;
            m_targetRightHeight = (float)DEFAULT_HEIGHT;
            break;
    }

    // Parallax squish when looking sideways (perspective distortion)
    if (m_offsetX < -4.0f) {
        m_targetLeftWidth  *= 0.92f;
        m_targetRightWidth *= 1.05f;
    } else if (m_offsetX > 4.0f) {
        m_targetRightWidth *= 0.92f;
        m_targetLeftWidth  *= 1.05f;
    }

    // Smooth organic interpolation (lerp) towards targets
    constexpr float LERP_SPEED = 0.16f;
    m_leftHeight  += (m_targetLeftHeight  - m_leftHeight)  * LERP_SPEED;
    m_rightHeight += (m_targetRightHeight - m_rightHeight) * LERP_SPEED;
    m_leftWidth   += (m_targetLeftWidth   - m_leftWidth)   * LERP_SPEED;
    m_rightWidth  += (m_targetRightWidth  - m_rightWidth)  * LERP_SPEED;
}

void RobotEyes::update() {
    unsigned long now = millis();
    // 60 FPS update tick rate (~16ms delta)
    if (now - m_lastFrameMs < 16) {
        return;
    }
    m_lastFrameMs = now;

    updateBlink(now);
    updateGaze(now);
    updateMoodDynamics(now);
}

void RobotEyes::drawCorneaGlint(U8G2& u8g2, int16_t xCenter, int16_t yCenter, float curW, float curH, bool isLeft) {
    if (curH < 18.0f) return; // Hide glint during closed/squinted eyelid state

    // 3D Spherical Cornea Parallax:
    // As the eye turns left, the highlight reflection on the glass surface shifts right
    float glintParallaxX = -m_offsetX * 0.38f;
    float glintParallaxY = -m_offsetY * 0.32f;

    int16_t glintBaseX = xCenter - (int16_t)(curW * 0.20f) + (int16_t)glintParallaxX;
    int16_t glintBaseY = yCenter - (int16_t)(curH * 0.22f) + (int16_t)glintParallaxY;

    // Carve out specular highlight using dark draw color
    u8g2.setDrawColor(0);

    // Primary bright glint (3px disc)
    u8g2.drawDisc(glintBaseX, glintBaseY, 2);

    // Secondary micro-reflection for glossy depth
    u8g2.drawPixel(glintBaseX + 4, glintBaseY + 4);
    u8g2.drawPixel(glintBaseX + 5, glintBaseY + 4);

    u8g2.setDrawColor(1);
}

void RobotEyes::draw(U8G2& u8g2) {
    constexpr int16_t BASE_X = 21;
    constexpr int16_t BASE_Y = 10;

    // Apply gaze offset + micro-tremor
    float totalOffsetX = m_offsetX + m_tremorX;
    float totalOffsetY = m_offsetY + m_tremorY + m_breathOffset;

    // Effective eyelid heights based on blink progress
    float effectiveLH = m_leftHeight  * m_blinkProgress * m_breathScale;
    float effectiveRH = m_rightHeight * m_blinkProgress * m_breathScale;
    if (effectiveLH < 2.5f) effectiveLH = 2.5f;
    if (effectiveRH < 2.5f) effectiveRH = 2.5f;

    // Asymmetric eyelid drop:
    // Upper lid drops 82% of the way down; lower lid rises 18% of the way up
    float dropL = (m_leftHeight  - effectiveLH) * 0.82f;
    float dropR = (m_rightHeight - effectiveRH) * 0.82f;

    // Positioning
    int16_t xL = BASE_X + (int16_t)totalOffsetX;
    int16_t xR = BASE_X + (int16_t)m_leftWidth + EYE_SPACING + (int16_t)totalOffsetX;

    // Skeptical mood raises left eye slightly
    float moodLiftL = (m_mood == EyeMood::SKEPTICAL) ? -3.0f : 0.0f;
    float moodLiftR = (m_mood == EyeMood::SKEPTICAL) ?  2.0f : 0.0f;

    int16_t yL = BASE_Y + (int16_t)(totalOffsetY + dropL + moodLiftL);
    int16_t yR = BASE_Y + (int16_t)(totalOffsetY + dropR + moodLiftR);

    int16_t radL = min((int16_t)CORNER_RADIUS, (int16_t)(effectiveLH / 2));
    int16_t radR = min((int16_t)CORNER_RADIUS, (int16_t)(effectiveRH / 2));
    if (radL < 1) radL = 1;
    if (radR < 1) radR = 1;

    // 1. Render Eye Bodies
    u8g2.drawRBox(xL, yL, (int16_t)m_leftWidth, (int16_t)effectiveLH, radL);
    u8g2.drawRBox(xR, yR, (int16_t)m_rightWidth, (int16_t)effectiveRH, radR);

    // 2. Render Mood Features & Cutouts
    if (m_blinkProgress > 0.4f) {
        if (m_mood == EyeMood::HAPPY) {
            // Joyous crescent cheeks: Cut out lower third with inverted curves
            u8g2.setDrawColor(0);
            int16_t cutH = (int16_t)(effectiveLH * 0.40f);
            u8g2.drawBox(xL - 2, yL + (int16_t)effectiveLH - cutH, (int16_t)m_leftWidth + 4, cutH + 2);
            u8g2.drawBox(xR - 2, yR + (int16_t)effectiveRH - cutH, (int16_t)m_rightWidth + 4, cutH + 2);

            // Re-round the smiling curved bottom
            u8g2.setDrawColor(1);
            u8g2.drawRBox(xL, yL, (int16_t)m_leftWidth, (int16_t)(effectiveLH - cutH + 4), 8);
            u8g2.drawRBox(xR, yR, (int16_t)m_rightWidth, (int16_t)(effectiveRH - cutH + 4), 8);
        } else if (m_mood == EyeMood::ANGRY) {
            // Angled furrowed brow slanting inward
            u8g2.setDrawColor(0);
            for (int i = 0; i < 15; ++i) {
                u8g2.drawHLine(xL + (int16_t)m_leftWidth - 15 + i, yL, 15 - i);
                u8g2.drawHLine(xR, yR, 15 - i);
            }
            u8g2.setDrawColor(1);
        } else if (m_mood == EyeMood::SKEPTICAL) {
            // Angled eyebrow slant on the right eye
            u8g2.setDrawColor(0);
            for (int i = 0; i < 10; ++i) {
                u8g2.drawHLine(xR, yR + i, 10 - i);
            }
            u8g2.setDrawColor(1);
            drawCorneaGlint(u8g2, xL + (int16_t)(m_leftWidth / 2), yL + (int16_t)(effectiveLH / 2), m_leftWidth, effectiveLH, true);
        } else {
            // 3D Glassy Cornea highlights for NEUTRAL, SLEEPY, SURPRISED
            drawCorneaGlint(u8g2, xL + (int16_t)(m_leftWidth / 2), yL + (int16_t)(effectiveLH / 2), m_leftWidth, effectiveLH, true);
            drawCorneaGlint(u8g2, xR + (int16_t)(m_rightWidth / 2), yR + (int16_t)(effectiveRH / 2), m_rightWidth, effectiveRH, false);
        }
    }
}
