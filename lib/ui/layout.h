// GÉNÉRÉ par design/tools/gen.py — ne pas éditer à la main.
// Source de vérité : design/tokens.json, design/tools/art_*.py
#pragma once
#include <cstdint>

namespace ui {
namespace layout {

constexpr int kScreenW = 240;
constexpr int kScreenH = 135;

constexpr int kCabH          = 94;
constexpr int kCabW          = 184;
constexpr int kCabX          = 22;
constexpr int kCabY          = 19;
constexpr int kHole          = 6;
constexpr int kHudH          = 17;
constexpr int kLampCount     = 12;
constexpr int kLampSize      = 4;
constexpr int kLampStep      = 14;
constexpr int kLampX0        = 36;
constexpr int kLampY         = 22;
constexpr int kLeverBaseY    = 101;
constexpr int kLeverCx       = 223;
constexpr int kLeverTop      = 24;
constexpr int kLeverTravel   = 40;
constexpr int kMsgY          = 115;
constexpr int kPaylineY      = 66;
constexpr int kPitch         = 48;
constexpr int kSym           = 48;
constexpr int kSymScale      = 3;
constexpr int kSymY          = 42;
constexpr int kWinGap        = 6;
constexpr int kWinH          = 74;
constexpr int kWinW          = 48;
constexpr int kWinX0         = 36;
constexpr int kWinY          = 29;

constexpr int winX(int i) { return kWinX0 + i * (kWinW + kWinGap); }
constexpr int symX(int i) { return winX(i) + (kWinW - kSym) / 2; }

}  // namespace layout

// Format vidéo 5x3 — zéro chrome : ni cabinet ni hublot, le HUD
// est en surimpression et les lignes sont signalées par des
// chevrons latéraux. Le levier garde sa colonne à droite.
namespace vlayout {

constexpr int kCell        = 32;
constexpr int kCols        = 5;
constexpr int kGap         = 3;
constexpr int kGridH       = 102;
constexpr int kGridW       = 172;
constexpr int kGridX       = 16;
constexpr int kGridY       = 18;
constexpr int kLeverBaseY  = 106;
constexpr int kLeverCx     = 214;
constexpr int kLeverTop    = 30;
constexpr int kLeverTravel = 34;
constexpr int kMsgY        = 121;
constexpr int kRows        = 3;
constexpr int kScale       = 2;

constexpr int cellX(int c) { return kGridX + c * (kCell + kGap); }
constexpr int cellY(int r) { return kGridY + r * (kCell + kGap); }

}  // namespace vlayout

// Blackjack — mêmes règles de chrome : rien que les cartes,
// les totaux et le choix d'action.
namespace bjlayout {

constexpr int kActionsY        = 110;
constexpr int kCardH           = 40;
constexpr int kCardStep        = 31;
constexpr int kCardStepTight   = 20;
constexpr int kCardW           = 28;
constexpr int kDealerY         = 20;
constexpr int kHandX           = 24;
constexpr int kPlayerY         = 66;

}  // namespace bjlayout
}  // namespace ui
