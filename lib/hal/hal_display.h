// Frontière d'affichage : tout le rendu passe par l'API lgfx::, jamais
// par le matériel. M5GFX côté appareil, LovyanGFX côté simulateur.
#pragma once

#if defined(SIM_BUILD)
  #define LGFX_USE_V1
  #include <LovyanGFX.hpp>
#else
  #include <M5GFX.h>
#endif

namespace hal {
constexpr int kScreenW = 240;
constexpr int kScreenH = 135;
}
