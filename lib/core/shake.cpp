#include "shake.h"

#include <cmath>

namespace core {

bool feedAccel(ShakeDetector& d, float magnitudeG, uint32_t now) {
    const float dev = std::fabs(magnitudeG - 1.0f);

    if (!d.primed) {  // premier échantillon : on observe, on ne déclenche pas
        d.primed = true;
        d.armed = dev < kShakeRearmG;
        return false;
    }

    if (!d.armed) {
        if (dev < kShakeRearmG) d.armed = true;
        return false;
    }

    if (dev >= kShakeTriggerG) {
        d.armed = false;
        if (now - d.lastTrigger < kShakeCooldownMs && d.lastTrigger != 0) return false;
        d.lastTrigger = now;
        return true;
    }
    return false;
}

}  // namespace core
