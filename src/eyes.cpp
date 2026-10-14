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
      m_pupilWidth(14.0f),
      m_pupilHeight(18.0f),
      m_targetPupilWidth(14.0f),
      m_targetPupilHeight(18.0f),
      m_pupilOffsetX(0.0f),
      m_pupilOffsetY(0.0f),
      m_targetPupilOffsetX(0.0f),
      m_targetPupilOffsetY(0.0f),
      m_pupilVelX(0.0f),
      m_pupilVelY(0.0f),
      m_leftBrowInnerY(8.0f),
      m_leftBrowMidY(6.8f),
      m_leftBrowOuterY(8.5f),
      m_rightBrowInnerY(8.0f),
      m_rightBrowMidY(6.8f),
      m_rightBrowOuterY(8.5f),
      m_targetLeftBrowInnerY(8.0f),
      m_targetLeftBrowMidY(6.8f),
      m_targetLeftBrowOuterY(8.5f),
      m_targetRightBrowInnerY(8.0f),
      m_targetRightBrowMidY(6.8f),
      m_targetRightBrowOuterY(8.5f),
      m_browInertiaY(0.0f),
      m_browBounce(0.0f),
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
      m_lastFrameMs(0),
      m_scanPhase(0.0f),
      m_heartPulsePhase(0.0f) {
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
    else if (strcasecmp(moodStr, "love") == 0 || strcasecmp(moodStr, "heart") == 0) m_mood = EyeMood::LOVE;
    else if (strcasecmp(moodStr, "wink") == 0) m_mood = EyeMood::WINK;
    else if (strcasecmp(moodStr, "cat") == 0 || strcasecmp(moodStr, "kitty") == 0 || strcasecmp(moodStr, "meow") == 0) m_mood = EyeMood::CAT;
    else if (strcasecmp(moodStr, "owl") == 0 || strcasecmp(moodStr, "hoot") == 0) m_mood = EyeMood::OWL;
    else m_mood = EyeMood::NEUTRAL;
}

const char* RobotEyes::getMoodString() const {
    switch (m_mood) {
        case EyeMood::HAPPY: return "HAPPY";
        case EyeMood::SKEPTICAL: return "SKEPTICAL";
        case EyeMood::ANGRY: return "ANGRY";
        case EyeMood::SLEEPY: return "SLEEPY";
        case EyeMood::LOVE: return "LOVE";
        case EyeMood::WINK: return "WINK";
        case EyeMood::CAT: return "CAT";
        case EyeMood::OWL: return "OWL";
        default: return "NEUTRAL";
    }
}

void RobotEyes::nextMood() {
    switch (m_mood) {
        case EyeMood::NEUTRAL: m_mood = EyeMood::HAPPY; break;
        case EyeMood::HAPPY: m_mood = EyeMood::SKEPTICAL; break;
        case EyeMood::SKEPTICAL: m_mood = EyeMood::ANGRY; break;
        case EyeMood::ANGRY: m_mood = EyeMood::SLEEPY; break;
        case EyeMood::SLEEPY: m_mood = EyeMood::LOVE; break;
        case EyeMood::LOVE: m_mood = EyeMood::WINK; break;
        case EyeMood::WINK: m_mood = EyeMood::CAT; break;
        case EyeMood::CAT: m_mood = EyeMood::OWL; break;
        case EyeMood::OWL: m_mood = EyeMood::NEUTRAL; break;
        default: m_mood = EyeMood::NEUTRAL; break;
    }
}

void RobotEyes::triggerBlink() {
    m_isBlinking = true;
    m_blinkStage = 1;
    m_blinkStartMs = millis();
    m_browBounce = 2.4f;
}

void RobotEyes::look(int8_t dirX, int8_t dirY) {
    m_targetPupilOffsetX = constrain(dirX * 9, -10, 10);
    m_targetPupilOffsetY = constrain(dirY * 6, -7, 7);
    m_targetOffsetX = constrain(dirX * 4, -5, 5);
    m_targetOffsetY = constrain(dirY * 3, -4, 4);
}

void RobotEyes::updateBlink(unsigned long now) {
    // 1. Spontaneous natural blink trigger
    if (!m_isBlinking) {
        bool shouldBlink = false;

        if (m_doubleBlinkPending && now >= m_doubleBlinkTriggerMs) {
            shouldBlink = true;
            m_doubleBlinkPending = false;
        } else if (now - m_lastBlinkMs >= m_nextBlinkIntervalMs) {
            shouldBlink = true;
            if (random(0, 100) < 25 && m_mood != EyeMood::SLEEPY) {
                m_doubleBlinkPending = true;
                m_doubleBlinkTriggerMs = now + 340;
            }
            m_lastBlinkMs = now;
            if (m_mood == EyeMood::SLEEPY) {
                m_nextBlinkIntervalMs = random(4000, 7000);
            } else {
                m_nextBlinkIntervalMs = random(2500, 5000);
            }
        }

        if (shouldBlink) {
            m_isBlinking = true;
            m_blinkStage = 1;
            m_blinkStartMs = now;
            m_browBounce = 2.0f;
        }
        return;
    }

    // 2. High-performance non-linear asymmetric blink curve
    unsigned long elapsed = now - m_blinkStartMs;
    constexpr unsigned long CLOSE_DURATION_MS = 60;
    constexpr unsigned long CLOSED_PAUSE_MS   = 22;
    constexpr unsigned long OPEN_DURATION_MS  = 135;

    if (m_blinkStage == 1) { // Closing
        if (elapsed < CLOSE_DURATION_MS) {
            float t = (float)elapsed / CLOSE_DURATION_MS;
            m_blinkProgress = 1.0f - (t * t);
        } else {
            m_blinkProgress = 0.05f;
            m_blinkStage = 2;
            m_blinkStartMs = now;
        }
    } else if (m_blinkStage == 2) { // Closed slit pause
        if (elapsed >= CLOSED_PAUSE_MS) {
            m_blinkStage = 3;
            m_blinkStartMs = now;
            m_browBounce = -1.2f;
        }
    } else if (m_blinkStage == 3) { // Opening with elastic ease-out
        if (elapsed < OPEN_DURATION_MS) {
            float t = (float)elapsed / OPEN_DURATION_MS;
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
    if (now - m_lastGazeMs >= m_nextGazeIntervalMs) {
        // Natural multi-directional gaze wander
        m_lastGazeMs = now;
        m_nextGazeIntervalMs = random(1600, 3800);

        int r = random(0, 100);
        if (r < 34) {
            // 1. Direct eye contact (straight at user)
            m_targetPupilOffsetX = 0.0f;
            m_targetPupilOffsetY = 0.0f;
            m_targetOffsetX = 0.0f;
            m_targetOffsetY = 0.0f;
        } else if (r < 56) {
            // 2. Look DOWN at desk/table (natural reading/companion angle)
            m_targetPupilOffsetY = (float)random(5, 8);
            m_targetPupilOffsetX = (float)random(-4, 5);
            m_targetOffsetX = m_targetPupilOffsetX * 0.35f;
            m_targetOffsetY = 3.5f;
        } else if (r < 72) {
            // 3. Glance LEFT
            m_targetPupilOffsetX = (float)random(-10, -7);
            m_targetPupilOffsetY = (float)random(-2, 4);
            m_targetOffsetX = -4.5f;
            m_targetOffsetY = m_targetPupilOffsetY * 0.4f;
        } else if (r < 88) {
            // 4. Glance RIGHT
            m_targetPupilOffsetX = (float)random(7, 11);
            m_targetPupilOffsetY = (float)random(-2, 4);
            m_targetOffsetX = 4.5f;
            m_targetOffsetY = m_targetPupilOffsetY * 0.4f;
        } else {
            // 5. Look slightly UP at user
            m_targetPupilOffsetX = (float)random(-4, 5);
            m_targetPupilOffsetY = (float)random(-6, -4);
            m_targetOffsetX = m_targetPupilOffsetX * 0.35f;
            m_targetOffsetY = -3.0f;
        }
    }

    // Spring physics for smooth pupil saccades
    constexpr float PUPIL_K = 0.36f;
    constexpr float PUPIL_D = 0.54f;

    float fpx = (m_targetPupilOffsetX - m_pupilOffsetX) * PUPIL_K;
    m_pupilVelX = (m_pupilVelX + fpx) * PUPIL_D;
    m_pupilOffsetX += m_pupilVelX;

    float fpy = (m_targetPupilOffsetY - m_pupilOffsetY) * PUPIL_K;
    m_pupilVelY = (m_pupilVelY + fpy) * PUPIL_D;
    m_pupilOffsetY += m_pupilVelY;

    // Head / Eye body spring
    constexpr float EYE_K = 0.28f;
    constexpr float EYE_D = 0.58f;

    float fex = (m_targetOffsetX - m_offsetX) * EYE_K;
    m_velX = (m_velX + fex) * EYE_D;
    m_offsetX += m_velX;

    float fey = (m_targetOffsetY - m_offsetY) * EYE_K;
    m_velY = (m_velY + fey) * EYE_D;
    m_offsetY += m_velY;

    // Physiological Ocular Micro-Tremor
    m_tremorX = (sinf(now * 0.007f) * 0.4f) + (sinf(now * 0.017f) * 0.2f);
    m_tremorY = (cosf(now * 0.006f) * 0.3f);

    // Organic Respiration Rhythm
    float breathPhase = now * 0.0022f;
    m_breathOffset = sinf(breathPhase) * 1.3f;
    m_breathScale  = 1.0f + (sinf(breathPhase) * 0.025f);

    // Eyebrow Inertial Secondary Motion
    float targetBrowGaze = (m_offsetY * 0.65f) + (m_pupilOffsetY * 0.25f);
    m_browInertiaY += (targetBrowGaze - m_browInertiaY) * 0.20f;
    m_browBounce *= 0.82f;
}

void RobotEyes::updateMoodDynamics(unsigned long now) {
    m_heartPulsePhase += 0.06f;

    // Targets for eye sizes, pupil sizes, and 3-point eyebrows
    switch (m_mood) {
        case EyeMood::HAPPY:
            m_targetLeftWidth   = 36.0f;
            m_targetRightWidth  = 36.0f;
            m_targetLeftHeight  = 36.0f;
            m_targetRightHeight = 36.0f;
            m_targetPupilWidth  = 13.0f;
            m_targetPupilHeight = 16.0f;
            // Joyous upward curved crescent brows
            m_targetLeftBrowInnerY  = 6.8f;
            m_targetLeftBrowMidY    = 3.8f;
            m_targetLeftBrowOuterY  = 7.2f;
            m_targetRightBrowInnerY = 6.8f;
            m_targetRightBrowMidY   = 3.8f;
            m_targetRightBrowOuterY = 7.2f;
            break;

        case EyeMood::SKEPTICAL:
            // Left eye arches high; right eye squints
            m_targetLeftWidth   = 35.0f;
            m_targetRightWidth  = 33.0f;
            m_targetLeftHeight  = 42.0f;
            m_targetRightHeight = 22.0f;
            m_targetPupilWidth  = 12.0f;
            m_targetPupilHeight = 14.0f;
            // Left brow high questioning arch, right brow furrowed low
            m_targetLeftBrowInnerY  = 4.0f;
            m_targetLeftBrowMidY    = 1.5f;
            m_targetLeftBrowOuterY  = 4.5f;
            m_targetRightBrowInnerY = 14.0f;
            m_targetRightBrowMidY   = 11.8f;
            m_targetRightBrowOuterY = 9.5f;
            break;

        case EyeMood::ANGRY:
            m_targetLeftWidth   = 34.0f;
            m_targetRightWidth  = 34.0f;
            m_targetLeftHeight  = 32.0f;
            m_targetRightHeight = 32.0f;
            m_targetPupilWidth  = 10.0f; // Constricted focused pupils
            m_targetPupilHeight = 14.0f;
            // Fierce downward chevron furrow
            m_targetLeftBrowInnerY  = 15.0f;
            m_targetLeftBrowMidY    = 9.5f;
            m_targetLeftBrowOuterY  = 4.0f;
            m_targetRightBrowInnerY = 15.0f;
            m_targetRightBrowMidY   = 9.5f;
            m_targetRightBrowOuterY = 4.0f;
            break;

        case EyeMood::SLEEPY:
            m_targetLeftWidth   = 34.0f;
            m_targetRightWidth  = 34.0f;
            m_targetLeftHeight  = 18.0f;
            m_targetRightHeight = 18.0f;
            m_targetPupilWidth  = 12.0f;
            m_targetPupilHeight = 12.0f;
            // Heavy, drooping brows
            m_targetLeftBrowInnerY  = 8.5f;
            m_targetLeftBrowMidY    = 11.2f;
            m_targetLeftBrowOuterY  = 14.5f;
            m_targetRightBrowInnerY = 8.5f;
            m_targetRightBrowMidY   = 11.2f;
            m_targetRightBrowOuterY = 14.5f;
            break;

        case EyeMood::LOVE:
            m_targetLeftWidth   = 36.0f;
            m_targetRightWidth  = 36.0f;
            m_targetLeftHeight  = 36.0f;
            m_targetRightHeight = 36.0f;
            m_targetPupilWidth  = 14.0f;
            m_targetPupilHeight = 18.0f;
            // Playful affectionate upward brows
            m_targetLeftBrowInnerY  = 5.5f;
            m_targetLeftBrowMidY    = 3.0f;
            m_targetLeftBrowOuterY  = 6.5f;
            m_targetRightBrowInnerY = 5.5f;
            m_targetRightBrowMidY   = 3.0f;
            m_targetRightBrowOuterY = 6.5f;
            break;

        case EyeMood::WINK:
            m_targetLeftWidth   = 36.0f;
            m_targetRightWidth  = 34.0f;
            m_targetLeftHeight  = 40.0f;
            m_targetRightHeight = 4.0f; // Right eye closed wink
            m_targetPupilWidth  = 14.0f;
            m_targetPupilHeight = 18.0f;
            // Left brow relaxed, right brow raised in cheeky wink
            m_targetLeftBrowInnerY  = 8.0f;
            m_targetLeftBrowMidY    = 6.5f;
            m_targetLeftBrowOuterY  = 8.0f;
            m_targetRightBrowInnerY = 5.0f;
            m_targetRightBrowMidY   = 2.8f;
            m_targetRightBrowOuterY = 5.5f;
            break;

        case EyeMood::CAT:
            m_targetLeftWidth   = 34.0f;
            m_targetRightWidth  = 34.0f;
            m_targetLeftHeight  = 34.0f;
            m_targetRightHeight = 34.0f;
            m_targetPupilWidth  = 6.0f;  // Feline vertical slit
            m_targetPupilHeight = 24.0f;
            m_targetLeftBrowInnerY  = 6.0f;
            m_targetLeftBrowMidY    = 4.5f;
            m_targetLeftBrowOuterY  = 6.0f;
            m_targetRightBrowInnerY = 6.0f;
            m_targetRightBrowMidY   = 4.5f;
            m_targetRightBrowOuterY = 6.0f;
            break;

        case EyeMood::OWL:
            m_targetLeftWidth   = 44.0f;
            m_targetRightWidth  = 44.0f;
            m_targetLeftHeight  = 44.0f;
            m_targetRightHeight = 44.0f;
            m_targetPupilWidth  = 18.0f; // Giant round nocturnal pupils
            m_targetPupilHeight = 18.0f;
            m_targetLeftBrowInnerY  = 4.0f;
            m_targetLeftBrowMidY    = 2.0f;
            m_targetLeftBrowOuterY  = 4.0f;
            m_targetRightBrowInnerY = 4.0f;
            m_targetRightBrowMidY   = 2.0f;
            m_targetRightBrowOuterY = 4.0f;
            break;

        default: // NEUTRAL
            m_targetLeftWidth   = (float)DEFAULT_WIDTH;
            m_targetRightWidth  = (float)DEFAULT_WIDTH;
            m_targetLeftHeight  = (float)DEFAULT_HEIGHT;
            m_targetRightHeight = (float)DEFAULT_HEIGHT;
            m_targetPupilWidth  = 14.0f;
            m_targetPupilHeight = 18.0f;
            // Relaxed, balanced, natural subtle arch
            m_targetLeftBrowInnerY  = 8.0f;
            m_targetLeftBrowMidY    = 6.8f;
            m_targetLeftBrowOuterY  = 8.5f;
            m_targetRightBrowInnerY = 8.0f;
            m_targetRightBrowMidY   = 6.8f;
            m_targetRightBrowOuterY = 8.5f;
            break;
    }

    // Smooth organic interpolation (lerp)
    constexpr float LERP_SPEED = 0.16f;
    m_leftHeight   += (m_targetLeftHeight   - m_leftHeight)   * LERP_SPEED;
    m_rightHeight  += (m_targetRightHeight  - m_rightHeight)  * LERP_SPEED;
    m_leftWidth    += (m_targetLeftWidth    - m_leftWidth)    * LERP_SPEED;
    m_rightWidth   += (m_targetRightWidth   - m_rightWidth)   * LERP_SPEED;

    m_pupilWidth   += (m_targetPupilWidth   - m_pupilWidth)   * LERP_SPEED;
    m_pupilHeight  += (m_targetPupilHeight  - m_pupilHeight)  * LERP_SPEED;

    m_leftBrowInnerY  += (m_targetLeftBrowInnerY  - m_leftBrowInnerY)  * LERP_SPEED;
    m_leftBrowMidY    += (m_targetLeftBrowMidY    - m_leftBrowMidY)    * LERP_SPEED;
    m_leftBrowOuterY  += (m_targetLeftBrowOuterY  - m_leftBrowOuterY)  * LERP_SPEED;

    m_rightBrowInnerY += (m_targetRightBrowInnerY - m_rightBrowInnerY) * LERP_SPEED;
    m_rightBrowMidY   += (m_targetRightBrowMidY   - m_rightBrowMidY)   * LERP_SPEED;
    m_rightBrowOuterY += (m_targetRightBrowOuterY - m_rightBrowOuterY) * LERP_SPEED;
}

void RobotEyes::update() {
    unsigned long now = millis();
    if (now - m_lastFrameMs < 16) {
        return;
    }
    m_lastFrameMs = now;

    updateBlink(now);
    updateGaze(now);
    updateMoodDynamics(now);
}

void RobotEyes::drawCurvedEyebrow(U8G2& u8g2, int16_t xIn, int16_t yIn, int16_t xMid, int16_t yMid, int16_t xOut, int16_t yOut, bool isLeft) {
    // 1. Inner to Mid segment (Tapered 4px to 3px)
    u8g2.drawLine(xIn, yIn,     xMid, yMid);
    u8g2.drawLine(xIn, yIn + 1, xMid, yMid + 1);
    u8g2.drawLine(xIn, yIn + 2, xMid, yMid + 2);
    u8g2.drawLine(xIn, yIn + 3, xMid, yMid + 2);

    // 2. Mid to Outer segment (Tapered 3px to 2px)
    u8g2.drawLine(xMid, yMid,     xOut, yOut);
    u8g2.drawLine(xMid, yMid + 1, xOut, yOut + 1);
    u8g2.drawLine(xMid, yMid + 2, xOut, yOut + 1);

    // 3. Rounded endcaps
    u8g2.drawDisc(xIn, yIn + 1, 1);
    u8g2.drawDisc(xMid, yMid + 1, 1);
}

void RobotEyes::drawHeartEye(U8G2& u8g2, int16_t cx, int16_t cy, float scale) {
    int16_t r = (int16_t)(6 * scale);
    int16_t offX = (int16_t)(6 * scale);
    int16_t offY = (int16_t)(4 * scale);
    int16_t tipY = (int16_t)(14 * scale);

    // Dual top lobes
    u8g2.drawDisc(cx - offX, cy - offY, r);
    u8g2.drawDisc(cx + offX, cy - offY, r);

    // Center body and bottom taper
    u8g2.drawBox(cx - offX, cy - offY, (offX * 2) + 1, r);
    u8g2.drawTriangle(cx - (int16_t)(10 * scale), cy, cx + (int16_t)(10 * scale), cy, cx, cy + tipY);
}

void RobotEyes::drawWinkEye(U8G2& u8g2, int16_t cx, int16_t cy, float w) {
    // Upward cheerful crescent arc for closed eye
    int16_t halfW = (int16_t)(w / 2);
    u8g2.drawCircle(cx, cy - 6, halfW, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
    u8g2.drawCircle(cx, cy - 7, halfW, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
    u8g2.drawCircle(cx, cy - 8, halfW, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);

    // Cheek smile tick
    u8g2.drawLine(cx - halfW, cy - 6, cx - halfW - 2, cy - 8);
    u8g2.drawLine(cx + halfW, cy - 6, cx + halfW + 2, cy - 8);
}

void RobotEyes::drawSingleEye(U8G2& u8g2, int16_t x, int16_t y, float w, float h, float pW, float pH, float pOffX, float pOffY, bool isLeft) {
    int16_t rad = min((int16_t)CORNER_RADIUS, (int16_t)(h / 2));
    if (rad < 1) rad = 1;

    // 1. Draw outer white eye capsule
    u8g2.setDrawColor(1);
    u8g2.drawRBox(x, y, (int16_t)w, (int16_t)h, rad);

    // If blinking or eye slit, don't draw inner pupil
    if (h < 14.0f) return;

    // 2. Draw dark active pupil inside
    int16_t centerX = x + (int16_t)(w / 2);
    int16_t centerY = y + (int16_t)(h / 2);

    // Clamp pupil offset so it stays naturally inside the white eye boundaries
    float maxPoffX = (w / 2.0f) - (pW / 2.0f) - 2.0f;
    float maxPoffY = (h / 2.0f) - (pH / 2.0f) - 2.0f;
    if (maxPoffX < 0) maxPoffX = 0;
    if (maxPoffY < 0) maxPoffY = 0;

    int16_t pX = centerX - (int16_t)(pW / 2) + (int16_t)constrain(pOffX, -maxPoffX, maxPoffX);
    int16_t pY = centerY - (int16_t)(pH / 2) + (int16_t)constrain(pOffY, -maxPoffY, maxPoffY);

    int16_t pRad = min((int16_t)4, (int16_t)(pH / 3));
    if (pRad < 1) pRad = 1;

    u8g2.setDrawColor(0);
    u8g2.drawRBox(pX, pY, (int16_t)pW, (int16_t)pH, pRad);

    // 3. Draw specular white glint inside dark pupil
    if (pH >= 10.0f) {
        u8g2.setDrawColor(1);
        u8g2.drawDisc(pX + 3, pY + 3, 1);
        if (pW >= 14.0f) {
            u8g2.drawPixel(pX + 5, pY + 5);
        }
    }

    u8g2.setDrawColor(1);
}

void RobotEyes::drawCatEyes(U8G2& u8g2, int16_t xL, int16_t yL, int16_t xR, int16_t yR, float pOffX, float pOffY, float blinkProgress) {
    u8g2.setDrawColor(1);

    // 1. Pointed Cat Ears (Triangle with inner ear detail)
    // Left Ear
    u8g2.drawTriangle(xL + 1, yL + 1, xL + 18, yL + 1, xL + 6, yL - 13);
    u8g2.setDrawColor(0);
    u8g2.drawTriangle(xL + 4, yL + 1, xL + 15, yL + 1, xL + 8, yL - 9);
    u8g2.setDrawColor(1);

    // Right Ear
    u8g2.drawTriangle(xR + 15, yR + 1, xR + 32, yR + 1, xR + 27, yR - 13);
    u8g2.setDrawColor(0);
    u8g2.drawTriangle(xR + 18, yR + 1, xR + 29, yR + 1, xR + 25, yR - 9);
    u8g2.setDrawColor(1);

    // 2. Feline Eye Capsules (w=34, h up to 34)
    int16_t h = (int16_t)(34.0f * blinkProgress);
    if (h < 2) h = 2;
    int16_t rad = min((int16_t)8, (int16_t)(h / 2));
    if (rad < 1) rad = 1;

    u8g2.drawRBox(xL, yL, 34, h, rad);
    u8g2.drawRBox(xR, yR, 34, h, rad);

    // 3. Vertical Elliptical Slit Pupils with Specular Glint
    if (h >= 10) {
        u8g2.setDrawColor(0);
        int16_t cxL = xL + 17 + (int16_t)constrain(pOffX, -9.0f, 9.0f);
        int16_t cyL = yL + (h / 2) + (int16_t)constrain(pOffY, -5.0f, 5.0f);
        int16_t cxR = xR + 17 + (int16_t)constrain(pOffX, -9.0f, 9.0f);
        int16_t cyR = yR + (h / 2) + (int16_t)constrain(pOffY, -5.0f, 5.0f);

        int16_t slitRy = (int16_t)((h - 6) / 2);
        if (slitRy < 2) slitRy = 2;
        u8g2.drawFilledEllipse(cxL, cyL, 2, slitRy);
        u8g2.drawFilledEllipse(cxR, cyR, 2, slitRy);

        // Specular glint
        u8g2.setDrawColor(1);
        u8g2.drawPixel(cxL + 3, cyL - 3);
        u8g2.drawPixel(cxR + 3, cyR - 3);
    }

    // 4. Cute Cat Nose & Whisker Muzzle
    u8g2.setDrawColor(1);
    // Nose triangle (inverted)
    u8g2.drawTriangle(61, 46, 67, 46, 64, 49);
    // Philtrum and mouth curves
    u8g2.drawLine(64, 49, 64, 51);
    u8g2.drawLine(64, 51, 60, 53);
    u8g2.drawLine(64, 51, 68, 53);

    // Whiskers
    u8g2.drawLine(xL - 2, 44, xL - 15, 41);
    u8g2.drawLine(xL - 2, 48, xL - 15, 49);
    u8g2.drawLine(xR + 35, 44, xR + 48, 41);
    u8g2.drawLine(xR + 35, 48, xR + 48, 49);
}

void RobotEyes::drawOwlEyes(U8G2& u8g2, float pOffX, float pOffY, float blinkProgress) {
    u8g2.setDrawColor(1);

    int16_t cxL = 36;
    int16_t cyL = 33 + (int16_t)m_offsetY;
    int16_t cxR = 92;
    int16_t cyR = 33 + (int16_t)m_offsetY;

    // 1. Owl Feather Tufts (Plumicorns / Ear Horns)
    u8g2.drawTriangle(22, cyL - 19, 30, cyL - 21, 13, cyL - 31);
    u8g2.drawTriangle(20, cyL - 19, 26, cyL - 20, 11, cyL - 29);
    u8g2.drawTriangle(106, cyR - 19, 98, cyR - 21, 115, cyR - 31);
    u8g2.drawTriangle(108, cyR - 19, 102, cyR - 20, 117, cyR - 29);

    // 2. Brow Ridge V connecting above the beak
    u8g2.drawLine(34, cyL - 19, 64, cyL - 9);
    u8g2.drawLine(34, cyL - 18, 64, cyL - 8);
    u8g2.drawLine(94, cyR - 19, 64, cyR - 9);
    u8g2.drawLine(94, cyR - 18, 64, cyR - 8);

    // 3. Large Feathery Facial Disc Rings
    u8g2.drawCircle(cxL, cyL, 22);
    u8g2.drawCircle(cxL, cyL, 20);
    u8g2.drawCircle(cxR, cyR, 22);
    u8g2.drawCircle(cxR, cyR, 20);

    // 4. White Sclera Disc (responds to blinking)
    int16_t radY = (int16_t)(17.0f * blinkProgress);
    if (radY < 2) radY = 2;
    u8g2.drawFilledEllipse(cxL, cyL, 17, radY);
    u8g2.drawFilledEllipse(cxR, cyR, 17, radY);

    // 5. Giant Nocturnal Black Owl Pupils
    if (radY >= 6) {
        u8g2.setDrawColor(0);
        int16_t pxL = cxL + (int16_t)constrain(pOffX * 0.65f, -6.0f, 6.0f);
        int16_t pyL = cyL + (int16_t)constrain(pOffY * 0.65f, -5.0f, 5.0f);
        int16_t pxR = cxR + (int16_t)constrain(pOffX * 0.65f, -6.0f, 6.0f);
        int16_t pyR = cyR + (int16_t)constrain(pOffY * 0.65f, -5.0f, 5.0f);

        int16_t pRadY = min((int16_t)9, (int16_t)(radY - 2));
        if (pRadY < 2) pRadY = 2;
        u8g2.drawFilledEllipse(pxL, pyL, 9, pRadY);
        u8g2.drawFilledEllipse(pxR, pyR, 9, pRadY);

        // Specular eye reflections
        u8g2.setDrawColor(1);
        u8g2.drawDisc(pxL + 3, pyL - 3, 2);
        u8g2.drawDisc(pxR + 3, pyR - 3, 2);
        u8g2.drawPixel(pxL + 5, pyL);
        u8g2.drawPixel(pxR + 5, pyR);
    }

    // 6. Hooked Center Owl Beak
    u8g2.setDrawColor(1);
    u8g2.drawTriangle(60, cyL - 7, 68, cyL - 7, 64, cyL + 9);
    u8g2.setDrawColor(0);
    u8g2.drawLine(64, cyL - 6, 64, cyL + 8);
    u8g2.setDrawColor(1);
}

void RobotEyes::draw(U8G2& u8g2) {
    constexpr int16_t BASE_X = 21;
    constexpr int16_t BASE_Y = 16;

    float totalOffsetX = m_offsetX + m_tremorX;
    float totalOffsetY = m_offsetY + m_tremorY + m_breathOffset;

    // Effective eyelid heights
    float effectiveLH = m_leftHeight  * m_blinkProgress * m_breathScale;
    float effectiveRH = m_rightHeight * m_blinkProgress * m_breathScale;
    if (effectiveLH < 2.5f) effectiveLH = 2.5f;
    if (effectiveRH < 2.5f) effectiveRH = 2.5f;

    // Asymmetric eyelid drop (upper lid drops 82%, lower lid rises 18%)
    float dropL = (m_leftHeight  - effectiveLH) * 0.82f;
    float dropR = (m_rightHeight - effectiveRH) * 0.82f;

    int16_t xL = BASE_X + (int16_t)totalOffsetX;
    int16_t xR = BASE_X + (int16_t)m_leftWidth + EYE_SPACING + (int16_t)totalOffsetX;

    float moodLiftL = (m_mood == EyeMood::SKEPTICAL) ? -3.0f : 0.0f;
    float moodLiftR = (m_mood == EyeMood::SKEPTICAL) ?  2.0f : 0.0f;

    int16_t yL = BASE_Y + (int16_t)(totalOffsetY + dropL + moodLiftL);
    int16_t yR = BASE_Y + (int16_t)(totalOffsetY + dropR + moodLiftR);

    // Dedicated Full Facial Renderers for Cat and Owl
    if (m_mood == EyeMood::CAT) {
        drawCatEyes(u8g2, xL, yL, xR, yR, m_pupilOffsetX, m_pupilOffsetY, m_blinkProgress);
        return;
    }
    if (m_mood == EyeMood::OWL) {
        drawOwlEyes(u8g2, m_pupilOffsetX, m_pupilOffsetY, m_blinkProgress);
        return;
    }

    // 1. Render Eye Bodies
    if (m_mood == EyeMood::LOVE) {
        float pulse = 1.0f + (sinf(m_heartPulsePhase) * 0.10f);
        drawHeartEye(u8g2, xL + (int16_t)(m_leftWidth / 2), yL + 16, pulse);
        drawHeartEye(u8g2, xR + (int16_t)(m_rightWidth / 2), yR + 16, pulse);
    } else if (m_mood == EyeMood::WINK) {
        // Left eye active, right eye winks
        drawSingleEye(u8g2, xL, yL, m_leftWidth, effectiveLH, m_pupilWidth, m_pupilHeight, m_pupilOffsetX, m_pupilOffsetY, true);
        drawWinkEye(u8g2, xR + (int16_t)(m_rightWidth / 2), yR + 18, m_rightWidth);
    } else {
        drawSingleEye(u8g2, xL, yL, m_leftWidth, effectiveLH, m_pupilWidth, m_pupilHeight, m_pupilOffsetX, m_pupilOffsetY, true);
        drawSingleEye(u8g2, xR, yR, m_rightWidth, effectiveRH, m_pupilWidth, m_pupilHeight, m_pupilOffsetX, m_pupilOffsetY, false);
    }

    // 2. Render Articulated 3-Point Curved Eyebrows
    float browMotionY = m_browInertiaY + m_breathOffset + m_browBounce;

    // Left Eyebrow
    int16_t lInX  = xL + (int16_t)m_leftWidth + 2;
    int16_t lMidX = xL + (int16_t)(m_leftWidth * 0.45f);
    int16_t lOutX = xL - 3;
    int16_t lInY  = constrain((int16_t)(m_leftBrowInnerY + browMotionY), 0, 22);
    int16_t lMidY = constrain((int16_t)(m_leftBrowMidY   + browMotionY), 0, 22);
    int16_t lOutY = constrain((int16_t)(m_leftBrowOuterY + browMotionY), 0, 22);
    drawCurvedEyebrow(u8g2, lInX, lInY, lMidX, lMidY, lOutX, lOutY, true);

    // Right Eyebrow
    int16_t rInX  = xR - 2;
    int16_t rMidX = xR + (int16_t)(m_rightWidth * 0.55f);
    int16_t rOutX = xR + (int16_t)m_rightWidth + 3;
    int16_t rInY  = constrain((int16_t)(m_rightBrowInnerY + browMotionY), 0, 22);
    int16_t rMidY = constrain((int16_t)(m_rightBrowMidY   + browMotionY), 0, 22);
    int16_t rOutY = constrain((int16_t)(m_rightBrowOuterY + browMotionY), 0, 22);
    drawCurvedEyebrow(u8g2, rInX, rInY, rMidX, rMidY, rOutX, rOutY, false);

    // 3. Render Mood Cutouts & Smiling Cheeks
    if (m_blinkProgress > 0.4f && m_mood == EyeMood::HAPPY) {
        u8g2.setDrawColor(0);
        int16_t cutH = (int16_t)(effectiveLH * 0.35f);
        u8g2.drawBox(xL - 2, yL + (int16_t)effectiveLH - cutH, (int16_t)m_leftWidth + 4, cutH + 2);
        u8g2.drawBox(xR - 2, yR + (int16_t)effectiveRH - cutH, (int16_t)m_rightWidth + 4, cutH + 2);

        u8g2.setDrawColor(1);
        u8g2.drawRBox(xL, yL, (int16_t)m_leftWidth, (int16_t)(effectiveLH - cutH + 3), 8);
        u8g2.drawRBox(xR, yR, (int16_t)m_rightWidth, (int16_t)(effectiveRH - cutH + 3), 8);
    }
}
