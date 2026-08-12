#!/usr/bin/env python3
"""Génère le design system (cartes HTML) ET le code de rendu (en-têtes C++)
depuis une source de vérité unique : tokens.json + art_symbols.py + art_font.py.

    python3 design/tools/gen.py

Sorties :
    design/build/**.html   cartes poussées vers claude.ai/design
    lib/ui/palette.h       couleurs quantifiées RGB565
    lib/ui/symbols.h       glyphes indexés 16x16
    lib/ui/font5x7.h       fonte bitmap

Rien de ce qui est généré ne doit être édité à la main.
"""

import json
import os
import struct
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
DESIGN = os.path.dirname(HERE)
ROOT = os.path.dirname(DESIGN)
BUILD = os.path.join(DESIGN, "build")

sys.path.insert(0, HERE)
from art_symbols import SYMBOLS, CLASSIC_SYMBOLS, SUITS, ICONS, CARD_BACK  # noqa: E402
from art_font import GLYPHS, W as FW, H as FH, ADVANCE  # noqa: E402

TOK = json.load(open(os.path.join(DESIGN, "tokens.json")))
PAL = TOK["palette"]
KEYS = TOK["artKeys"]
SCREEN_W, SCREEN_H = TOK["screen"]["w"], TOK["screen"]["h"]


# ------------------------------------------------------- captures réelles
# Le design system a dérivé : ses maquettes étaient redessinées en Python
# et ne montraient plus ce que l'appareil affiche. On embarque désormais
# les CAPTURES du simulateur — même code de rendu que le firmware, donc
# aucune dérive possible.
SHOTS = os.path.join(ROOT, "captures", "screens")


def bmp_to_png_data_uri(path):
    """BMP 24 bits → PNG, en data URI. Écrit en pur Python pour ne
    dépendre d'aucun outil externe : la génération doit marcher partout."""
    with open(path, "rb") as f:
        raw = f.read()
    off = struct.unpack_from("<I", raw, 10)[0]
    w, h = struct.unpack_from("<ii", raw, 18)
    rowb = ((w * 3 + 3) // 4) * 4
    rows = []
    for y in range(h - 1, -1, -1):          # le BMP stocke de bas en haut
        base = off + y * rowb
        line = bytearray(b"\x00")           # filtre PNG « None »
        for x in range(w):
            b, g, r = raw[base + x * 3: base + x * 3 + 3]
            line += bytes((r, g, b))
        rows.append(bytes(line))
    idat = zlib.compress(b"".join(rows), 9)

    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data +
                struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    png = (b"\x89PNG\r\n\x1a\n" +
           chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) +
           chunk(b"IDAT", idat) + chunk(b"IEND", b""))
    import base64
    return "data:image/png;base64," + base64.b64encode(png).decode()


def shot(name, caption, scale=3):
    """Une capture de l'appareil, agrandie sans lissage."""
    path = os.path.join(SHOTS, name + ".bmp")
    if not os.path.exists(path):
        return ('<div class="cap">capture manquante : %s — lancer '
                '<code>.pio/build/sim/program --screens captures/screens</code>'
                '</div>' % name)
    return ('<div><div class="dev"><img src="%s" width="%d" '
            'style="image-rendering:pixelated;display:block"></div>'
            '<div class="cap">%s</div></div>'
            % (bmp_to_png_data_uri(path), SCREEN_W * scale, caption))


# --------------------------------------------------------------- couleurs
def to565(hex_str):
    r, g, b = (int(hex_str[i:i + 2], 16) for i in (1, 3, 5))
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def from565(v):
    r5, g6, b5 = (v >> 11) & 0x1F, (v >> 5) & 0x3F, v & 0x1F
    r = (r5 << 3) | (r5 >> 2)
    g = (g6 << 2) | (g6 >> 4)
    b = (b5 << 3) | (b5 >> 2)
    return "#%02X%02X%02X" % (r, g, b)


# Couleur RÉELLE après quantification RGB565 : c'est elle qu'on affiche
# partout, jamais la valeur d'origine. Le design system doit montrer ce que
# l'appareil affichera, pas ce qu'on a souhaité.
for name, entry in PAL.items():
    entry["v565"] = to565(entry["hex"])
    entry["real"] = from565(entry["v565"])

C = {name: entry["real"] for name, entry in PAL.items()}
ART = {k: C[v] for k, v in KEYS.items()}


# ------------------------------------------------------------- validation
def validate():
    errs = []
    for s in SYMBOLS:
        art = s["art"]
        if len(art) != 16:
            errs.append("%s : %d lignes au lieu de 16" % (s["id"], len(art)))
        for i, row in enumerate(art):
            if len(row) != 16:
                errs.append("%s ligne %d : %d colonnes au lieu de 16 (%r)"
                            % (s["id"], i, len(row), row))
            for ch in row:
                if ch != "." and ch not in KEYS:
                    errs.append("%s ligne %d : clé inconnue %r" % (s["id"], i, ch))
    if len(CLASSIC_SYMBOLS) != len(SYMBOLS):
        errs.append("il faut exactement un glyphe classique par glyphe geek")
    for ci, s_ in enumerate(CLASSIC_SYMBOLS):
        if len(s_["art"]) != 16:
            errs.append("%s : %d lignes au lieu de 16" % (s_["id"], len(s_["art"])))
        for i, row in enumerate(s_["art"]):
            if len(row) != 16:
                errs.append("%s ligne %d : %d colonnes au lieu de 16" % (s_["id"], i, len(row)))
            for ch in row:
                if ch != "." and ch not in KEYS:
                    errs.append("%s ligne %d : clé inconnue %r" % (s_["id"], i, ch))
        if ci < len(SYMBOLS) and s_["maps"] != SYMBOLS[ci]["id"]:
            errs.append("%s (rang %d) devrait correspondre à %s"
                        % (s_["id"], ci, SYMBOLS[ci]["id"]))
    for iid, ic in ICONS.items():
        n = ic["size"]
        if len(ic["art"]) != n:
            errs.append("icone %s : %d lignes au lieu de %d" % (iid, len(ic["art"]), n))
        for i, row in enumerate(ic["art"]):
            if len(row) != n:
                errs.append("icone %s ligne %d : %d colonnes au lieu de %d"
                            % (iid, i, len(row), n))
            for ch in row:
                if ch != "." and ch not in KEYS:
                    errs.append("icone %s ligne %d : clé inconnue %r" % (iid, i, ch))
    for i, row in enumerate(CARD_BACK):
        if len(row) != 14:
            errs.append("dos de carte ligne %d : %d colonnes au lieu de 14" % (i, len(row)))
        for ch in row:
            if ch != "." and ch not in KEYS:
                errs.append("dos de carte ligne %d : clé inconnue %r" % (i, ch))
    for ch, g in GLYPHS.items():
        if len(g) != FH:
            errs.append("glyphe %r : %d lignes au lieu de %d" % (ch, len(g), FH))
        for i, row in enumerate(g):
            if len(row) != FW:
                errs.append("glyphe %r ligne %d : %d colonnes au lieu de %d (%r)"
                            % (ch, i, len(row), FW, row))
            if set(row) - set(".#"):
                errs.append("glyphe %r ligne %d : caractères hors '.#'" % (ch, i))
    if errs:
        print("Art invalide :", file=sys.stderr)
        for e in errs:
            print("  -", e, file=sys.stderr)
        sys.exit(1)


# -------------------------------------------------------------- peinture
_CLIP_SEQ = [0]


class Paint:
    """Accumule des rectangles en coordonnées écran (pixels appareil)."""

    def __init__(self):
        self.out = []

    def clipped(self, x, y, w, h, fn):
        """Découpe tout ce que `fn` dessine à un rectangle. Indispensable pour
        les rouleaux : un symbole qui défile déborde forcément du hublot."""
        _CLIP_SEQ[0] += 1
        cid = "clip%d" % _CLIP_SEQ[0]
        start = len(self.out)
        fn()
        inner = "".join(self.out[start:])
        del self.out[start:]
        self.out.append(
            '<defs><clipPath id="%s"><rect x="%g" y="%g" width="%g" height="%g"/>'
            '</clipPath></defs><g clip-path="url(#%s)">%s</g>'
            % (cid, x, y, w, h, cid, inner))

    def rect(self, x, y, w, h, color):
        if w <= 0 or h <= 0:
            return
        self.out.append('<rect x="%g" y="%g" width="%g" height="%g" fill="%s"/>'
                        % (x, y, w, h, color))

    def frame(self, x, y, w, h, color, t=1):
        self.rect(x, y, w, t, color)
        self.rect(x, y + h - t, w, t, color)
        self.rect(x, y + t, t, h - 2 * t, color)
        self.rect(x + w - t, y + t, t, h - 2 * t, color)

    def art(self, art, ox, oy, scale=1, override=None, alpha=None):
        """Pose un pixel-art. `override` remplace toutes les couleurs opaques
        (utile pour une silhouette de gain qui clignote en blanc)."""
        for ry, row in enumerate(art):
            rx = 0
            while rx < len(row):
                ch = row[rx]
                if ch == ".":
                    rx += 1
                    continue
                run = 1
                while rx + run < len(row) and row[rx + run] == ch:
                    run += 1
                col = override or ART[ch]
                r = ('<rect x="%g" y="%g" width="%g" height="%g" fill="%s"%s/>'
                     % (ox + rx * scale, oy + ry * scale, run * scale, scale, col,
                        '' if alpha is None else ' opacity="%g"' % alpha))
                self.out.append(r)
                rx += run

    def text(self, s, x, y, color, scale=1, align="left"):
        w = text_w(s, scale)
        if align == "center":
            x -= w / 2
        elif align == "right":
            x -= w
        for i, ch in enumerate(s.upper() if ch_upper(s) else s):
            g = GLYPHS.get(ch) or GLYPHS.get(ch.upper()) or GLYPHS[" "]
            self.art([r.replace("#", "w") for r in g],
                     x + i * ADVANCE * scale, y, scale, override=color)
        return w

    def svg(self, w, h, bg=None, cls=""):
        body = ('<rect width="%d" height="%d" fill="%s"/>' % (w, h, bg)) if bg else ""
        return ('<svg class="%s" viewBox="0 0 %d %d" width="%d" height="%d" '
                'shape-rendering="crispEdges" xmlns="http://www.w3.org/2000/svg">'
                '%s%s</svg>' % (cls, w, h, w, h, body, "".join(self.out)))


def ch_upper(s):
    return not any(c in GLYPHS and c.islower() for c in s)


def text_w(s, scale=1):
    return (len(s) * ADVANCE - 1) * scale


# ---------------------------------------------------- géométrie de l'écran
# Un seul endroit définit la mise en page ; le C++ reprendra ces valeurs.
GEO = {
    "hud_h": 17,
    # Le cabinet est une carte électronique : trous de fixation, pistes, vias.
    # C'est ce qui distingue cette machine d'un cabinet de casino générique.
    "cab_x": 22, "cab_y": 19, "cab_w": 184, "cab_h": 94,
    "hole": 6,
    "lamp_y": 22, "lamp_x0": 36, "lamp_step": 14, "lamp_n": 12, "lamp_s": 4,
    # Hublot nettement plus haut que le symbole : on voit les voisins arriver.
    "win_y": 29, "win_h": 74, "win_w": 48, "win_gap": 6, "win_x0": 36,
    "sym": 48, "sym_scale": 3,
    # Le levier vit hors du cabinet, à droite — il donne un corps au geste IMU.
    # Socle aligné sur le bas du cabinet : le levier est monté sur la machine,
    # il ne flotte pas à côté.
    "lever_cx": 223, "lever_top": 24, "lever_base_y": 101, "lever_travel": 40,
    "msg_y": 115,
}
GEO["payline_y"] = GEO["win_y"] + GEO["win_h"] // 2
GEO["sym_y"] = GEO["win_y"] + (GEO["win_h"] - GEO["sym"]) // 2
GEO["pitch"] = GEO["sym"]  # les symboles se suivent sans jeu sur la bande


def win_x(i):
    return GEO["win_x0"] + i * (GEO["win_w"] + GEO["win_gap"])


def sym_x(i):
    return win_x(i) + (GEO["win_w"] - GEO["sym"]) // 2


SYM_BY_ID = {s["id"]: s for s in SYMBOLS}


def draw_hud(p, credits, bet, low=False):
    p.rect(0, 0, SCREEN_W, GEO["hud_h"], C["ink800"])
    p.rect(0, GEO["hud_h"] - 1, SCREEN_W, 1, C["ink600"])
    p.art(ICONS["COIN"]["art"], 6, 3, 1)
    p.text(str(credits), 22, 4, C["red"] if low else C["yellow"], 2)
    p.text("BET", SCREEN_W - 6 - text_w(str(bet), 2) - 8, 4, C["steel300"], 2, "right")
    p.text(str(bet), SCREEN_W - 6, 4, C["cyan"], 2, "right")


def draw_traces(p):
    """Pistes de circuit dans le vide autour du cabinet. Le fond ne raconte
    plus « rien à afficher ici » mais « vous êtes dans une machine »."""
    # Uniquement dans la bande libre à gauche du cabinet : ailleurs elles
    # passeraient sous le levier ou sous le bandeau de message.
    t, via = C["ink700"], C["ink600"]
    for (x, y, w, h) in [(4, 24, 2, 44), (4, 66, 12, 2), (14, 68, 2, 24),
                         (9, 30, 2, 24), (9, 30, 8, 2),
                         (4, 94, 2, 16), (4, 108, 10, 2),
                         (16, 34, 2, 30), (12, 100, 2, 12)]:
        p.rect(x, y, w, h, t)
    for (x, y) in [(3, 65), (13, 91), (15, 29), (3, 107), (11, 111)]:
        p.rect(x, y, 4, 4, via)


def draw_cabinet(p, lamp_on=None, frame_color=None, board=None):
    g = GEO
    fc = frame_color or C["cyanDk"]
    p.rect(g["cab_x"], g["cab_y"], g["cab_w"], g["cab_h"], board or C["ink800"])
    p.frame(g["cab_x"], g["cab_y"], g["cab_w"], g["cab_h"], fc, 2)
    # Trous de fixation aux quatre angles — bague métal, perçage noir.
    hs = g["hole"]
    for (hx, hy) in [(g["cab_x"] + 3, g["cab_y"] + 3),
                     (g["cab_x"] + g["cab_w"] - 3 - hs, g["cab_y"] + 3),
                     (g["cab_x"] + 3, g["cab_y"] + g["cab_h"] - 3 - hs),
                     (g["cab_x"] + g["cab_w"] - 3 - hs, g["cab_y"] + g["cab_h"] - 3 - hs)]:
        p.rect(hx, hy, hs, hs, C["steel500"])
        p.rect(hx + 2, hy + 2, hs - 4, hs - 4, C["ink900"])
    for i in range(g["lamp_n"]):
        on = lamp_on(i) if lamp_on else (i % 2 == 0)
        col = C["yellow"] if on else C["ink600"]
        p.rect(g["lamp_x0"] + i * g["lamp_step"], g["lamp_y"], g["lamp_s"], g["lamp_s"], col)
    # Pistes sortant de chaque hublot vers le bas de la carte, avec via.
    bot = g["win_y"] + g["win_h"]
    for i in range(3):
        cx = win_x(i) + g["win_w"] // 2
        p.rect(cx - 1, bot + 1, 2, 4, C["ink600"])
        p.rect(cx - 2, bot + 5, 4, 3, C["steel500"])
        p.rect(g["cab_x"] + 8, bot + 6, g["cab_w"] - 16, 1, C["ink600"])


def draw_windows(p, highlight=None):
    g = GEO
    for i in range(3):
        p.rect(win_x(i), g["win_y"], g["win_w"], g["win_h"], C["ink900"])
        p.frame(win_x(i) - 1, g["win_y"] - 1, g["win_w"] + 2, g["win_h"] + 2, C["ink600"], 1)
        if highlight is not None and highlight(i):
            p.frame(win_x(i) - 1, g["win_y"] - 1, g["win_w"] + 2, g["win_h"] + 2, C["white"], 1)


def draw_payline(p, color=None):
    """Ligne de paiement + les deux chevrons qui la désignent — c'est le
    repère qui dit au joueur quelle rangée compte."""
    g = GEO
    col = color or C["magentaDk"]
    for i in range(3):
        p.rect(win_x(i), g["payline_y"], g["win_w"], 1, col)
    y = g["payline_y"]
    for k in range(4):
        p.rect(win_x(0) - 3 - k, y - k, 1, 1 + 2 * k, col)
        p.rect(win_x(2) + g["win_w"] + 2 + k, y - k, 1, 1 + 2 * k, col)


def draw_lever(p, pull=0.0, glow=False):
    """Levier mécanique. `pull` de 0 (haut) à 1 (tiré à fond) : c'est la
    traduction visuelle du geste de secousse."""
    g = GEO
    cx = g["lever_cx"]
    top = g["lever_top"] + int(pull * g["lever_travel"])
    base_y = g["lever_base_y"]
    p.rect(cx - 2, top + 10, 4, base_y - top - 10, C["steel500"])
    p.rect(cx - 1, top + 10, 1, base_y - top - 10, C["steel300"])
    p.art(ICONS["BALL"]["art"], cx - 6, top, 1)
    p.rect(cx - 9, base_y, 18, 3, C["ink600"])
    p.rect(cx - 11, base_y + 3, 22, 9, C["steel500"])
    p.frame(cx - 11, base_y + 3, 22, 9, C["ink900"], 1)
    p.rect(cx - 7, base_y + 6, 14, 3, C["ink800"])
    if glow:
        for k in range(3):
            p.rect(cx - 12 - k * 3, top + 4, 2, 2, C["yellow"])
            p.rect(cx + 11 + k * 3, top + 4, 2, 2, C["yellow"])


def draw_symbols(p, ids, override=None):
    for i, sid in enumerate(ids):
        p.art(SYM_BY_ID[sid]["art"], sym_x(i), GEO["sym_y"], GEO["sym_scale"],
              override=override)


def draw_reel(p, i, ids, offset=0, dim=None):
    """Une colonne de symboles découpée au hublot. `ids` = (au-dessus, centre,
    en dessous) ; `offset` décale la bande pour figurer le défilement."""
    g = GEO

    def body():
        for k, sid in enumerate(ids):
            if sid:
                p.art(SYM_BY_ID[sid]["art"], sym_x(i),
                      g["sym_y"] + (k - 1) * g["pitch"] + offset, g["sym_scale"],
                      override=dim)

    p.clipped(win_x(i), g["win_y"], g["win_w"], g["win_h"], body)


def draw_msg(p, msg, color, scale=2, sub=None):
    y = GEO["msg_y"]
    p.rect(0, y, SCREEN_W, SCREEN_H - y, C["ink800"])
    p.rect(0, y, SCREEN_W, 1, C["ink600"])
    if sub:
        p.text(msg, SCREEN_W // 2, y + 4, color, scale, "center")
        p.text(sub, SCREEN_W // 2, y + 4 + FH * scale + 2, C["steel300"], 1, "center")
    else:
        p.text(msg, SCREEN_W // 2, y + (SCREEN_H - y - FH * scale) // 2, color, scale, "center")


# ----------------------------------------------------------------- écrans
def screen_idle():
    p = Paint()
    p.rect(0, 0, SCREEN_W, SCREEN_H, C["ink900"])
    draw_traces(p)
    draw_hud(p, 1250, 5)
    draw_cabinet(p)
    draw_windows(p)
    draw_payline(p)
    draw_reel(p, 0, ["CRT", "FLOPPY", "LED"])
    draw_reel(p, 1, ["RESISTOR", "CHIP", "D20"])
    draw_reel(p, 2, ["INVADER", "GAMEPAD", "CRT"])
    draw_lever(p, 0.0)
    draw_msg(p, "SHAKE TO SPIN", C["cyan"], 2)
    return p.svg(SCREEN_W, SCREEN_H)


def screen_spin():
    p = Paint()
    g = GEO
    p.rect(0, 0, SCREEN_W, SCREEN_H, C["ink900"])
    draw_traces(p)
    draw_hud(p, 1245, 5)
    draw_cabinet(p, lamp_on=lambda i: i % 3 == 1)
    draw_windows(p)
    draw_payline(p)
    # Cascade d'arrêt : rouleau 0 posé, 1 en décélération (bande décalée),
    # 2 en pleine vitesse — bandes tirées des symboles qui défilent.
    draw_reel(p, 0, ["CRT", "D20", "LED"])
    draw_reel(p, 1, ["FLOPPY", "GAMEPAD", "CHIP"], offset=18)

    def fast():
        for j, col in enumerate([C["violet"], C["cyan"], C["green"], C["yellow"],
                                 C["magenta"], C["cyan"], C["orange"], C["green"],
                                 C["violet"], C["magenta"], C["yellow"]]):
            p.rect(sym_x(2), g["win_y"] - 3 + j * 7, g["sym"], 4, col)

    p.clipped(win_x(2), g["win_y"], g["win_w"], g["win_h"], fast)
    draw_lever(p, 1.0, glow=True)
    draw_msg(p, "SPINNING", C["magenta"], 2)
    return p.svg(SCREEN_W, SCREEN_H)


def screen_win():
    p = Paint()
    p.rect(0, 0, SCREEN_W, SCREEN_H, C["ink900"])
    draw_traces(p)
    draw_hud(p, 1495, 5)
    draw_cabinet(p, lamp_on=lambda i: True, frame_color=C["yellow"])
    draw_windows(p, highlight=lambda i: True)
    draw_payline(p, C["magenta"])
    draw_reel(p, 0, ["CRT", "CHIP", "LED"])
    draw_reel(p, 1, ["D20", "CHIP", "FLOPPY"])
    draw_reel(p, 2, ["GAMEPAD", "CHIP", "RESISTOR"])
    draw_lever(p, 0.25)
    # Étincelles en gerbes depuis les angles du cabinet — jamais isolées au
    # milieu d'un bord, où elles se liraient comme des pixels morts.
    for (x, y, s, col) in [
            (16, 13, 3, C["yellow"]), (8, 21, 2, C["orange"]), (24, 6, 2, C["white"]),
            (204, 13, 3, C["yellow"]), (212, 21, 2, C["orange"]), (196, 6, 2, C["white"]),
            (15, 114, 2, C["orange"]), (6, 107, 2, C["yellow"]),
            (203, 114, 2, C["orange"]), (212, 107, 2, C["yellow"])]:
        p.rect(x, y, s, s, col)
    draw_msg(p, "WIN 100", C["yellow"], 2)
    return p.svg(SCREEN_W, SCREEN_H)


def screen_jackpot():
    p = Paint()
    g = GEO
    p.rect(0, 0, SCREEN_W, SCREEN_H, C["ink900"])
    # Déluge : le cabinet disparaît, l'écran appartient aux invaders.
    for (x, y, sc, a) in [(6, 8, 2, 0.35), (206, 14, 2, 0.35), (30, 96, 2, 0.35),
                          (186, 104, 2, 0.35), (110, 4, 2, 0.25)]:
        p.art(SYM_BY_ID["INVADER"]["art"], x, y, sc, alpha=a)
    p.rect(0, 34, SCREEN_W, 46, C["ink800"])
    p.rect(0, 34, SCREEN_W, 2, C["green"])
    p.rect(0, 78, SCREEN_W, 2, C["green"])
    p.text("JACKPOT", SCREEN_W // 2, 40, C["green"], 3, "center")
    p.text("6000", SCREEN_W // 2, 64, C["white"], 2, "center")
    for i in range(3):
        p.art(SYM_BY_ID["INVADER"]["art"], 42 + i * 54, 88, 2, override=C["green"])
    for x in range(0, SCREEN_W, 8):
        p.rect(x, 128, 4, 4, C["yellow"] if (x // 8) % 2 == 0 else C["magenta"])
    return p.svg(SCREEN_W, SCREEN_H)


def screen_broke():
    p = Paint()
    p.rect(0, 0, SCREEN_W, SCREEN_H, C["ink900"])
    draw_traces(p)
    draw_hud(p, 0, 5, low=True)
    draw_cabinet(p, lamp_on=lambda i: False, frame_color=C["ink600"])
    draw_windows(p)
    draw_payline(p, C["ink600"])
    # Machine éteinte : les symboles restent en place mais perdent leur néon.
    for i, ids in enumerate([["LED", "RESISTOR", "CRT"], ["CHIP", "LED", "FLOPPY"],
                             ["D20", "RESISTOR", "GAMEPAD"]]):
        draw_reel(p, i, ids, dim=C["ink600"])
    draw_lever(p, 0.0)
    draw_msg(p, "THE HOUSE REFILLS", C["green"], 2, sub="+500 CREDITS")
    return p.svg(SCREEN_W, SCREEN_H)


def screen_lobby():
    p = Paint()
    p.rect(0, 0, SCREEN_W, SCREEN_H, C["ink900"])
    draw_traces(p)
    p.rect(0, 0, SCREEN_W, 24, C["ink800"])
    p.rect(0, 23, SCREEN_W, 1, C["magenta"])
    p.text("GEEK CASINO", 8, 5, C["magenta"], 2)
    p.art(ICONS["COIN"]["art"], SCREEN_W - 62, 6, 1)
    p.text("1250", SCREEN_W - 46, 6, C["yellow"], 2)
    # Tout le texte est à l'échelle 2 : à l'échelle 1 (7 px) une ligne serait
    # illisible en main, y compris une mention secondaire.
    games = [("SLOTS", "INVADER", True, ["FLOPPY", "CHIP", "INVADER"]),
             ("BLACKJACK", "D20", False, None),
             ("VIDEO POKER", "GAMEPAD", False, None)]
    for i, (name, icon, live, preview) in enumerate(games):
        y = 28 + i * 28
        p.rect(6, y, SCREEN_W - 12, 26, C["ink700"] if live else C["ink800"])
        if live:
            p.frame(6, y, SCREEN_W - 12, 26, C["cyan"], 1)
            p.rect(6, y, 3, 26, C["cyan"])
        p.art(SYM_BY_ID[icon]["art"], 14, y + 5, 1,
              override=None if live else C["ink600"])
        p.text(name, 38, y + 6, C["white"] if live else C["steel500"], 2)
        if preview:
            # Aperçu de la machine dans sa propre ligne : l'écran d'accueil
            # montre le jeu au lieu de seulement le nommer.
            for k, sid in enumerate(preview):
                x = 176 + k * 18
                p.rect(x - 1, y + 4, 18, 18, C["ink900"])
                p.art(SYM_BY_ID[sid]["art"], x, y + 5, 1)
        else:
            p.text("SOON", SCREEN_W - 12, y + 6, C["ink600"], 2, "right")
    # Le jackpot en cours donne une raison de revenir à cet écran.
    p.rect(0, 112, SCREEN_W, SCREEN_H - 112, C["ink800"])
    p.rect(0, 112, SCREEN_W, 1, C["greenDk"])
    w = 16 + 4 + text_w("JACKPOT 6000", 2)
    x0 = (SCREEN_W - w) // 2
    p.art(SYM_BY_ID["INVADER"]["art"], x0, 115, 1)
    p.text("JACKPOT", x0 + 20, 117, C["green"], 2)
    p.text("6000", x0 + 20 + text_w("JACKPOT ", 2), 117, C["white"], 2)
    return p.svg(SCREEN_W, SCREEN_H)


# ------------------------------------------------------------- coquille HTML
CSS = """
:root{color-scheme:light dark;--pg:#F4F5F9;--fg:#14162A;--mut:#5A6080;--card:#FFFFFF;--line:#E1E3ED}
@media (prefers-color-scheme:dark){:root{--pg:#0A0B14;--fg:#EDEEF5;--mut:#9AA0BE;--card:#12131F;--line:#242637}}
:root[data-theme="dark"]{--pg:#0A0B14;--fg:#EDEEF5;--mut:#9AA0BE;--card:#12131F;--line:#242637}
:root[data-theme="light"]{--pg:#F4F5F9;--fg:#14162A;--mut:#5A6080;--card:#FFFFFF;--line:#E1E3ED}
*{box-sizing:border-box}
body{margin:0;padding:28px;background:var(--pg);color:var(--fg);
 font:14px/1.55 ui-sans-serif,-apple-system,"Segoe UI",Roboto,sans-serif}
h1{font-size:20px;margin:0 0 4px;letter-spacing:-.01em}
h2{font-size:13px;margin:28px 0 10px;text-transform:uppercase;letter-spacing:.09em;color:var(--mut)}
p.lede{margin:0 0 20px;color:var(--mut);max-width:62ch}
.wrap{max-width:940px;margin:0 auto}
svg{display:block;image-rendering:pixelated}
.dev{background:#07070E;padding:10px;border-radius:10px;display:inline-block;
 border:1px solid var(--line);box-shadow:0 6px 26px rgba(0,0,0,.35)}
.dev svg{width:720px;max-width:100%;height:auto}
.dev.x2 svg{width:480px}
.cap{margin-top:8px;font-size:12px;color:var(--mut)}
.grid{display:grid;gap:14px}
.g4{grid-template-columns:repeat(auto-fill,minmax(190px,1fr))}
.g3{grid-template-columns:repeat(auto-fill,minmax(260px,1fr))}
.gdev{grid-template-columns:repeat(auto-fill,minmax(480px,1fr));gap:22px}
.gdev img{max-width:100%;height:auto}
.tile{background:var(--card);border:1px solid var(--line);border-radius:10px;overflow:hidden}
.tile .sw{height:62px}
.tile .meta{padding:9px 11px}
.tile .nm{font-weight:600;font-size:13px}
.tile .rl{font-size:11.5px;color:var(--mut);margin-top:2px}
code,.mono{font:12px/1.5 ui-monospace,SFMono-Regular,Menlo,monospace}
.hex{display:flex;gap:8px;margin-top:5px;flex-wrap:wrap}
.hex span{background:var(--pg);border:1px solid var(--line);border-radius:4px;padding:1px 5px;
 font:11px/1.5 ui-monospace,Menlo,monospace}
table{border-collapse:collapse;width:100%;font-size:13px}
th,td{text-align:left;padding:7px 10px;border-bottom:1px solid var(--line);vertical-align:top}
th{font-size:11px;text-transform:uppercase;letter-spacing:.07em;color:var(--mut);font-weight:600}
.scroll{overflow-x:auto}
.sym{background:#07070E;border-radius:8px;padding:12px;display:flex;justify-content:center}
.sym svg{width:144px;height:144px}
.note{font-size:12px;color:var(--mut);margin-top:6px}
.tag{display:inline-block;font-size:10.5px;letter-spacing:.05em;text-transform:uppercase;
 padding:2px 7px;border-radius:99px;border:1px solid var(--line);color:var(--mut)}
.warn{border-left:3px solid #FF7A1A;padding:9px 13px;background:var(--card);border-radius:0 8px 8px 0;
 font-size:13px;margin:16px 0}
.row{display:flex;gap:22px;flex-wrap:wrap;align-items:flex-start}
"""


def page(path, group, name, subtitle, width, body, lede=""):
    html = ('<!-- @dsCard group="%s" name="%s" subtitle="%s" width="%d" -->\n'
            '<meta charset="utf-8">\n<title>%s — Geek Casino</title>\n'
            '<style>%s</style>\n<div class="wrap">\n<h1>%s</h1>\n'
            '<p class="lede">%s</p>\n%s\n</div>\n'
            % (group, name, subtitle, width, name, CSS, name, lede or subtitle, body))
    full = os.path.join(BUILD, path)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    open(full, "w").write(html)
    return {"path": path, "group": group, "name": name,
            "subtitle": subtitle, "viewport": {"width": width}}


def device(svg, caption, cls=""):
    return ('<div><div class="dev %s">%s</div><div class="cap">%s</div></div>'
            % (cls, svg, caption))


# ------------------------------------------------------------------ cartes
def card_palette():
    # L'ordre vient des tokens, pas d'une liste écrite ici : les trois verts
    # de circuit imprimé sont restés invisibles pendant tout le blackjack
    # parce qu'une liste figée ne connaissait pas leur nom.
    order = list(PAL.keys())
    tiles = []
    for k in order:
        e = PAL[k]
        drift = "" if e["real"].upper() == e["hex"].upper() else \
            '<span title="décalage dû à la quantification">%s → %s</span>' % (e["hex"], e["real"])
        tiles.append(
            '<div class="tile"><div class="sw" style="background:%s"></div>'
            '<div class="meta"><div class="nm">%s</div><div class="rl">%s</div>'
            '<div class="hex"><span>0x%04X</span>%s</div></div></div>'
            % (e["real"], k, e["role"], e["v565"], drift or "<span>%s</span>" % e["real"]))
    body = (
        '<div class="warn"><b>Les pastilles montrent la couleur quantifiée RGB565</b>, '
        "pas la valeur d'origine — c'est ce que l'écran affichera réellement. "
        "Là où les deux diffèrent, la flèche indique la dérive.</div>"
        '<h2>Nuit d\'arcade — ' + str(len(order)) + ' teintes</h2>'
        '<div class="grid g4">' + "".join(tiles) + '</div>'
        '<h2>Règle d\'emploi</h2>'
        "<p>Le fond reste dans la famille <code>ink</code> : les néons ne servent "
        "qu'aux symboles, aux cadres et aux effets. Un écran où tout brille n'a plus "
        "de jackpot possible. Les couleurs traversent le code en "
        "<code>constexpr uint16_t</code> nommées — jamais de littéral inline, "
        "un <code>uint32</code> nu serait interprété RGB888.</p>")
    return page("foundations/palette.html", "Foundations", "Palette",
                "Toute la palette, quantifiée RGB565", 720, body,
                "La palette « nuit d'arcade » : fond très sombre, néons saturés réservés "
                "aux symboles et aux effets, pour que chaque gain allume la salle.")


def card_typography():
    rows = []
    for line in ["ABCDEFGHIJKLM", "NOPQRSTUVWXYZ", "0123456789",
                 ":.,-+/x%!?><()'*"]:
        p = Paint()
        p.text(line, 0, 0, C["white"], 2)
        rows.append('<div style="margin:6px 0">%s</div>'
                    % p.svg(text_w(line, 2), FH * 2))
    scales = []
    for sc, lbl in [(1, "x1 — 7 px : sous le seuil lisible, réservé aux mentions"),
                    (2, "x2 — 14 px : l'échelle d'interface par défaut"),
                    (3, "x3 — 21 px : bannières WIN / JACKPOT")]:
        p = Paint()
        p.text("WIN 250", 0, 0, C["yellow"], sc)
        scales.append('<tr><td>%s</td><td>%s</td></tr>'
                      % ('<div style="background:#07070E;padding:6px;border-radius:6px;'
                         'display:inline-block">%s</div>'
                         % p.svg(text_w("WIN 250", sc), FH * sc), lbl))
    body = (
        '<h2>Jeu de caractères</h2><div class="dev" style="padding:14px">%s</div>'
        '<h2>Échelles</h2><div class="scroll"><table><tr><th>Rendu</th><th>Usage</th></tr>'
        '%s</table></div>'
        '<h2>Ce qu\'il faut savoir</h2>'
        "<p>Chasse fixe 5x7, avance de 6 px. Les chiffres sont tabulaires : un solde "
        "qui passe de 999 à 1000 ne fait pas sauter la mise en page. "
        "L'interface est en anglais, donc ASCII pur — pas d'accent à dessiner, "
        "et surtout pas les 2 Mo d'efontJA à embarquer.</p>"
        '<div class="warn"><b>Pas de glyphe « $ », volontairement.</b> Jetons virtuels '
        "uniquement : aucun signe monétaire ne doit pouvoir s'afficher. "
        "C'est un garde-fou du projet, pas un oubli de dessin.</div>"
        % ("".join(rows), "".join(scales)))
    return page("foundations/typography.html", "Foundations", "Fonte 5x7",
                "Pixel font maison, chiffres tabulaires, ASCII pur", 720, body,
                "Une fonte bitmap dessinée pour cet écran plutôt qu'une fonte importée : "
                "cohérente avec le pixel-art, et lisible à 245 ppi dès l'échelle 2.")


def card_geometry():
    p = Paint()
    g = GEO
    p.rect(0, 0, SCREEN_W, SCREEN_H, C["ink900"])
    p.rect(0, 0, SCREEN_W, g["hud_h"], C["violetDk"])
    p.rect(g["cab_x"], g["cab_y"], g["cab_w"], g["cab_h"], C["cyanDk"])
    for i in range(3):
        p.rect(win_x(i), g["win_y"], g["win_w"], g["win_h"], C["ink900"])
        p.frame(sym_x(i), g["sym_y"], g["sym"], g["sym"], C["magenta"], 1)
    p.rect(0, g["msg_y"], SCREEN_W, SCREEN_H - g["msg_y"], C["amber"])
    p.rect(g["cab_x"] + 4, g["payline_y"], g["cab_w"] - 8, 1, C["magenta"])
    p.text("HUD 18", 4, 5, C["white"], 1)
    p.text("48x48", sym_x(1) + 2, g["sym_y"] + 20, C["white"], 1)
    p.text("MESSAGE", 4, g["msg_y"] + 6, C["ink900"], 1)
    rows = "".join('<tr><td class="mono">%s</td><td class="mono">%s</td><td>%s</td></tr>' % r
                   for r in [
                       ("écran", "240 x 135", "ST7789V2, ~25 x 14 mm, ~245 ppi"),
                       ("HUD", "y 0-17", "jeton + crédits à gauche, mise à droite"),
                       ("cabinet", "x 30-209, y 20-103", "cadre 2 px, 13 lampes en haut"),
                       ("hublot", "52 x 62, pas 58", "3 hublots, x 36 / 94 / 152"),
                       ("symbole", "48 x 48", "art 16x16 a l'echelle 3"),
                       ("ligne de paiement", "y 61", "traverse le cabinet, sous les symboles"),
                       ("message", "y 107-134", "etat courant, invite, gain"),
                   ])
    body = (
        '<div class="row">%s</div>'
        '<h2>Cotes</h2><div class="scroll"><table>'
        '<tr><th>Zone</th><th>Cote</th><th>Contenu</th></tr>%s</table></div>'
        '<h2>Pourquoi 16x16 a l\'echelle 3</h2>'
        "<p>Un symbole de 48 px fait environ 5 mm sur l'écran physique. Dessiner "
        "l'art en 16x16 puis le tripler donne des pixels de 0,31 mm : assez gros "
        "pour rester lisibles à 245 ppi, là où un dessin en 48x48 natif se "
        "dissoudrait en détails invisibles. La grille grossière est un choix de "
        "lisibilité avant d'être un choix de style.</p>"
        % (device(p.svg(SCREEN_W, SCREEN_H), "Zones de l'écran de jeu — cotes en pixels appareil"),
           rows))
    return page("foundations/geometry.html", "Foundations", "Grille écran",
                "Découpe des 240 x 135 : HUD, cabinet, hublots, message", 720, body,
                "La mise en page de l'écran de jeu, cotée au pixel. Ces valeurs sont "
                "générées depuis la même source que le code de rendu.")


def card_motion():
    tiers = [
        ("Perte", "83 % des tours", "Aucun effet. Les rouleaux s'arrêtent, la ligne reste sourde.",
         "Le silence du quotidien est ce qui rend le reste énorme."),
        ("Petit gain", "2x a 9x", "Hublots gagnants cerclés de blanc, lampes clignotantes.",
         "400 ms. On note, on ne célèbre pas."),
        ("Gain moyen", "10x a 49x", "Étincelles aux angles, hublots cerclés, cadre cyan.",
         "900 ms."),
        ("Gros gain", "50x et plus", "Cadre passé à l'or, lampes toutes allumées, gerbes complètes.",
         "1,6 s. Premier moment où le cabinet perd son calme."),
        ("JACKPOT", "3 invaders", "Le cabinet disparaît. Invaders en fond, bannière verte, déluge.",
         "3 s. Le seul état qui casse la mise en page."),
    ]
    rows = "".join(
        '<tr><td><b>%s</b></td><td class="mono">%s</td><td>%s</td><td>%s</td></tr>' % t
        for t in tiers)

    # Courbe de décélération d'un rouleau : rapide, puis freinage long, puis
    # dépassement d'un cran et retour — le « clac » mécanique.
    p = Paint()
    p.rect(0, 0, 320, 90, C["ink900"])
    prev = None
    for i in range(321):
        t = i / 320.0
        e = 1 - pow(1 - t, 3)
        over = 0.0
        if t > 0.82:
            k = (t - 0.82) / 0.18
            over = 0.09 * (1 - k) * pow(2.718, -4 * k) * (1 if k < 0.5 else -0.4)
        y = 78 - (e + over) * 68
        if prev is not None:
            p.rect(i, min(prev, y), 1, max(1, abs(prev - y)), C["cyan"])
        prev = y
    p.rect(0, 78, 320, 1, C["ink600"])
    p.rect(0, 10, 320, 1, C["ink600"])
    p.text("STOP", 292, 2, C["steel300"], 1)
    p.text("SPIN", 2, 80, C["steel300"], 1)

    body = (
        '<h2>Escalade selon le gain</h2>'
        '<div class="scroll"><table><tr><th>Palier</th><th>Déclencheur</th>'
        '<th>Effet</th><th>Durée / intention</th></tr>%s</table></div>'
        '<h2>Arrêt d\'un rouleau</h2>'
        '<div class="row"><div><div class="dev">%s</div>'
        '<div class="cap">Position du rouleau dans le temps — décélération cubique, '
        'dépassement d\'un cran, retour.</div></div>'
        '<div style="max-width:34ch"><p>Le dépassement suivi du retour est ce qui donne '
        'du poids : sans lui, le rouleau « se pose », avec lui il « claque ». Le son '
        "d'arrêt tombe sur le pic de dépassement, pas sur l'immobilisation.</p>"
        "<p>Les trois rouleaux s'arrêtent en cascade, jamais ensemble : "
        "l'attente sur le dernier est le seul suspense que la machine possède.</p></div></div>"
        '<h2>Flou de vitesse</h2>'
        "<p>Au-delà de 1,2 symbole par image, un rouleau ne peut plus montrer "
        "de détail à 30 images par seconde : afficher les glyphes ne donne pas "
        "de la vitesse, ça donne du scintillement. Le rouleau bascule alors sur "
        "des bandes de la couleur dominante de chaque symbole, qui défilent "
        "environ cinq fois moins vite que le rouleau réel. C'est un mensonge "
        "assumé : l'œil lit « très vite » et n'a aucun moyen de compter.</p>"
        '<h2>Cadence</h2>'
        "<p>Cible 30 images par seconde. Tout est dessiné dans un sprite plein écran "
        "puis poussé d'un bloc — aucun effet ne doit dépendre d'un dessin direct à "
        "l'écran, qui scintillerait.</p>"
        % (rows, p.svg(320, 90)))
    return page("foundations/motion.html", "Foundations", "Animation",
                "Paliers d'effets selon le gain, courbe d'arrêt des rouleaux", 720, body,
                "L'animation escalade avec le gain. La sobriété des tours ordinaires est "
                "ce qui rend le jackpot spectaculaire.")


def card_symbols():
    tiles = []
    for i, s in enumerate(SYMBOLS):
        p = Paint()
        p.rect(0, 0, 16, 16, C["ink900"])
        p.art(s["art"], 0, 0, 1)
        jack = s["id"] == "INVADER"
        tiles.append(
            '<div class="tile"><div class="sym">%s</div><div class="meta">'
            '<div class="nm">%s <span class="tag">%s</span></div>'
            '<div class="rl">%s</div><div class="note">%s</div>'
            '<div class="hex"><span>index %d</span><span>%s</span></div>'
            '</div></div>'
            % (p.svg(16, 16), s["label"], "JACKPOT" if jack else "rang %d" % (i + 1),
               s["family"], s["note"], i, s["id"]))
    icons = []
    for iid, ic in ICONS.items():
        p = Paint()
        p.rect(0, 0, ic["size"], ic["size"], C["ink900"])
        p.art(ic["art"], 0, 0, 1)
        icons.append('<div class="tile"><div class="sym">%s</div>'
                     '<div class="meta"><div class="nm">%s</div>'
                     '<div class="rl">%d x %d — interface</div></div></div>'
                     % (p.svg(ic["size"], ic["size"]), ic["label"], ic["size"], ic["size"]))
    # Bande de rouleau : les symboles tels qu'ils défilent.
    strip = Paint()
    strip.rect(0, 0, 16, 16 * len(SYMBOLS), C["ink900"])
    for i, s in enumerate(SYMBOLS):
        strip.art(s["art"], 0, i * 16, 1)
    sheet = Paint()
    sheet.rect(0, 0, 16 * len(SYMBOLS) + 2 * (len(SYMBOLS) + 1), 20, C["ink900"])
    for i, s in enumerate(SYMBOLS):
        sheet.art(s["art"], 2 + i * 18, 2, 1)
    body = (
        '<h2>Planche-contact</h2><div class="dev" style="padding:8px">%s</div>'
        '<div class="cap">Les 8 glyphes à la taille de dessin (16x16). '
        "Un symbole qui ne se lit pas ici ne se lira pas sur l'appareil.</div>"
        '<h2>Les 8 glyphes, du plus faible au jackpot</h2>'
        '<div class="grid g3">%s</div>'
        '<h2>Bande de rouleau</h2>'
        '<div class="row"><div class="dev" style="padding:8px">'
        '<svg viewBox="0 0 16 %d" width="96" height="%d" shape-rendering="crispEdges" '
        'xmlns="http://www.w3.org/2000/svg">%s</svg></div>'
        '<div style="max-width:40ch"><p>L\'ordre ci-dessus est l\'ordre de valeur, '
        "et sert d'index de symbole dans le code. La bande réelle de chaque rouleau "
        "répétera ces glyphes avec des pondérations différentes — c'est elle qui fixe "
        "le taux de retour, pas la table de gains seule.</p>"
        "<p>Trois familles se partagent le jeu : l'électronique maker (résistance, LED, "
        "puce), le rétro-computing et gaming (disquette, manette, CRT), et la geek pop "
        "(dé 20, invader). Aucun fruit, aucune cloche, aucun bar.</p></div></div>"
        '<h2>Pictogrammes d\'interface</h2><div class="grid g4">%s</div>'
        % (sheet.svg(16 * len(SYMBOLS) + 2 * (len(SYMBOLS) + 1), 20),
           "".join(tiles), 16 * len(SYMBOLS), 16 * len(SYMBOLS) * 6,
           "".join(strip.out), "".join(icons)))
    return page("symbols/reel-set.html", "Symbols", "Glyphes des rouleaux",
                "8 symboles pixel 16x16 — maker, rétro-computing, geek pop", 720, body,
                "Le vocabulaire de la machine. Chaque glyphe est dessiné sur une grille "
                "16x16 puis affiché à l'échelle 3, soit 48 px sur l'appareil.")


def card_equivalence():
    rows = []
    pt3 = [8, 12, 20, 50, 100, 250, 400, 1200]  # miroir de paytable.cpp
    for i, (gk, cl) in enumerate(zip(SYMBOLS, CLASSIC_SYMBOLS)):
        pg, pc = Paint(), Paint()
        pg.rect(0, 0, 16, 16, C["ink900"]); pg.art(gk["art"], 0, 0, 1)
        pc.rect(0, 0, 16, 16, C["ink900"]); pc.art(cl["art"], 0, 0, 1)
        rows.append(
            '<tr><td>%s</td><td><b>%s</b></td><td>%s</td><td><b>%s</b></td>'
            '<td class="mono">x%d</td></tr>'
            % (pg.svg(16, 16, cls="eq"), gk["label"], pc.svg(16, 16, cls="eq"),
               cl["label"], pt3[i]))
    body = (
        '<style>.eq{width:48px;height:48px}</style>'
        '<h2>Geek et classique — même machine, deux habillages</h2>'
        '<div class="scroll"><table>'
        '<tr><th>Geek</th><th></th><th>Classique</th><th></th><th>3 en ligne</th></tr>'
        '%s</table></div>'
        "<p>Le réglage « GLYPHS » du jeu (touche S en jeu) bascule l'habillage "
        "sans toucher aux probabilités : même index, même bande, même gain. "
        "La page d'aide (touche H) montre cette table sur l'appareil.</p>"
        "<p>Deux identiques en tête de ligne : x2, quel que soit le symbole.</p>"
        % "".join(rows))
    return page("symbols/equivalence.html", "Symbols", "Correspondance classique",
                "Cerises, citron, cloche, BAR, 7... rang pour rang", 720, body,
                "Chaque glyphe geek a son équivalent de machine traditionnelle, "
                "au même rang de valeur. C'est la page d'aide du jeu.")


def _lever(p, cx, top, base_y, pull=0.0):
    """Le levier, dessiné hors grille. Sert à vérifier qu'un format laisse
    vraiment la place au geste plutôt qu'à l'affirmer."""
    t = top + int(pull * 30)
    p.rect(cx - 2, t + 10, 4, base_y - t - 10, C["steel500"])
    p.rect(cx - 1, t + 10, 1, base_y - t - 10, C["steel300"])
    p.art(ICONS["BALL"]["art"], cx - 6, t, 1)
    p.rect(cx - 9, base_y, 18, 3, C["ink600"])
    p.rect(cx - 11, base_y + 3, 22, 9, C["steel500"])
    p.frame(cx - 11, base_y + 3, 22, 9, C["ink900"], 1)


def _mini(cols, rows, scale, lines, label, lever=False):
    """Maquette d'un format : HUD en surimpression, zéro chrome, la grille
    seule. C'est l'état d'esprit demandé — ne garder que jetons et mise."""
    p = Paint()
    sym = 16 * scale
    gapx, gapy = (4 if scale == 3 else 3), (4 if scale == 3 else 3)
    gw = cols * sym + (cols - 1) * gapx
    gh = rows * sym + (rows - 1) * gapy
    # Avec levier, la grille se décale à gauche pour lui laisser sa colonne.
    lever_w = 46 if lever else 0
    x0 = 16 if lever else (SCREEN_W - gw) // 2
    y0 = 19 + (102 - gh) // 2
    p.rect(0, 0, SCREEN_W, SCREEN_H, C["ink900"])
    # HUD en surimpression : pas de bandeau, juste les deux chiffres.
    p.art(ICONS["COIN"]["art"], 4, 3, 1)
    p.text("1250", 20, 3, C["yellow"], 2)
    p.text("BET 5", SCREEN_W - 4, 3, C["cyan"], 2, "right")
    picks = ["FLOPPY", "CHIP", "GAMEPAD", "CRT", "D20", "LED", "RESISTOR",
             "INVADER", "CHIP", "FLOPPY", "CRT", "GAMEPAD", "LED", "D20",
             "RESISTOR"]
    for r in range(rows):
        for c in range(cols):
            x = x0 + c * (sym + gapx)
            y = y0 + r * (sym + gapy)
            p.rect(x - 1, y - 1, sym + 2, sym + 2, C["ink800"])
            p.art(SYM_BY_ID[picks[(r * cols + c) % len(picks)]]["art"], x, y, scale)
    # Lignes de paiement, en repères latéraux seulement — pas de tracé qui
    # barre les symboles.
    for r in range(min(rows, lines)):
        cy = y0 + r * (sym + gapy) + sym // 2
        for k in range(3):
            p.rect(x0 - 4 - k, cy - k, 1, 1 + 2 * k, C["magenta"])
            p.rect(x0 + gw + 3 + k, cy - k, 1, 1 + 2 * k, C["magenta"])
    if lever:
        # Quelques pistes dans la marge gauche, comme sur l'écran réel.
        for (tx, ty, tw, th) in [(4, 24, 2, 40), (4, 62, 8, 2), (10, 64, 2, 30),
                                 (4, 96, 2, 18)]:
            p.rect(tx, ty, tw, th, C["ink700"])
        p.rect(3, 61, 4, 4, C["ink600"])
        _lever(p, x0 + gw + lever_w // 2 - 2, 26, 104)
    p.text(label, SCREEN_W // 2, SCREEN_H - 11, C["steel300"], 1, "center")
    free = SCREEN_W - gw - (16 if lever else 0)
    return p.svg(SCREEN_W, SCREEN_H), gw, gh, free


def card_device():
    """Toutes les captures réelles, groupées. Cette carte remplace les
    maquettes redessinées : elle ne peut pas mentir sur ce que l'appareil
    affiche, puisqu'elle EST ce que l'appareil affiche."""
    groups = [
        ("Allumage", [
            ("boot_noise", "Bruit multicolore : la mémoire vidéo « se remplit »."),
            ("boot_bars", "Barres de couleur et déchirures de balayage."),
            ("boot_test", "Faux test mémoire, en phosphore vert."),
            ("boot_logo", "Le nom émerge du bruit résiduel."),
        ]),
        ("Accueil et navigation", [
            ("lobby", "Cinq jeux, solde commun, description du jeu pointé."),
            ("name_entry", "Saisie du nom au premier lancement."),
            ("leaderboard", "Classement — c'est la table des joueurs elle-même."),
            ("settings", "Réglages généraux : son, démo, allumage, joueur."),
        ]),
        ("Machine à sous 3x1", [
            ("slot", "Cabinet-circuit, levier à droite, hublots hauts."),
            ("slot_spin", "En plein tour : le bandeau du bas est un analyseur "
                          "de spectre, pas un mot."),
            ("slot_classic", "Même jeu, glyphes classiques (réglage GLYPHS)."),
            ("celeb_count", "Célébration : le gain se décompte."),
            ("help", "Table de gains, les deux habillages côte à côte."),
        ]),
        ("Machine vidéo 5x3", [
            ("video", "Zéro chrome : HUD en surimpression, chevrons de ligne."),
            ("video_spin", "Cinq rouleaux, donc cinq à-coups : le spectre "
                           "s'apaise à chaque verrouillage."),
            ("video_lines", "Les cinq lignes DESSINÉES — un chevron ne s'explique pas."),
            ("video_help", "Table de gains par longueur d'alignement."),
        ]),
        ("Blackjack", [
            ("bj_table", "La table EST une carte : vias, pistes à 45°, pastilles."),
            ("blackjack", "Main en cours, conseil de stratégie en point vert."),
            ("bj_help", "Règles et cartes d'exemple."),
        ]),
        ("Video poker", [
            ("poker", "Cartes 38x54 : au poker elles SONT le jeu, elles ont "
                      "donc toute la place."),
            ("poker_result", "Main conclue : la bande du bouton DRAW accueille "
                             "le nom de la main et le gain."),
            ("poker_help", "Barème 9/6 et bonus de mise maximale."),
        ]),
        ("Roulette", [
            ("roulette", "La roue en bande, montée sur encodeur rotatif."),
            ("roulette_spin", "Pari sur un plein, bille lancée."),
            ("roulette_help", "Dix paris — tous à 97,3 %."),
        ]),
        ("Mode démo", [
            ("demo_poker", "Gris intégral, cartes comprises."),
            ("demo_roulette", "Même traitement : gratuit, muet, gris."),
        ]),
    ]
    body = ('<div class="warn"><b>Ces images sont des captures réelles</b> du '
            "moteur de rendu, pas des maquettes redessinées. Le simulateur "
            "et le firmware partagent le même code d'affichage : ce que "
            "montre cette page est, au pixel près, ce que montre "
            "l'appareil.</div>")
    for title, items in groups:
        body += "<h2>%s</h2><div class=\"grid gdev\">%s</div>" % (
            title, "".join(shot(n, c, 2) for n, c in items))
    return page("screens/device.html", "Screens", "Écrans de l'appareil",
                "Captures réelles des cinq jeux, de l'allumage et de la démo",
                1040, body,
                "Ce que l'appareil affiche vraiment — capturé par le "
                "simulateur, qui partage son code de rendu avec le firmware.")


def card_formats():
    opts = [
        (3, 2, 3, 2, False, "3x2 - 2 LIGNES",
         "Symboles gardés à 48 px. Le multi-ligne sans rien sacrifier."),
        (3, 2, 3, 2, True, "3x2 + LEVIER",
         "Le levier tient, mais il mange la marge : la grille se colle à "
         "gauche et l'équilibre se perd."),
        (5, 3, 2, 3, False, "5x3 - VIDEO SLOT",
         "Symboles 32 px. Occupe vraiment la largeur."),
        (5, 3, 2, 3, True, "5x3 + LEVIER",
         "La place existe : 46 px de colonne à droite, la grille reste "
         "entière et les pistes gardent leur marge à gauche."),
        (3, 3, 2, 3, False, "3x3 - 5 LIGNES",
         "Symboles 32 px ET 100 px de vide de chaque côté. Le pire des "
         "deux mondes."),
    ]
    blocks = []
    for cols, rows, scale, lines, lever, label, note in opts:
        svg, gw, gh, free = _mini(cols, rows, scale, lines, label, lever)
        blocks.append(
            '<div style="margin-bottom:20px">%s'
            '<div class="cap"><b>%s</b> — %s<br>Grille %d x %d px. '
            'Marge restante : %d px%s.</div></div>'
            % (device(svg, ""), label, note, gw, gh, free,
               " (le levier en occupe 46)" if lever else ""))
    body = (
        '<div class="warn"><b>Constat en construisant ces maquettes :</b> sur un '
        "écran deux fois plus large que haut, grandir en <i>hauteur</i> coûte "
        "cher (il faut rapetisser les symboles) alors que grandir en "
        "<i>largeur</i> ne coûte rien. Le 3x3 est le pire des deux mondes : il "
        "impose des symboles de 32 px sans utiliser la largeur gagnée.</div>"
        "%s"
        '<h2>Le chrome</h2>'
        "<p>Toutes les maquettes ci-dessus appliquent la règle demandée : "
        "aucun cabinet, aucun hublot, aucune lampe — jetons et mise en "
        "surimpression, et les lignes de paiement signalées par des chevrons "
        "latéraux plutôt que par un trait qui barre les symboles. "
        "Le cabinet-circuit reste au format 3x1, où la place existe.</p>"
        % "".join(blocks))
    return page("screens/formats.html", "Screens", "Formats de grille",
                "3x1, 3x2, 5x3, 3x3 — comparés à l'échelle réelle", 720, body,
                "Quel format pour aller au-delà d'une ligne ? Les quatre "
                "candidats rendus à la taille réelle de l'écran.")


def card_screens():
    body = (
        '<h2>Repos</h2>%s'
        '<h2>Rotation</h2>%s'
        '<h2>Gain</h2>%s'
        '<h2>Jackpot</h2>%s'
        '<h2>Renflouement</h2>%s'
        % (device(screen_idle(), "Trois rouleaux à l'arrêt, ligne de paiement sourde, "
                                 "invite au geste IMU."),
           device(screen_spin(), "Cascade d'arrêt : rouleau 1 posé, rouleau 2 en "
                                 "décélération, rouleau 3 en flou de vitesse."),
           device(screen_win(), "Hublots cerclés, cadre passé à l'or, lampes toutes "
                                "allumées, étincelles."),
           device(screen_jackpot(), "Le cabinet disparaît : le seul état qui casse la "
                                    "mise en page."),
           device(screen_broke(), "Solde à zéro : la maison remet au pot. "
                                  "Aucun cul-de-sac possible, c'est un garde-fou du projet.")))
    return page("screens/slot-states.html", "Screens", "Machine à sous — états",
                "Repos, rotation, gain, jackpot, renflouement", 720, body,
                "Les cinq états de l'écran de jeu, rendus à la taille réelle de l'écran "
                "(240 x 135) puis agrandis x3 pour la lecture.")


def card_lobby():
    body = (
        '<h2>Écran d\'accueil</h2>'
        '<div class="grid gdev">' +
        shot("lobby", "Les cinq jeux. Le solde est commun ; la mise, elle, "
                      "appartient au couple (joueur, jeu).", 2) +
        shot("leaderboard", "Le classement n'est pas un écran de plus : c'est la "
                            "table des joueurs, affichée telle quelle.", 2) +
        '</div>'
        '<h2>Pourquoi un accueil dès la première version</h2>'
        "<p>La machine à sous était le premier module, pas le seul prévu. "
        "L'accueil existait donc dès la première version, avec une seule entrée "
        "jouable et deux entrées grisées : il fixait l'interface commune que "
        "tout jeu devrait présenter. Les cinq entrées d'aujourd'hui sont "
        "arrivées sans qu'on ait à démonter l'architecture — c'est très "
        "exactement ce que cet écran devait acheter.</p>")
    return page("screens/lobby.html", "Screens", "Accueil du casino",
                "Cinq jeux, solde commun, classement persistant", 1040, body,
                "L'écran d'accueil : ce qui fait de l'objet un casino plutôt qu'une "
                "seule machine.")


def card_cabinet():
    parts = []
    p = Paint()
    p.rect(0, 0, SCREEN_W, 118, C["ink900"])
    draw_traces(p)
    draw_cabinet(p)
    draw_windows(p)
    draw_payline(p)
    draw_lever(p, 0.0)
    parts.append(device(p.svg(SCREEN_W, 118),
                        "Cabinet vide — carte, trous de fixation, lampes, hublots, "
                        "chevrons de ligne, levier au repos."))
    p = Paint()
    p.rect(0, 0, 40, 118, C["ink900"])
    for k, (pull, lbl) in enumerate([(0.0, "repos"), (0.55, "en course"), (1.0, "tiré")]):
        q = Paint()
        q.rect(0, 0, 40, 118, C["ink900"])
        draw_lever(q, pull, glow=(pull >= 1.0))
        parts.append('<div style="display:inline-block;margin-right:12px">'
                     '<div class="dev" style="padding:6px">'
                     '<svg viewBox="%d 0 40 118" width="120" height="354" '
                     'shape-rendering="crispEdges" xmlns="http://www.w3.org/2000/svg">'
                     '%s</svg></div><div class="cap">Levier — %s</div></div>'
                     % (GEO["lever_cx"] - 20, "".join(q.out), lbl))
    p = Paint()
    p.rect(0, 0, SCREEN_W, 22, C["ink900"])
    draw_hud(p, 1250, 5)
    parts.append(device(p.svg(SCREEN_W, 22), "HUD au repos."))
    p = Paint()
    p.rect(0, 0, SCREEN_W, 22, C["ink900"])
    draw_hud(p, 0, 5, low=True)
    parts.append(device(p.svg(SCREEN_W, 22), "HUD à solde nul — le compteur passe au rouge."))
    body = ('<h2>Éléments</h2><div style="display:grid;gap:18px">%s</div>'
            '<h2>Le cabinet est une carte électronique</h2>'
            "<p>Trous de fixation aux quatre angles, pistes sortant de chaque hublot "
            "vers le bas de la carte, vias, pistes dans la marge gauche : le décor "
            "vient de l'univers maker, pas du casino générique. C'est ce qui distingue "
            "cette machine de n'importe quelle machine à sous.</p>"
            "<p>Le levier vit hors du cabinet, à droite, et se tire vraiment. Il donne "
            "un corps au geste : secouer l'appareil actionne un objet visible, pas une "
            "fonction abstraite. Les 12 lampes du bandeau sont le canal d'expression le "
            "moins coûteux de l'écran — chenillard à la rotation, toutes allumées au "
            "gain, éteintes à solde nul.</p>" % "".join(parts))
    return page("components/cabinet.html", "Components", "Cabinet & HUD",
                "Cadre, lampes, hublots, bandeau de crédits", 720, body,
                "Les pièces réutilisables de l'écran de jeu, isolées de leur contenu.")


# ------------------------------------------------------------ export C++
def cpp_header(path, body):
    full = os.path.join(ROOT, path)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    open(full, "w").write(
        "// GÉNÉRÉ par design/tools/gen.py — ne pas éditer à la main.\n"
        "// Source de vérité : design/tokens.json, design/tools/art_*.py\n"
        "#pragma once\n#include <cstdint>\n\n" + body)


def export_palette():
    lines = ["namespace ui {\nnamespace pal {\n",
             "// Couleurs RGB565. Typées uint16_t : un littéral nu serait lu RGB888.\n"]
    for k, e in PAL.items():
        lines.append("constexpr uint16_t %-10s = 0x%04X;  // %s  %s\n"
                     % (k, e["v565"], e["real"], e["role"]))
    lines.append("\n}  // namespace pal\n}  // namespace ui\n")
    cpp_header("lib/ui/palette.h", "".join(lines))


def export_symbols():
    keys = [None] + sorted(KEYS.keys())
    idx = {k: i for i, k in enumerate(keys) if k}
    out = ["namespace ui {\n\nconstexpr int kSymbolPx = 16;\nconstexpr int kSymbolCount = %d;\n\n"
           % len(SYMBOLS)]
    out.append("// Index 0 = transparent. Les autres pointent dans kSymbolPalette.\n")
    out.append("constexpr uint16_t kSymbolPalette[%d] = {\n    0x0000,\n" % len(keys))
    for k in keys[1:]:
        out.append("    0x%04X,  // %s (%s)\n" % (PAL[KEYS[k]]["v565"], k, KEYS[k]))
    out.append("};\n\nconstexpr uint8_t kSymbols[kSymbolCount][kSymbolPx * kSymbolPx] = {\n")
    for s in SYMBOLS:
        out.append("    {  // %s\n" % s["id"])
        for row in s["art"]:
            out.append("        " + ",".join("%d" % (0 if c == "." else idx[c]) for c in row) + ",\n")
        out.append("    },\n")
    out.append("};\n\n")

    out.append("// Jeu CLASSIQUE — même index, même gain, autre habillage.\n")
    out.append("constexpr uint8_t kSymbolsClassic[kSymbolCount][kSymbolPx * kSymbolPx] = {\n")
    for s in CLASSIC_SYMBOLS:
        out.append("    {  // %s\n" % s["id"])
        for row in s["art"]:
            out.append("        " + ",".join("%d" % (0 if c == "." else idx[c]) for c in row) + ",\n")
        out.append("    },\n")
    out.append("};\n\n")

    # Rampe de démo : chaque couleur mappée sur 3 gris par luminance.
    # La démo garde ainsi le volume des dessins — trois nuances, pas une
    # silhouette plate (retour Pierre, D-014).
    grays = [to565("#2E3352"), to565("#767D9E"), to565("#D2D7EE")]
    out.append("// Palette 3 gris pour le mode démo — même indexation.\n")
    out.append("constexpr uint16_t kSymbolPaletteGray[%d] = {\n    0x0000,\n" % len(keys))
    for k in keys[1:]:
        hx = PAL[KEYS[k]]["hex"]
        r, gr, b = int(hx[1:3], 16), int(hx[3:5], 16), int(hx[5:7], 16)
        lum = 0.299 * r + 0.587 * gr + 0.114 * b
        # Seuils choisis pour répartir les 20 teintes sur les 3 gris
        # plutôt que d'en tasser la moitié dans le medium.
        v = grays[2] if lum >= 125 else (grays[1] if lum >= 55 else grays[0])
        out.append("    0x%04X,  // %s lum=%d\n" % (v, k, int(lum)))
    out.append("};\n\n")
    out.append("// Enseignes de cartes 8x8 — blackjack.\n")
    out.append("constexpr int kSuitPx = 8;\nenum Suit : uint8_t {\n")
    for i, su in enumerate(SUITS):
        out.append("    SUIT_%-8s = %d,  // %s\n" % (su["id"], i, su["label"]))
    out.append("};\nconstexpr int kSuitCount = %d;\n\n" % len(SUITS))
    out.append("constexpr uint8_t kSuits[kSuitCount][kSuitPx * kSuitPx] = {\n")
    for su in SUITS:
        if len(su["art"]) != 8 or any(len(r) != 8 for r in su["art"]):
            raise SystemExit("enseigne %s : doit etre 8x8" % su["id"])
        out.append("    {  // %s\n" % su["id"])
        for row in su["art"]:
            out.append("        " + ",".join("%d" % (0 if c == "." else idx[c]) for c in row) + ",\n")
        out.append("    },\n")
    out.append("};\n")
    out.append("// Dos de carte 14x20, affiché à l'échelle 2.\n")
    out.append("constexpr int kCardBackW = 14;\nconstexpr int kCardBackH = 20;\n")
    out.append("constexpr uint8_t kCardBack[kCardBackW * kCardBackH] = {\n")
    for row in CARD_BACK:
        if len(row) != 14:
            raise SystemExit("dos de carte : %d colonnes au lieu de 14 (%r)" % (len(row), row))
        out.append("    " + ",".join("%d" % (0 if c == "." else idx[c]) for c in row) + ",\n")
    if len(CARD_BACK) != 20:
        raise SystemExit("dos de carte : %d lignes au lieu de 20" % len(CARD_BACK))
    out.append("};\n\n")
    out.append("// Les trois nuances, nommées : le chrome de la démo les utilise\n")
    out.append("// directement (indexer la rampe dépendrait de l'ordre des clés).\n")
    out.append("constexpr uint16_t kGrayDark  = 0x%04X;\n" % grays[0])
    out.append("constexpr uint16_t kGrayMid   = 0x%04X;\n" % grays[1])
    out.append("constexpr uint16_t kGrayLight = 0x%04X;\n\n" % grays[2])

    # Couleur dominante de chaque glyphe : au-delà d'une certaine vitesse, un
    # rouleau ne peut plus montrer de détail à 30 images/s — il montre une
    # bande de cette couleur. Sans ça, le défilement scintille au lieu d'aller
    # vite.
    out.append("// Couleur dominante — utilisée pour le flou de vitesse.\n")
    out.append("constexpr uint16_t kSymbolDominant[kSymbolCount] = {\n")
    for s in SYMBOLS:
        tally = {}
        for row in s["art"]:
            for ch in row:
                if ch not in (".", "k", "K"):  # le contour ne fait pas la teinte
                    tally[ch] = tally.get(ch, 0) + 1
        best = max(tally, key=tally.get)
        out.append("    0x%04X,  // %s → %s\n" % (PAL[KEYS[best]]["v565"], s["id"], best))
    out.append("};\n\n")

    # Pictogrammes d'interface : même palette indexée, tailles variables.
    ids = sorted(ICONS)
    out.append("constexpr int kIconPx = 12;\nenum Icon : uint8_t {\n")
    for i, iid in enumerate(ids):
        out.append("    ICON_%-8s = %d,  // %s\n" % (iid, i, ICONS[iid]["label"]))
    out.append("};\nconstexpr int kIconCount = %d;\n\n" % len(ids))
    out.append("constexpr uint8_t kIcons[kIconCount][kIconPx * kIconPx] = {\n")
    for iid in ids:
        ic = ICONS[iid]
        if ic["size"] != 12:
            raise SystemExit("icone %s : seules les icones 12x12 sont exportées" % iid)
        out.append("    {  // %s\n" % iid)
        for row in ic["art"]:
            out.append("        " + ",".join("%d" % (0 if c == "." else idx[c]) for c in row) + ",\n")
        out.append("    },\n")
    out.append("};\n\n}  // namespace ui\n")
    cpp_header("lib/ui/symbols.h", "".join(out))


def export_symbol_ids():
    """Le vocabulaire de symboles, côté logique pure. Aucune donnée de rendu :
    lib/core doit pouvoir se compiler et se tester sans rien connaître du
    dessin. L'équilibrage (bandes, gains) est écrit à la main dans core."""
    out = ["namespace core {\n\n",
           "// Ordre = ordre de valeur croissante. INVADER est le jackpot.\n",
           "enum Symbol : uint8_t {\n"]
    for i, s in enumerate(SYMBOLS):
        out.append("    SYM_%-9s = %d,  // %s — %s\n"
                   % (s["id"], i, s["label"], s["family"]))
    out.append("};\n\nconstexpr uint8_t kSymbolCount = %d;\n" % len(SYMBOLS))
    out.append("constexpr Symbol kJackpotSymbol = SYM_INVADER;\n\n")
    out.append("}  // namespace core\n")
    cpp_header("lib/core/symbol_ids.h", "".join(out))


def export_layout():
    """La géométrie de l'écran, du design system vers le code de rendu.
    Déplacer un hublot dans gen.py déplace le hublot dans le firmware."""
    names = {
        "hud_h": "kHudH", "cab_x": "kCabX", "cab_y": "kCabY",
        "cab_w": "kCabW", "cab_h": "kCabH", "hole": "kHole",
        "lamp_y": "kLampY", "lamp_x0": "kLampX0", "lamp_step": "kLampStep",
        "lamp_n": "kLampCount", "lamp_s": "kLampSize",
        "win_y": "kWinY", "win_h": "kWinH", "win_w": "kWinW",
        "win_gap": "kWinGap", "win_x0": "kWinX0",
        "sym": "kSym", "sym_scale": "kSymScale", "sym_y": "kSymY",
        "pitch": "kPitch", "payline_y": "kPaylineY",
        "lever_cx": "kLeverCx", "lever_top": "kLeverTop",
        "lever_base_y": "kLeverBaseY", "lever_travel": "kLeverTravel",
        "msg_y": "kMsgY",
    }
    out = ["namespace ui {\nnamespace layout {\n\n",
           "constexpr int kScreenW = %d;\n" % SCREEN_W,
           "constexpr int kScreenH = %d;\n\n" % SCREEN_H]
    for key in sorted(GEO):
        if key in names:
            out.append("constexpr int %-14s = %d;\n" % (names[key], GEO[key]))
    out.append("\nconstexpr int winX(int i) { return kWinX0 + i * (kWinW + kWinGap); }\n")
    out.append("constexpr int symX(int i) { return winX(i) + (kWinW - kSym) / 2; }\n")
    out.append("\n}  // namespace layout\n\n")

    # --- format vidéo 5x3 : chiffres vérifiés sur la carte « Formats ».
    out.append("// Format vidéo 5x3 — zéro chrome : ni cabinet ni hublot, le HUD\n")
    out.append("// est en surimpression et les lignes sont signalées par des\n")
    out.append("// chevrons latéraux. Le levier garde sa colonne à droite.\n")
    out.append("namespace vlayout {\n\n")
    v = {"kCols": 5, "kRows": 3, "kCell": 32, "kGap": 3, "kGridX": 16,
         "kGridY": 18, "kScale": 2, "kLeverCx": 214, "kLeverTop": 30,
         "kLeverBaseY": 106, "kLeverTravel": 34, "kMsgY": 121}
    v["kGridW"] = v["kCols"] * v["kCell"] + (v["kCols"] - 1) * v["kGap"]
    v["kGridH"] = v["kRows"] * v["kCell"] + (v["kRows"] - 1) * v["kGap"]
    for k in sorted(v):
        out.append("constexpr int %-12s = %d;\n" % (k, v[k]))
    out.append("\nconstexpr int cellX(int c) { return kGridX + c * (kCell + kGap); }\n")
    out.append("constexpr int cellY(int r) { return kGridY + r * (kCell + kGap); }\n")
    out.append("\n}  // namespace vlayout\n\n")

    # --- blackjack : deux rangées de cartes, HUD en surimpression.
    out.append("// Blackjack — mêmes règles de chrome : rien que les cartes,\n")
    out.append("// les totaux et le choix d'action.\n")
    out.append("namespace bjlayout {\n\n")
    b = {"kCardW": 28, "kCardH": 40, "kCardStep": 31, "kCardStepTight": 20,
         "kDealerY": 20, "kPlayerY": 66, "kActionsY": 110, "kHandX": 24}
    for k in sorted(b):
        out.append("constexpr int %-16s = %d;\n" % (k, b[k]))
    out.append("\n}  // namespace bjlayout\n\n")
    out.append("// Video poker — cinq cartes en ligne, pas assez de place\n")
    out.append("// pour l'écart du blackjack : on resserre.\n")
    out.append("namespace vplayout {\n\n")
    vp = {"kCardsY": 26, "kStep": 36, "kHeldY": 68, "kActionY": 84,
          "kMsgY": 106}
    vp["kRowW"] = 4 * vp["kStep"] + 28
    vp["kRowX"] = (SCREEN_W - vp["kRowW"]) // 2
    for k in sorted(vp):
        out.append("constexpr int %-10s = %d;\n" % (k, vp[k]))
    out.append("\nconstexpr int cardX(int i) { return kRowX + i * kStep; }\n")
    out.append("\n}  // namespace vplayout\n\n")
    out.append("// Roulette — la roue est rendue en BANDE horizontale : une\n")
    out.append("// roue ronde de 37 cases est illisible sur 240 px de large.\n")
    out.append("namespace rlayout {\n\n")
    rl = {"kCellW": 44, "kStripY": 24, "kStripH": 36, "kVisible": 5,
          "kBetY": 70, "kBetH": 28, "kMsgY": 104}
    rl["kStripX"] = (SCREEN_W - rl["kVisible"] * rl["kCellW"]) // 2
    for k in sorted(rl):
        out.append("constexpr int %-10s = %d;\n" % (k, rl[k]))
    out.append("\nconstexpr int cellLeft(int i) { return kStripX + i * kCellW; }\n")
    out.append("\n}  // namespace rlayout\n}  // namespace ui\n")
    cpp_header("lib/ui/layout.h", "".join(out))


def export_font():
    out = ["namespace ui {\n\nconstexpr int kFontW = %d;\nconstexpr int kFontH = %d;\n"
           "constexpr int kFontAdvance = %d;\n\n" % (FW, FH, ADVANCE)]
    out.append("// ASCII 32..126. Une ligne = 5 bits de poids fort à gauche.\n")
    out.append("constexpr uint8_t kFont5x7[95][%d] = {\n" % FH)
    for code in range(32, 127):
        ch = chr(code)
        g = GLYPHS.get(ch) or GLYPHS.get(ch.upper()) or GLYPHS[" "]
        bits = []
        for row in g:
            v = 0
            for i, c in enumerate(row):
                if c == "#":
                    v |= 1 << (FW - 1 - i)
            bits.append("0x%02X" % v)
        # Jamais le caractère brut en commentaire : « \ » en fin de ligne
        # continuerait le commentaire sur la suivante et mangerait un glyphe.
        safe = {" ": "espace", "\\": "antislash", "/": "slash"}.get(ch, ch)
        out.append("    {%s},  // 0x%02X %s\n" % (",".join(bits), code, safe))
    out.append("};\n\n}  // namespace ui\n")
    cpp_header("lib/ui/font5x7.h", "".join(out))


# ------------------------------------------------------------------- main
def export_overview():
    """Planche de synthèse autonome — un seul fichier à regarder pour juger
    la direction artistique d'un coup d'œil."""
    gap, cols = 14, 3
    cw, ch = SCREEN_W, SCREEN_H
    shots = [("ACCUEIL", screen_lobby()), ("REPOS", screen_idle()),
             ("ROTATION", screen_spin()), ("GAIN", screen_win()),
             ("JACKPOT", screen_jackpot()), ("RENFLOUEMENT", screen_broke())]
    rows = (len(shots) + cols - 1) // cols
    lbl = 13
    W = gap + cols * (cw + gap)
    H = 58 + rows * (ch + lbl + gap)
    out = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" '
           'height="%d" shape-rendering="crispEdges">' % (W, H, W * 3, H * 3),
           '<rect width="%d" height="%d" fill="#101018"/>' % (W, H)]
    t = Paint()
    t.text("GEEK CASINO", gap, 14, C["magenta"], 2)
    t.text("PIXEL-ART NEON - NUIT D'ARCADE - 240x135", gap, 34, C["steel300"], 1)
    out.append("".join(t.out))
    for i, (name, svg) in enumerate(shots):
        x = gap + (i % cols) * (cw + gap)
        y = 58 + (i // cols) * (ch + lbl + gap)
        lp = Paint()
        lp.text(name, x, y - 10, C["cyan"], 1)
        out.append("".join(lp.out))
        inner = svg[svg.index(">") + 1:svg.rindex("</svg>")]
        out.append('<g transform="translate(%d,%d)">%s</g>' % (x, y, inner))
    out.append("</svg>")
    path = os.path.join(ROOT, "captures", "design", "overview.svg")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, "w").write("".join(out))
    return path


def main():
    validate()
    os.makedirs(BUILD, exist_ok=True)
    cards = [card_palette(), card_typography(), card_geometry(), card_motion(),
             card_symbols(), card_equivalence(), card_cabinet(), card_screens(),
             card_lobby(), card_formats(), card_device()]
    json.dump(cards, open(os.path.join(BUILD, "cards.json"), "w"), indent=2,
              ensure_ascii=False)
    export_palette()
    export_symbols()
    export_symbol_ids()
    export_layout()
    export_font()
    print("%d cartes -> design/build/" % len(cards))
    for c in cards:
        print("   %-34s %s / %s" % (c["path"], c["group"], c["name"]))
    print("en-tetes -> lib/ui/palette.h, lib/ui/symbols.h, lib/ui/font5x7.h")
    print("planche  -> %s" % os.path.relpath(export_overview(), ROOT))


if __name__ == "__main__":
    main()
