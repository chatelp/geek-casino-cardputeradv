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
}  // namespace ui
