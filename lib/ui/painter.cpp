#include "painter.h"

namespace ui {

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

void drawFrame(lgfx::LGFX_Sprite& g, int x, int y, int w, int h, uint16_t c, int t) {
    g.fillRect(x, y, w, t, c);
    g.fillRect(x, y + h - t, w, t, c);
    g.fillRect(x, y + t, t, h - 2 * t, c);
    g.fillRect(x + w - t, y + t, t, h - 2 * t, c);
}

}  // namespace ui
