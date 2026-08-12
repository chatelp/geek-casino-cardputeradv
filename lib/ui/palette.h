// GÉNÉRÉ par design/tools/gen.py — ne pas éditer à la main.
// Source de vérité : design/tokens.json, design/tools/art_*.py
#pragma once
#include <cstdint>

namespace ui {
namespace pal {
// Couleurs RGB565. Typées uint16_t : un littéral nu serait lu RGB888.
constexpr uint16_t ink900     = 0x0021;  // #000408  Fond le plus profond — la nuit d'arcade
constexpr uint16_t ink800     = 0x0863;  // #080C18  Panneau, fond des hublots de rouleaux
constexpr uint16_t ink700     = 0x10C5;  // #101829  Surface surélevée, alternance de bandes
constexpr uint16_t ink600     = 0x2128;  // #212442  Bordure sourde, élément inactif
constexpr uint16_t steel500   = 0x4A8F;  // #4A517B  Métal sombre — corps de puce, châssis
constexpr uint16_t steel300   = 0x8C97;  // #8C92BD  Métal clair — broches, volet de disquette
constexpr uint16_t white      = 0xFFFF;  // #FFFFFF  Texte principal, éclat maximal
constexpr uint16_t cyan       = 0x275E;  // #21EBF7  Néon primaire — cadre, d20, silicium
constexpr uint16_t cyanDk     = 0x0C52;  // #088A94  Ombre du cyan
constexpr uint16_t magenta    = 0xF9F7;  // #FF3CBD  Néon secondaire — ligne de paiement, accent
constexpr uint16_t magentaDk  = 0xA10D;  // #A5206B  Ombre du magenta
constexpr uint16_t yellow     = 0xFE85;  // #FFD329  Crédits, gains, LED de cabinet
constexpr uint16_t amber      = 0xC440;  // #C68A00  Ombre du jaune
constexpr uint16_t green      = 0x6FE7;  // #6BFF39  Vert acide — l'invader, le jackpot
constexpr uint16_t greenDk    = 0x2CE3;  // #299E18  Ombre du vert, phosphore CRT
constexpr uint16_t orange     = 0xFBC3;  // #FF7918  Chaleur, étincelles, LED témoin
constexpr uint16_t violet     = 0xA2FF;  // #A55DFF  Néon tertiaire — disquette, halo
constexpr uint16_t violetDk   = 0x5973;  // #5A2C9C  Ombre du violet
constexpr uint16_t red        = 0xF969;  // #FF2C4A  Alerte, LED rouge, solde bas
constexpr uint16_t tan        = 0xEE11;  // #EFC38C  Corps de résistance — seule teinte non néon
constexpr uint16_t pcb        = 0x0962;  // #082C10  Surface de carte — le tapis de blackjack EST un circuit
constexpr uint16_t pcbLine    = 0x1AE6;  // #185D31  Piste et sérigraphie sur la carte
constexpr uint16_t pcbEdge    = 0x3C6A;  // #398E52  Bord de carte, contours de sérigraphie

}  // namespace pal
}  // namespace ui
