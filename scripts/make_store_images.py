#!/usr/bin/env python3
"""Images de publication (M5Burner, posts) pour Silicon Casino.

La vignette du catalogue M5Burner est une boîte 4:3 d'environ 170x135 px :
tout doit survivre à une réduction x7. D'où l'affiche : le nom en très
grand dans la fonte 5x7 du projet, l'invader au centre, une ligne de
sous-titre — rien d'autre. Leçon du catalogue (relevé Daoa, confirmé) :
sur une étagère saturée, l'image lisible gagne.

Tout sort des MÊMES sources que l'appareil : la fonte et l'invader
viennent de design/tools/art_*.py, les écrans de captures/screens, le GIF
de captures/gif (mode --frames). Aucune maquette redessinée.

    pio run -e sim
    .pio/build/sim/program --screens captures/screens
    .pio/build/sim/program --frames captures/gif 170
    python3 scripts/make_store_images.py

Sortie : docs/m5burner/ (cover, mosaïque) et docs/images/spin.gif.
"""
import json
import os
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "design/tools"))
import art_font      # noqa: E402
import art_symbols   # noqa: E402

OUT = ROOT / "docs/m5burner"
SHOTS = ROOT / "captures/screens"
GIF_SRC = ROOT / "captures/gif"
OUT.mkdir(parents=True, exist_ok=True)

W, H = 1200, 900

TOKENS = json.load(open(ROOT / "design/tokens.json"))
PAL = {k: tuple(int(v["hex"][i:i + 2], 16) for i in (1, 3, 5))
       for k, v in TOKENS["palette"].items()}
KEYS = {k: PAL[v] for k, v in TOKENS["artKeys"].items()}


def text5x7(draw_px, s, x, y, scale, colour):
    """La fonte du projet, au pixel près — pas une police système."""
    for ch in s:
        g = art_font.GLYPHS.get(ch, art_font.GLYPHS[" "])
        for r, row in enumerate(g):
            for c, cell in enumerate(row):
                if cell != ".":
                    draw_px(x + c * scale, y + r * scale, scale, colour)
        x += art_font.ADVANCE * scale
    return x


def text_width(s, scale):
    return (len(s) * art_font.ADVANCE - 1) * scale


def blit_art(draw_px, art, x, y, scale):
    for r, row in enumerate(art):
        for c, ch in enumerate(row):
            if ch != ".":
                draw_px(x + c * scale, y + r * scale, scale, KEYS[ch])


def make_cover():
    img = Image.new("RGB", (W, H), PAL["ink900"])

    def px(x, y, s, colour):
        for dy in range(s):
            for dx in range(s):
                if 0 <= x + dx < W and 0 <= y + dy < H:
                    img.putpixel((x + dx, y + dy), colour)

    # Le nom, comme au boot : SILICON magenta, CASINO cyan. Échelle 20 →
    # des lettres de 140 px, encore nettes à la réduction x7 du catalogue.
    s1, s2 = "SILICON", "CASINO"
    sc = 20
    text5x7(px, s1, (W - text_width(s1, sc)) // 2, 130, sc, PAL["magenta"])
    text5x7(px, s2, (W - text_width(s2, sc)) // 2, 300, sc, PAL["cyan"])

    # L'invader — le jackpot — au centre, encadré de deux plus petits.
    inv = [x for x in art_symbols.SYMBOLS if x["id"] == "INVADER"][0]["art"]
    blit_art(px, inv, (W - 16 * 22) // 2, 480, 22)
    blit_art(px, inv, (W - 16 * 22) // 2 - 280, 530, 10)
    blit_art(px, inv, (W - 16 * 22) // 2 + 16 * 22 + 120, 530, 10)

    # Deux lignes de silence en bas : ce que c'est, et la règle.
    l1 = "SLOTS+BLACKJACK+POKER+ROULETTE"
    l2 = "VIRTUAL CHIPS ONLY"
    text5x7(px, l1, (W - text_width(l1, 5)) // 2, 810, 5, PAL["steel300"])
    text5x7(px, l2, (W - text_width(l2, 5)) // 2, 858, 5, PAL["yellow"])

    img.save(OUT / "01-cover.png")
    print("01-cover.png        1200x900")


# La mosaïque Reddit : six écrans réels, 3x2, à l'échelle entière x2.
MOSAIC = [["lobby", "slot_spin", "roulette"],
          ["blackjack", "poker", "topup"]]


def make_mosaic():
    gap, sc = 8, 2
    tw, th = 240 * sc, 135 * sc
    cols, rows = len(MOSAIC[0]), len(MOSAIC)
    img = Image.new("RGB", (cols * tw + (cols - 1) * gap,
                            rows * th + (rows - 1) * gap), PAL["ink900"])
    for r, row in enumerate(MOSAIC):
        for c, name in enumerate(row):
            tile = Image.open(SHOTS / f"{name}.bmp").resize(
                (tw, th), Image.NEAREST)
            img.paste(tile, (c * (tw + gap), r * (th + gap)))
    img.save(OUT / "02-mosaic.png")
    print("02-mosaic.png       %dx%d" % img.size)


def make_gif():
    frames = sorted(GIF_SRC.glob("frame_*.bmp"))
    if not frames:
        sys.exit("pas de frames : .pio/build/sim/program --frames captures/gif 170")
    sc = 2
    imgs = [Image.open(f).resize((240 * sc, 135 * sc), Image.NEAREST)
            .quantize(64) for f in frames]
    # 33 ms par image comme l'appareil ; le GIF arrondit à 30 — fidèle.
    imgs[0].save(ROOT / "docs/images/spin.gif", save_all=True,
                 append_images=imgs[1:], duration=33, loop=0, optimize=True)
    size = os.path.getsize(ROOT / "docs/images/spin.gif") // 1024
    print("spin.gif            %d frames, %d Ko" % (len(imgs), size))


if __name__ == "__main__":
    make_cover()
    make_mosaic()
    make_gif()
