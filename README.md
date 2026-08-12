# Geek Casino — machine à sous pour M5Stack Cardputer ADV

Jeu de casino hors-ligne pour **M5Stack Cardputer ADV** (ESP32-S3, écran
240 × 135, clavier 56 touches, IMU BMI270, haut-parleur 1 W).

> **Jetons virtuels uniquement.** Pas d'argent réel, pas d'achat, pas de
> compte, pas de réseau — ni WiFi ni BLE. C'est un jouet et un exercice
> d'animation, pas un produit de jeu d'argent. Le joueur ne peut jamais
> être définitivement ruiné : la machine renfloue.

Nom de code provisoire. Le premier module est une machine à sous à trois
rouleaux ; l'architecture prévoit d'autres jeux.

## État

Cinq jeux tournent sur l'appareil : machine à sous 3 rouleaux, machine
vidéo 5x3 à 5 lignes, blackjack, video poker, roulette. Accueil, animation,
son PCM, levier au geste (secouer l'appareil), aide, réglages,
multi-joueurs avec classement persistant en NVS.

- [x] Trois environnements PlatformIO (firmware, simulateur, tests)
- [x] Aléa injecté, testé, sans biais de modulo
- [x] Design system : palette, fonte 5×7, 8 glyphes, écrans
- [x] Bandes de rouleaux, table de gains, économie — **RTP exact 95,24 %**
- [x] Animation des rouleaux, machine à états, mode démo (gratuit)
- [x] Son 800–2600 Hz (contrainte testée), geste IMU, NVS signée
- [x] Accueil, aide (H), réglages globaux et par jeu (S), classement (L)
- [x] Deux habillages de rouleaux : geek et classique — mêmes gains
- [x] Blackjack — règles complètes testées, **RTP 95,81 %**
- [x] Format vidéo 5x3, 5 lignes — **RTP par ligne 94,95 %**
- [x] Écrans des quatre jeux, aides paginées et réglages dédiés
- [x] Video poker Jacks or Better 9/6 — classement vérifié sur les
      2 598 960 mains possibles
- [x] Roulette européenne — toutes les mises à **97,3 %** exactement

## Construire

```bash
pio run -e sim && .pio/build/sim/program   # simulateur macOS (SDL2)
pio test -e test-native                     # tests unitaires
pio run -e cardputer-adv -t upload          # firmware
```

Le simulateur rend le même écran que l'appareil via LovyanGFX + SDL2 :
tout le rendu passe par l'API `lgfx::`, jamais par le matériel. Il sait
aussi produire des captures déterministes, sans fenêtre :

```bash
.pio/build/sim/program --shot captures         # une image
.pio/build/sim/program --frames captures 300   # une suite, pour un GIF
```

Dans la fenêtre : **espace/Entrée** valide ou tire, **←/→/↑/↓**
naviguent, **H** aide, **S** réglages (globaux à l'accueil, du jeu en
jeu), **L** classement, **Échap** revient (et quitte depuis l'accueil).
Sur l'appareil, les flèches sont `,` `/` `;` `.` et retour est `` ` `` ;
secouer l'appareil tire le levier. Sans geste pendant douze secondes en
jeu, le mode démo prend la main — sans toucher aux jetons.

## Design system

La direction artistique — pixel-art néon, palette « nuit d'arcade »,
glyphes issus de l'univers maker et geek — vit dans un projet
claude.ai/design et se régénère depuis ce dépôt :

```bash
python3 design/tools/gen.py
```

Une source de vérité unique (`design/tokens.json`, `design/tools/art_*.py`)
produit à la fois les cartes du design system et les en-têtes C++
`lib/ui/palette.h`, `symbols.h`, `font5x7.h`. **Ces trois fichiers sont
générés : les corriger à la main ne sert à rien**, ils sont écrasés.

## Structure

```
lib/core/   logique pure C++17, sans Arduino ni M5 — testée en natif
lib/hal/    frontières matérielles : écran, touches, aléa
lib/ui/     rendu, uniquement via lgfx::
src/        main appareil et main simulateur, triés par #ifdef
design/     source de vérité de l'identité visuelle
test/       tests Unity de lib/core
```

Les décisions de conception sont journalisées dans
[docs/DECISIONS.md](docs/DECISIONS.md), les contraintes et pièges
matériels mesurés dans [CLAUDE.md](CLAUDE.md).

## Licence

[MIT](LICENSE) — prenez, apprenez, construisez dessus.

Contrairement à mon autre projet Cardputer, la licence couvre ici **aussi
l'identité visuelle** : palette, fonte 5×7 et glyphes sont dans le dépôt
et sont libres au même titre que le code. Il n'y a pas de marque à
préserver derrière ce nom de code.

Les bibliothèques tierces (M5Unified, M5GFX, LovyanGFX) restent sous leurs
licences respectives.
