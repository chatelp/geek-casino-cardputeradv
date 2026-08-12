#include "painter.h"

#include "palette.h"

namespace ui {

namespace P = pal;

namespace {
inline const uint8_t* glyphOf(char c) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    if (c < 32 || c > 126) c = ' ';
    return kFont5x7[c - 32];
}
}  // namespace

int textWidth(const char* s, int scale) {
    int n = 0;
    while (s[n]) ++n;
    return n == 0 ? 0 : (n * kFontAdvance - 1) * scale;
}

void drawText(lgfx::LGFX_Sprite& g, const char* s, int x, int y, uint16_t color,
              int scale, Align a) {
    const int w = textWidth(s, scale);
    if (a == Align::Center) x -= w / 2;
    else if (a == Align::Right) x -= w;

    for (int i = 0; s[i]; ++i) {
        const uint8_t* rows = glyphOf(s[i]);
        const int gx = x + i * kFontAdvance * scale;
        for (int ry = 0; ry < kFontH; ++ry) {
            const uint8_t bits = rows[ry];
            int c = 0;
            while (c < kFontW) {
                if (!(bits & (1 << (kFontW - 1 - c)))) { ++c; continue; }
                int run = 1;  // fusionne les pixels voisins : moins d'appels
                while (c + run < kFontW && (bits & (1 << (kFontW - 1 - c - run)))) ++run;
                g.fillRect(gx + c * scale, y + ry * scale, run * scale, scale, color);
                c += run;
            }
        }
    }
}

int numberWidth(int32_t v, int scale) {
    int digits = 1;
    for (int32_t t = v < 0 ? -v : v; t >= 10; t /= 10) ++digits;
    if (v < 0) ++digits;
    return (digits * kFontAdvance - 1) * scale;
}

void drawNumber(lgfx::LGFX_Sprite& g, int32_t v, int x, int y, uint16_t color,
                int scale, Align a) {
    char buf[13];
    int n = 0;
    const bool neg = v < 0;
    uint32_t t = neg ? static_cast<uint32_t>(-v) : static_cast<uint32_t>(v);
    do { buf[n++] = static_cast<char>('0' + (t % 10)); t /= 10; } while (t);
    if (neg) buf[n++] = '-';
    char s[13];
    for (int i = 0; i < n; ++i) s[i] = buf[n - 1 - i];
    s[n] = '\0';
    drawText(g, s, x, y, color, scale, a);
}

namespace {
void blitIndexed(lgfx::LGFX_Sprite& g, const uint8_t* art, int px, int x, int y,
                 int scale, uint16_t tint, const uint16_t* pal) {
    for (int ry = 0; ry < px; ++ry) {
        int c = 0;
        while (c < px) {
            const uint8_t idx = art[ry * px + c];
            if (idx == 0) { ++c; continue; }  // 0 = transparent
            int run = 1;
            while (c + run < px && art[ry * px + c + run] == idx) ++run;
            const uint16_t col = tint ? tint : pal[idx];
            g.fillRect(x + c * scale, y + ry * scale, run * scale, scale, col);
            c += run;
        }
    }
}
}  // namespace

void drawSymbol(lgfx::LGFX_Sprite& g, uint8_t sym, int x, int y, int scale,
                uint16_t tint, bool classic) {
    if (sym >= kSymbolCount) return;
    blitIndexed(g, classic ? kSymbolsClassic[sym] : kSymbols[sym], kSymbolPx,
                x, y, scale, tint, kSymbolPalette);
}

void drawSymbolGray(lgfx::LGFX_Sprite& g, uint8_t sym, int x, int y, int scale,
                    bool classic) {
    if (sym >= kSymbolCount) return;
    blitIndexed(g, classic ? kSymbolsClassic[sym] : kSymbols[sym], kSymbolPx,
                x, y, scale, 0, kSymbolPaletteGray);
}

void drawIconGray(lgfx::LGFX_Sprite& g, uint8_t icon, int x, int y, int scale) {
    if (icon >= kIconCount) return;
    blitIndexed(g, kIcons[icon], kIconPx, x, y, scale, 0, kSymbolPaletteGray);
}

void drawIcon(lgfx::LGFX_Sprite& g, uint8_t icon, int x, int y, int scale,
              uint16_t tint) {
    if (icon >= kIconCount) return;
    blitIndexed(g, kIcons[icon], kIconPx, x, y, scale, tint, kSymbolPalette);
}

void drawChevronV(lgfx::LGFX_Sprite& g, int x, int y, bool down, uint16_t color) {
    for (int k = 0; k < 3; ++k) {
        const int row = down ? y + k : y + 2 - k;
        g.fillRect(x + k, row, 1, 1, color);
        g.fillRect(x + 4 - k, row, 1, 1, color);
    }
}

void trace45(lgfx::LGFX_Sprite& g, int x0, int y0, int x1, int y1,
             uint16_t color, int w) {
    const int dx = x1 - x0, dy = y1 - y0;
    const int sx = dx >= 0 ? 1 : -1, sy = dy >= 0 ? 1 : -1;
    const int adx = dx >= 0 ? dx : -dx, ady = dy >= 0 ? dy : -dy;
    const int diag = adx < ady ? adx : ady;

    // Segment diagonal d'abord, puis le reste tout droit.
    int x = x0, y = y0;
    for (int i = 0; i < diag; ++i) {
        g.fillRect(x, y, w, w, color);
        x += sx;
        y += sy;
    }
    if (adx > ady) g.fillRect(x < x1 ? x : x1, y, adx - ady + w, w, color);
    else if (ady > adx) g.fillRect(x, y < y1 ? y : y1, w, ady - adx + w, color);
    g.fillRect(x1, y1, w, w, color);
}

void drawVia(lgfx::LGFX_Sprite& g, int x, int y, uint16_t ring, uint16_t hole) {
    g.fillRect(x, y, 3, 3, ring);
    g.fillRect(x + 1, y + 1, 1, 1, hole);
}

namespace {
void blitCardBack(lgfx::LGFX_Sprite& g, int x, int y, int scale, const uint16_t* pal) {
    for (int ry = 0; ry < kCardBackH; ++ry) {
        int c = 0;
        while (c < kCardBackW) {
            const uint8_t idx = kCardBack[ry * kCardBackW + c];
            if (idx == 0) { ++c; continue; }
            int run = 1;
            while (c + run < kCardBackW &&
                   kCardBack[ry * kCardBackW + c + run] == idx) ++run;
            g.fillRect(x + c * scale, y + ry * scale, run * scale, scale, pal[idx]);
            c += run;
        }
    }
}
}  // namespace

void drawCardBackGray(lgfx::LGFX_Sprite& g, int x, int y, int scale) {
    blitCardBack(g, x, y, scale, kSymbolPaletteGray);
}

void drawCardBack(lgfx::LGFX_Sprite& g, int x, int y, int scale) {
    blitCardBack(g, x, y, scale, kSymbolPalette);
}

void blitSuit(lgfx::LGFX_Sprite& g, uint8_t suit, int x, int y, int scale,
              uint16_t tint) {
    if (suit >= kSuitCount) return;
    blitIndexed(g, kSuits[suit], kSuitPx, x, y, scale, tint, kSymbolPalette);
}

void drawBetArrows(lgfx::LGFX_Sprite& g, int leftX, int rightX, int y,
                   uint16_t color, bool canLower, bool canRaise) {
    for (int k = 0; k < 3; ++k) {
        if (canLower) g.fillRect(leftX + 2 - k, y + 3 - k, 1, 1 + 2 * k, color);
        if (canRaise) g.fillRect(rightX + k, y + 3 - k, 1, 1 + 2 * k, color);
    }
}

void drawFrame(lgfx::LGFX_Sprite& g, int x, int y, int w, int h, uint16_t c, int t) {
    g.fillRect(x, y, w, t, c);
    g.fillRect(x, y + h - t, w, t, c);
    g.fillRect(x, y + t, t, h - 2 * t, c);
    g.fillRect(x + w - t, y + t, t, h - 2 * t, c);
}

void desaturate(lgfx::LGFX_Sprite& g) {
    auto* px = static_cast<uint16_t*>(g.getBuffer());
    if (!px) return;
    const int n = g.width() * g.height();
    for (int i = 0; i < n; ++i) {
        // Le sprite 16 bits stocke le RGB565 OCTETS INVERSÉS — c'est le
        // format que veut l'écran, et c'est le même piège que celui déjà
        // noté pour readRect. Lire le mot tel quel donnait un écran magenta
        // uniforme, pas un gris. Mesuré sur capture le 2026-08-12.
        const uint16_t c = __builtin_bswap16(px[i]);
        // RGB565 ramené en 8 bits par canal, puis luminance perçue
        // (coefficients Rec.601 en entiers : 77/150/29 sur 256). Une moyenne
        // simple ferait passer le vert du circuit pour un gris trop sombre.
        // Extension 5/6 bits vers 8 par recopie des bits de poids fort :
        // exact aux extrémités, sans division — 32 400 pixels par image, ce
        // n'est pas l'endroit où en faire trois.
        const uint32_t r5 = (c >> 11) & 0x1F;
        const uint32_t g6 = (c >> 5) & 0x3F;
        const uint32_t b5 = c & 0x1F;
        const uint32_t r = (r5 << 3) | (r5 >> 2);
        const uint32_t gr = (g6 << 2) | (g6 >> 4);
        const uint32_t b = (b5 << 3) | (b5 >> 2);
        const uint32_t l = (77u * r + 150u * gr + 29u * b) >> 8;
        const uint16_t grey = static_cast<uint16_t>(
            ((l >> 3) << 11) | ((l >> 2) << 5) | (l >> 3));
        px[i] = __builtin_bswap16(grey);
    }
}

void drawScope(lgfx::LGFX_Sprite& g, int x, int y, int w, int h,
               const core::ScopeDrive& d, uint32_t now) {
    if (w <= 0 || h < 6) return;
    const int mid = y + h / 2;
    const int halfH = h / 2 - 1;

    // L'écran de la sonde : fond d'encre, réticule discret. Sans grille,
    // une trace flotte ; avec, elle est POSÉE sur un instrument.
    g.fillRect(x, y, w, h, P::ink900);
    for (int gx = x; gx < x + w; gx += 24) {
        g.fillRect(gx, y + 1, 1, h - 2, P::ink700);
    }
    g.fillRect(x, mid, w, 1, P::ink700);

    const int shock = core::scopeShock(now, d);
    int prev = mid;
    for (int i = 0; i < w; ++i) {
        const int v = core::scopeAt(i, w, now, d);
        int yy = mid - (v * halfH) / 100;
        if (yy < y) yy = y;
        if (yy > y + h - 1) yy = y + h - 1;
        // Pendant une secousse, la trace décroche : elle passe au magenta
        // sur TOUTE sa largeur. La couleur dit l'événement, la déchirure
        // dit sa violence — un seul des deux signaux se raterait.
        const uint16_t c = shock > 25 ? P::magenta : P::cyan;
        const int a = yy < prev ? yy : prev;
        const int b = yy > prev ? yy : prev;
        g.fillRect(x + i, a, 1, b - a + 1, c);
        prev = yy;
    }
}

}  // namespace ui
