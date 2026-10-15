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
      m_squashX(0.0f),
      m_targetSquashX(0.0f),
      m_pupilDilation(1.0f),
      m_targetPupilDilation(1.0f),
      m_browCuriosityTwitch(0.0f),
      m_nextTwitchMs(4500),
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
    m_browBounce = 1.8f;
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
            if (random(0, 100) < 22 && m_mood != EyeMood::SLEEPY) {
                m_doubleBlinkPending = true;
                m_doubleBlinkTriggerMs = now + random(300, 420);
            }
            m_lastBlinkMs = now;
            if (m_mood == EyeMood::SLEEPY) {
                m_nextBlinkIntervalMs = random(4000, 7000);
            } else {
                m_nextBlinkIntervalMs = random(2600, 5200);
            }
        }

        if (shouldBlink) {
            m_isBlinking = true;
            m_blinkStage = 1;
            m_blinkStartMs = now;
            m_browBounce = 1.6f;
        }
        return;
    }

    // 2. High-fidelity non-linear asymmetric biological blink curve
    unsigned long elapsed = now - m_blinkStartMs;
    constexpr unsigned long CLOSE_DURATION_MS = 85;  // Fast, sharp downstroke
    constexpr unsigned long CLOSED_PAUSE_MS   = 28;  // Clear momentary closed contact
    constexpr unsigned long OPEN_DURATION_MS  = 175; // Smooth elastic ease-out upstroke

    if (m_blinkStage == 1) { // Closing
        if (elapsed < CLOSE_DURATION_MS) {
            float t = (float)elapsed / CLOSE_DURATION_MS;
            m_blinkProgress = 1.0f - (t * t * (3.0f - 2.0f * t));
        } else {
            m_blinkProgress = 0.0f;
            m_blinkStage = 2;
            m_blinkStartMs = now;
        }
    } else if (m_blinkStage == 2) { // Closed contact rest
        m_blinkProgress = 0.0f;
        if (elapsed >= CLOSED_PAUSE_MS) {
            m_blinkStage = 3;
            m_blinkStartMs = now;
            m_browBounce = -1.2f;
        }
    } else if (m_blinkStage == 3) { // Opening with elastic ease-out
        if (elapsed < OPEN_DURATION_MS) {
            float t = (float)elapsed / OPEN_DURATION_MS;
            float easeOut = 1.0f - powf(1.0f - t, 2.8f);
            m_blinkProgress = easeOut;
        } else {
            m_blinkProgress = 1.0f;
            m_isBlinking = false;
            m_blinkStage = 0;
        }
    }
}

void RobotEyes::updateGaze(unsigned long now) {
    if (now - m_lastGazeMs >= m_nextGazeIntervalMs) {
        // Natural multi-directional gaze wander with biological fixation timings
        m_lastGazeMs = now;
        m_nextGazeIntervalMs = random(1800, 4200);

        int r = random(0, 100);
        if (r < 32) {
            // 1. Direct eye contact (straight at user)
            m_targetPupilOffsetX = 0.0f;
            m_targetPupilOffsetY = 0.0f;
            m_targetOffsetX = 0.0f;
            m_targetOffsetY = 0.0f;
            m_targetSquashX = 0.0f;
            m_targetPupilDilation = 1.0f;
        } else if (r < 52) {
            // 2. Look DOWN at desk/table (natural reading/companion angle)
            m_targetPupilOffsetY = (float)random(5, 8);
            m_targetPupilOffsetX = (float)random(-4, 5);
            m_targetOffsetX = m_targetPupilOffsetX * 0.35f;
            m_targetOffsetY = 3.5f;
            m_targetSquashX = m_targetPupilOffsetX * 0.2f;
            m_targetPupilDilation = 0.95f; // Focusing constriction
        } else if (r < 72) {
            // 3. Glance LEFT (curious side glance)
            m_targetPupilOffsetX = (float)random(-10, -7);
            m_targetPupilOffsetY = (float)random(-3, 4);
            m_targetOffsetX = -4.5f;
            m_targetOffsetY = m_targetPupilOffsetY * 0.4f;
            m_targetSquashX = -2.4f; // Left eye expands, right narrows (3D perspective)
            m_targetPupilDilation = 1.15f; // Alert dilation
        } else if (r < 88) {
            // 4. Glance RIGHT
            m_targetPupilOffsetX = (float)random(7, 11);
            m_targetPupilOffsetY = (float)random(-3, 4);
            m_targetOffsetX = 4.5f;
            m_targetOffsetY = m_targetPupilOffsetY * 0.4f;
            m_targetSquashX = 2.4f; // Right eye expands, left narrows
            m_targetPupilDilation = 1.15f;
        } else {
            // 5. Look UP / Pondering
            m_targetPupilOffsetX = (float)random(-5, 6);
            m_targetPupilOffsetY = (float)random(-7, -4);
            m_targetOffsetX = m_targetPupilOffsetX * 0.35f;
            m_targetOffsetY = -3.5f;
            m_targetSquashX = 0.0f;
            m_targetPupilDilation = 1.20f;
        }
    }

    // Occasional subtle brow curiosity micro-twitch
    if (now >= m_nextTwitchMs) {
        m_nextTwitchMs = now + random(4500, 9500);
        if (m_mood == EyeMood::NEUTRAL || m_mood == EyeMood::SKEPTICAL) {
            m_browCuriosityTwitch = (random(0, 100) < 50) ? -2.0f : 1.6f;
        }
    }
    m_browCuriosityTwitch *= 0.88f;

    // Smooth organic perspective squash and pupil dilation
    m_squashX += (m_targetSquashX - m_squashX) * 0.22f;
    m_pupilDilation += (m_targetPupilDilation - m_pupilDilation) * 0.16f;

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
    // Organic curved tapered brow with anti-aliased look on OLED
    // Segment 1: Inner root to Mid arch (4px thick tapering to 3px)
    u8g2.drawLine(xIn, yIn,     xMid, yMid);
    u8g2.drawLine(xIn, yIn + 1, xMid, yMid + 1);
    u8g2.drawLine(xIn, yIn + 2, xMid, yMid + 2);
    u8g2.drawLine(xIn, yIn + 3, xMid, yMid + 2);

    // Inner brow root soft endcap
    u8g2.drawDisc(xIn, yIn + 1, 1);
    u8g2.drawPixel(xIn, yIn + 3);

    // Segment 2: Mid arch to Outer temple tail (3px tapering down to 1px)
    u8g2.drawLine(xMid, yMid,     xOut, yOut);
    u8g2.drawLine(xMid, yMid + 1, xOut, yOut);
    u8g2.drawLine(xMid, yMid + 2, xOut, yOut + 1);

    // Arch peak soft join
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

void RobotEyes::drawSingleEye(U8G2& u8g2, int16_t x, int16_t y, float w, float h, float pW, float pH, float pOffX, float pOffY, float blinkProgress, bool isLeft) {
    // 1. Biological Closed Eyelid Seam (when eye is fully closed)
    if (blinkProgress <= 0.08f) {
        u8g2.setDrawColor(1);
        int16_t seamY = y + (int16_t)(h * 0.74f);
        // Expressive curved closed-eye arc
        u8g2.drawLine(x + 2, seamY, x + (int16_t)w - 3, seamY);
        u8g2.drawLine(x + 5, seamY + 1, x + (int16_t)w - 6, seamY + 1);
        if (isLeft) {
            u8g2.drawLine(x + 2, seamY, x - 1, seamY - 2);
        } else {
            u8g2.drawLine(x + (int16_t)w - 3, seamY, x + (int16_t)w, seamY - 2);
        }
        return;
    }

    int16_t rad = min((int16_t)CORNER_RADIUS, (int16_t)(h / 2));
    if (rad < 2) rad = 2;

    // 2. Outer White Eye Capsule (sclera) rendered at stable anchored position
    u8g2.setDrawColor(1);
    u8g2.drawRBox(x, y, (int16_t)w, (int16_t)h, rad);

    // 3. Active pupil dynamics inside
    int16_t centerX = x + (int16_t)(w / 2);
    int16_t centerY = y + (int16_t)(h / 2);

    float effectivePW = pW * m_pupilDilation;
    float effectivePH = pH * m_pupilDilation;

    float maxPoffX = (w / 2.0f) - (effectivePW / 2.0f) - 2.5f;
    float maxPoffY = (h / 2.0f) - (effectivePH / 2.0f) - 2.0f;
    if (maxPoffX < 0) maxPoffX = 0;
    if (maxPoffY < 0) maxPoffY = 0;

    int16_t pX = centerX - (int16_t)(effectivePW / 2) + (int16_t)constrain(pOffX, -maxPoffX, maxPoffX);
    int16_t pY = centerY - (int16_t)(effectivePH / 2) + (int16_t)constrain(pOffY, -maxPoffY, maxPoffY);

    int16_t pRad = min((int16_t)5, (int16_t)(effectivePH / 3));
    if (pRad < 2) pRad = 2;

    u8g2.setDrawColor(0);
    u8g2.drawRBox(pX, pY, (int16_t)effectivePW, (int16_t)effectivePH, pRad);

    // 4. 3D Cornea Specular Catchlight
    if (effectivePH >= 12.0f && effectivePW >= 10.0f) {
        u8g2.setDrawColor(1);
        int16_t glintX = pX + (int16_t)(effectivePW * 0.65f);
        int16_t glintY = pY + 3;
        if (glintX > pX + (int16_t)effectivePW - 3) glintX = pX + (int16_t)effectivePW - 3;
        u8g2.drawDisc(glintX, glintY, 1);

        int16_t secGlintX = pX + 3;
        int16_t secGlintY = pY + (int16_t)effectivePH - 4;
        if (secGlintY > pY + 4 && secGlintX < glintX - 2) {
            u8g2.drawPixel(secGlintX, secGlintY);
        }
    }

    // 5. Biological Eyelid Sweeping Masks (Upper drops 80%, lower rises 20%)
    if (blinkProgress < 0.98f) {
        u8g2.setDrawColor(0); // Cut with face background color

        int16_t dropH = (int16_t)(h * (1.0f - blinkProgress) * 0.80f);
        if (dropH > 0) {
            u8g2.drawBox(x - 2, y - 4, (int16_t)w + 4, dropH + 4);
        }

        int16_t riseH = (int16_t)(h * (1.0f - blinkProgress) * 0.20f);
        if (riseH > 0) {
            u8g2.drawBox(x - 2, y + (int16_t)h - riseH, (int16_t)w + 4, riseH + 4);
        }

        u8g2.setDrawColor(1);
    }
}

void RobotEyes::drawCatEyes(U8G2& u8g2, int16_t xL, int16_t yL, int16_t xR, int16_t yR, float pOffX, float pOffY, float blinkProgress) {
    u8g2.setDrawColor(1);

    // Subtle ear twitch with glance
    int16_t earTwitchL = (pOffX < -3.0f) ? -1 : 0;
    int16_t earTwitchR = (pOffX > 3.0f) ? -1 : 0;

    // 1. Pointed Cat Ears (3D triangle with inner ear cavity)
    u8g2.drawTriangle(xL + 1, yL + 1, xL + 18, yL + 1, xL + 5, yL - 14 + earTwitchL);
    u8g2.setDrawColor(0);
    u8g2.drawTriangle(xL + 4, yL + 1, xL + 15, yL + 1, xL + 7, yL - 10 + earTwitchL);
    u8g2.setDrawColor(1);

    u8g2.drawTriangle(xR + 15, yR + 1, xR + 32, yR + 1, xR + 28, yR - 14 + earTwitchR);
    u8g2.setDrawColor(0);
    u8g2.drawTriangle(xR + 18, yR + 1, xR + 29, yR + 1, xR + 26, yR - 10 + earTwitchR);
    u8g2.setDrawColor(1);

    // 2. Feline Eyes or Sleeping Cat Closed Seams
    if (blinkProgress <= 0.08f) {
        int16_t seamYL = yL + 24;
        int16_t seamYR = yR + 24;
        // Cute sleeping cat crescent slits
        u8g2.drawCircle(xL + 17, seamYL - 4, 10, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
        u8g2.drawCircle(xL + 17, seamYL - 5, 10, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
        u8g2.drawCircle(xR + 17, seamYR - 4, 10, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
        u8g2.drawCircle(xR + 17, seamYR - 5, 10, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
    } else {
        u8g2.drawRBox(xL, yL, 34, 34, 9);
        u8g2.drawRBox(xR, yR, 34, 34, 9);

        // Feline Slit Pupils with Dynamic Dilation
        u8g2.setDrawColor(0);
        int16_t cxL = xL + 17 + (int16_t)constrain(pOffX, -8.0f, 8.0f);
        int16_t cyL = yL + 17 + (int16_t)constrain(pOffY, -5.0f, 5.0f);
        int16_t cxR = xR + 17 + (int16_t)constrain(pOffX, -8.0f, 8.0f);
        int16_t cyR = yR + 17 + (int16_t)constrain(pOffY, -5.0f, 5.0f);

        int16_t slitRx = (m_pupilDilation > 1.08f) ? 3 : 2;
        u8g2.drawFilledEllipse(cxL, cyL, slitRx, 14);
        u8g2.drawFilledEllipse(cxR, cyR, slitRx, 14);

        // Specular glint
        u8g2.setDrawColor(1);
        u8g2.drawPixel(cxL + slitRx + 1, cyL - 3);
        u8g2.drawPixel(cxR + slitRx + 1, cyR - 3);

        // Eyelid sweep mask
        if (blinkProgress < 0.98f) {
            u8g2.setDrawColor(0);
            int16_t dropH = (int16_t)(34.0f * (1.0f - blinkProgress) * 0.82f);
            if (dropH > 0) {
                u8g2.drawBox(xL - 2, yL - 2, 38, dropH + 2);
                u8g2.drawBox(xR - 2, yR - 2, 38, dropH + 2);
            }
            int16_t riseH = (int16_t)(34.0f * (1.0f - blinkProgress) * 0.18f);
            if (riseH > 0) {
                u8g2.drawBox(xL - 2, yL + 34 - riseH, 38, riseH + 2);
                u8g2.drawBox(xR - 2, yR + 34 - riseH, 38, riseH + 2);
            }
            u8g2.setDrawColor(1);
        }
    }

    // 3. Cat Muzzle, Inverted Nose, and Whiskers with Breathing Motion
    u8g2.setDrawColor(1);
    int16_t noseY = 46 + (int16_t)(m_breathOffset * 0.4f);
    u8g2.drawTriangle(61, noseY, 67, noseY, 64, noseY + 3);
    u8g2.drawLine(64, noseY + 3, 64, noseY + 5);
    u8g2.drawLine(64, noseY + 5, 59, noseY + 7);
    u8g2.drawLine(64, noseY + 5, 69, noseY + 7);

    int16_t wSway = (int16_t)(sinf(millis() * 0.003f) * 1.0f);
    u8g2.drawLine(xL - 2, 44 + wSway, xL - 15, 41);
    u8g2.drawLine(xL - 2, 48,         xL - 16, 49 - wSway);
    u8g2.drawLine(xR + 35, 44 - wSway, xR + 48, 41);
    u8g2.drawLine(xR + 35, 48,         xR + 49, 49 + wSway);
}

void RobotEyes::drawOwlEyes(U8G2& u8g2, float pOffX, float pOffY, float blinkProgress) {
    u8g2.setDrawColor(1);

    int16_t cxL = 36;
    int16_t cyL = 33 + (int16_t)m_offsetY;
    int16_t cxR = 92;
    int16_t cyR = 33 + (int16_t)m_offsetY;

    // 1. Owl Feather Horns (Plumicorns) with curiosity angle
    int16_t hornTilt = (int16_t)(pOffX * 0.3f);
    u8g2.drawTriangle(22, cyL - 19, 30, cyL - 21, 12 + hornTilt, cyL - 32);
    u8g2.drawTriangle(20, cyL - 19, 26, cyL - 20, 10 + hornTilt, cyL - 30);
    u8g2.drawTriangle(106, cyR - 19, 98, cyR - 21, 116 + hornTilt, cyR - 32);
    u8g2.drawTriangle(108, cyR - 19, 102, cyR - 20, 118 + hornTilt, cyR - 30);

    // 2. Brow Ridge V connecting above the beak
    u8g2.drawLine(34, cyL - 19, 64, cyL - 9);
    u8g2.drawLine(34, cyL - 18, 64, cyL - 8);
    u8g2.drawLine(94, cyR - 19, 64, cyR - 9);
    u8g2.drawLine(94, cyR - 18, 64, cyR - 8);

    // 3. Concentric Feathery Orbital Disc Rings
    u8g2.drawCircle(cxL, cyL, 22);
    u8g2.drawCircle(cxL, cyL, 20);
    u8g2.drawCircle(cxR, cyR, 22);
    u8g2.drawCircle(cxR, cyR, 20);

    u8g2.drawPixel(cxL - 22, cyL);
    u8g2.drawPixel(cxL + 22, cyL);
    u8g2.drawPixel(cxL, cyL - 22);
    u8g2.drawPixel(cxL, cyL + 22);
    u8g2.drawPixel(cxR - 22, cyR);
    u8g2.drawPixel(cxR + 22, cyR);
    u8g2.drawPixel(cxR, cyR - 22);
    u8g2.drawPixel(cxR, cyR + 22);

    // 4. Owl Eyes or Sleeping Closed Seams
    if (blinkProgress <= 0.08f) {
        u8g2.drawLine(cxL - 14, cyL + 4, cxL + 14, cyL + 4);
        u8g2.drawLine(cxL - 10, cyL + 5, cxL + 10, cyL + 5);
        u8g2.drawLine(cxR - 14, cyR + 4, cxR + 14, cyR + 4);
        u8g2.drawLine(cxR - 10, cyR + 5, cxR + 10, cyR + 5);
    } else {
        u8g2.drawFilledEllipse(cxL, cyL, 17, 17);
        u8g2.drawFilledEllipse(cxR, cyR, 17, 17);

        u8g2.setDrawColor(0);
        int16_t pxL = cxL + (int16_t)constrain(pOffX * 0.65f, -6.0f, 6.0f);
        int16_t pyL = cyL + (int16_t)constrain(pOffY * 0.65f, -5.0f, 5.0f);
        int16_t pxR = cxR + (int16_t)constrain(pOffX * 0.65f, -6.0f, 6.0f);
        int16_t pyR = cyR + (int16_t)constrain(pOffY * 0.65f, -5.0f, 5.0f);

        u8g2.drawFilledEllipse(pxL, pyL, 10, 10);
        u8g2.drawFilledEllipse(pxR, pyR, 10, 10);

        u8g2.setDrawColor(1);
        u8g2.drawDisc(pxL + 3, pyL - 3, 2);
        u8g2.drawDisc(pxR + 3, pyR - 3, 2);
        u8g2.drawPixel(pxL + 5, pyL + 1);
        u8g2.drawPixel(pxR + 5, pyR + 1);

        if (blinkProgress < 0.98f) {
            u8g2.setDrawColor(0);
            int16_t dropH = (int16_t)(34.0f * (1.0f - blinkProgress));
            if (dropH > 0) {
                u8g2.drawBox(cxL - 18, cyL - 18, 36, dropH);
                u8g2.drawBox(cxR - 18, cyR - 18, 36, dropH);
            }
            u8g2.setDrawColor(1);
        }
    }

    // 5. Hooked Center Owl Beak
    u8g2.setDrawColor(1);
    u8g2.drawTriangle(60, cyL - 7, 68, cyL - 7, 64, cyL + 9);
    u8g2.setDrawColor(0);
    u8g2.drawLine(64, cyL - 6, 64, cyL + 8);
    u8g2.drawPixel(62, cyL - 5);
    u8g2.drawPixel(66, cyL - 5);
    u8g2.setDrawColor(1);
}

void RobotEyes::draw(U8G2& u8g2) {
    constexpr int16_t BASE_X = 21;
    constexpr int16_t BASE_Y = 16;

    float totalOffsetX = m_offsetX + m_tremorX;
    float totalOffsetY = m_offsetY + m_tremorY + m_breathOffset;

    // 3D Perspective Squash & Stretch
    float effectiveLW = m_leftWidth  - m_squashX;
    float effectiveRW = m_rightWidth + m_squashX;
    if (effectiveLW < 24.0f) effectiveLW = 24.0f;
    if (effectiveRW < 24.0f) effectiveRW = 24.0f;

    float effectiveLH = m_leftHeight  * m_breathScale;
    float effectiveRH = m_rightHeight * m_breathScale;

    int16_t xL = BASE_X + (int16_t)totalOffsetX;
    int16_t xR = BASE_X + (int16_t)effectiveLW + EYE_SPACING + (int16_t)totalOffsetX;

    float moodLiftL = (m_mood == EyeMood::SKEPTICAL) ? -3.0f : 0.0f;
    float moodLiftR = (m_mood == EyeMood::SKEPTICAL) ?  2.0f : 0.0f;

    // Stable, anchored eye position - no collapsing down the screen!
    int16_t yL = BASE_Y + (int16_t)(totalOffsetY + moodLiftL);
    int16_t yR = BASE_Y + (int16_t)(totalOffsetY + moodLiftR);

    // Dedicated Full Facial Renderers for Cat and Owl
    if (m_mood == EyeMood::CAT) {
        drawCatEyes(u8g2, xL, yL, xR, yR, m_pupilOffsetX, m_pupilOffsetY, m_blinkProgress);
        return;
    }
    if (m_mood == EyeMood::OWL) {
        drawOwlEyes(u8g2, m_pupilOffsetX, m_pupilOffsetY, m_blinkProgress);
        return;
    }

    // 1. Render Eye Bodies with biological sweeping eyelid masks
    if (m_mood == EyeMood::LOVE) {
        float pulse = 1.0f + (sinf(m_heartPulsePhase) * 0.10f);
        drawHeartEye(u8g2, xL + (int16_t)(effectiveLW / 2), yL + 16, pulse);
        drawHeartEye(u8g2, xR + (int16_t)(effectiveRW / 2), yR + 16, pulse);
    } else if (m_mood == EyeMood::WINK) {
        drawSingleEye(u8g2, xL, yL, effectiveLW, effectiveLH, m_pupilWidth, m_pupilHeight, m_pupilOffsetX, m_pupilOffsetY, m_blinkProgress, true);
        drawWinkEye(u8g2, xR + (int16_t)(effectiveRW / 2), yR + 18, effectiveRW);
    } else {
        drawSingleEye(u8g2, xL, yL, effectiveLW, effectiveLH, m_pupilWidth, m_pupilHeight, m_pupilOffsetX, m_pupilOffsetY, m_blinkProgress, true);
        drawSingleEye(u8g2, xR, yR, effectiveRW, effectiveRH, m_pupilWidth, m_pupilHeight, m_pupilOffsetX, m_pupilOffsetY, m_blinkProgress, false);
    }

    // 2. Render Articulated 3-Point Curved Eyebrows
    float browMotionY = m_browInertiaY + m_breathOffset + m_browBounce;

    // Left Eyebrow
    int16_t lInX  = xL + (int16_t)effectiveLW + 2;
    int16_t lMidX = xL + (int16_t)(effectiveLW * 0.45f);
    int16_t lOutX = xL - 3;
    int16_t lInY  = constrain((int16_t)(m_leftBrowInnerY + browMotionY + m_browCuriosityTwitch), 0, 22);
    int16_t lMidY = constrain((int16_t)(m_leftBrowMidY   + browMotionY + m_browCuriosityTwitch), 0, 22);
    int16_t lOutY = constrain((int16_t)(m_leftBrowOuterY + browMotionY + m_browCuriosityTwitch), 0, 22);
    drawCurvedEyebrow(u8g2, lInX, lInY, lMidX, lMidY, lOutX, lOutY, true);

    // Right Eyebrow
    int16_t rInX  = xR - 2;
    int16_t rMidX = xR + (int16_t)(effectiveRW * 0.55f);
    int16_t rOutX = xR + (int16_t)effectiveRW + 3;
    int16_t rInY  = constrain((int16_t)(m_rightBrowInnerY + browMotionY), 0, 22);
    int16_t rMidY = constrain((int16_t)(m_rightBrowMidY   + browMotionY), 0, 22);
    int16_t rOutY = constrain((int16_t)(m_rightBrowOuterY + browMotionY), 0, 22);
    drawCurvedEyebrow(u8g2, rInX, rInY, rMidX, rMidY, rOutX, rOutY, false);

    // 3. Render Mood Cutouts & Smiling Cheeks
    if (m_blinkProgress > 0.4f && m_mood == EyeMood::HAPPY) {
        u8g2.setDrawColor(0);
        int16_t cutH = (int16_t)(effectiveLH * 0.35f);
        u8g2.drawBox(xL - 2, yL + (int16_t)effectiveLH - cutH, (int16_t)effectiveLW + 4, cutH + 2);
        u8g2.drawBox(xR - 2, yR + (int16_t)effectiveRH - cutH, (int16_t)effectiveRW + 4, cutH + 2);

        u8g2.setDrawColor(1);
        u8g2.drawRBox(xL, yL, (int16_t)effectiveLW, (int16_t)(effectiveLH - cutH + 3), 8);
        u8g2.drawRBox(xR, yR, (int16_t)effectiveRW, (int16_t)(effectiveRH - cutH + 3), 8);
    }
}
