// Détection de secousse — logique pure, nourrie par l'accéléromètre.
//
// Le capteur reste dans lib/hal ; ici on ne voit qu'une suite de normes
// d'accélération en g. C'est ce découpage qui permet de régler le geste
// par des tests plutôt qu'en secouant l'appareil au jugé.
#pragma once
#include <cstdint>

namespace core {

// Au repos, la norme vaut ~1 g (la pesanteur). On déclenche sur l'écart.
constexpr float kShakeTriggerG = 0.55f;  // écart qui déclenche
constexpr float kShakeRearmG = 0.22f;    // écart sous lequel on réarme
constexpr uint32_t kShakeCooldownMs = 500;

struct ShakeDetector {
    bool armed = true;
    uint32_t lastTrigger = 0;
    bool primed = false;  // un premier échantillon a été vu
};

// Renvoie vrai à l'instant précis où une secousse est reconnue.
// Hystérésis : il faut redescendre au calme avant de pouvoir redéclencher,
// sinon une seule secousse énergique lancerait plusieurs tours.
bool feedAccel(ShakeDetector& d, float magnitudeG, uint32_t now);

}  // namespace core
