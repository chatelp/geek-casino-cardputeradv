// Écrans secondaires : accueil, saisie du nom, aide, réglages, classement.
// Comme slot_screen : affiche l'état de core::App, ne décide rien.
#pragma once
#include "app.h"
#include "hal_display.h"

namespace ui {

// Point d'entrée unique : dessine l'écran que app.screen désigne.
void drawApp(lgfx::LGFX_Sprite& g, const core::App& app, uint32_t now);

}  // namespace ui
