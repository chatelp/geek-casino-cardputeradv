#!/usr/bin/env python3
"""Images du README, régénérées depuis les captures du simulateur.

Doctrine (reprise de Daoa Mini) : chaque image du README sort du
simulateur — jamais d'une maquette. La chaîne complète :

    pio run -e sim
    .pio/build/sim/program --screens captures/screens
    python3 scripts/readme_images.py

`captures/` n'est pas versionné ; `docs/images/` l'est. Ce script est la
seule passerelle entre les deux, pour que les images publiées soient
toujours reconstruisibles.

Pur Python (BMP 24 bits → PNG via zlib) : aucune dépendance à installer.
"""
import os
import struct
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "captures", "screens")
DST = os.path.join(ROOT, "docs", "images")

# Écrans publiés, et l'échelle de chacun. Échelle 2 par défaut : nette sur
# GitHub sans peser. Le héros est composé à part.
SHOTS = [
    ("boot_test", 2), ("boot_logo", 2),
    ("lobby", 2), ("about", 2), ("leaderboard", 2), ("settings", 2),
    ("slot", 2), ("slot_classic", 2), ("slot_spin", 2), ("celeb_count", 2),
    ("video", 2), ("video_lines", 2),
    ("bj_table", 2), ("blackjack", 2),
    ("poker", 2), ("poker_result", 2),
    ("roulette", 2), ("roulette_spin", 2),
    ("demo_poker", 2), ("help", 2),
]

# Le héros : quatre écrans côte à côte, deux par deux.
HERO = [["lobby", "slot"], ["roulette", "blackjack"]]
HERO_GAP = 6


def read_bmp(path):
    raw = open(path, "rb").read()
    off = struct.unpack_from("<I", raw, 10)[0]
    w, h = struct.unpack_from("<ii", raw, 18)
    rowb = ((w * 3 + 3) // 4) * 4
    px = []
    for y in range(h - 1, -1, -1):
        base = off + y * rowb
        row = []
        for x in range(w):
            b, g, r = raw[base + x * 3: base + x * 3 + 3]
            row.append((r, g, b))
        px.append(row)
    return px


def write_png(path, px, scale=1):
    h, w = len(px), len(px[0])
    rows = []
    for row in px:
        line = bytearray(b"\x00")
        for p in row:
            line += bytes(p) * scale
        for _ in range(scale):
            rows.append(bytes(line))

    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data +
                struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    png = (b"\x89PNG\r\n\x1a\n" +
           chunk(b"IHDR", struct.pack(">IIBBBBB", w * scale, h * scale,
                                      8, 2, 0, 0, 0)) +
           chunk(b"IDAT", zlib.compress(b"".join(rows), 9)) +
           chunk(b"IEND", b""))
    open(path, "wb").write(png)


def compose(grid, gap):
    tiles = [[read_bmp(os.path.join(SRC, n + ".bmp")) for n in row]
             for row in grid]
    th, tw = len(tiles[0][0]), len(tiles[0][0][0])
    out_w = len(grid[0]) * tw + (len(grid[0]) - 1) * gap
    dark = (11, 13, 26)  # fond entre les tuiles : l'encre du projet
    px = []
    for r, row in enumerate(tiles):
        if r:
            px += [[dark] * out_w for _ in range(gap)]
        for y in range(th):
            line = []
            for c, tile in enumerate(row):
                if c:
                    line += [dark] * gap
                line += tile[y]
            px.append(line)
    return px


def main():
    os.makedirs(DST, exist_ok=True)
    missing = [n for n, _ in SHOTS if not os.path.exists(
        os.path.join(SRC, n + ".bmp"))]
    if missing:
        sys.exit("captures manquantes : %s\nlancer : .pio/build/sim/program"
                 " --screens captures/screens" % ", ".join(missing))
    for name, scale in SHOTS:
        write_png(os.path.join(DST, name + ".png"),
                  read_bmp(os.path.join(SRC, name + ".bmp")), scale)
    write_png(os.path.join(DST, "hero.png"), compose(HERO, HERO_GAP), 2)
    print("%d images -> docs/images/" % (len(SHOTS) + 1))


if __name__ == "__main__":
    main()
