*[English](README.md) · **Français***

# Silicon Casino

**Un casino de cinq jeux dans la poche — jetons virtuels uniquement.**
M5Stack Cardputer ADV · ESP32-S3 · écran 240 × 135 · entièrement hors-ligne.

![Silicon Casino — accueil, machine à sous, roulette, blackjack](docs/images/hero.png)

> ### Secouez pour lancer.

Silicon Casino est un jouet de casino hors-ligne pour le M5Stack
**Cardputer ADV** : machine à sous trois rouleaux, machine vidéo 5×3,
blackjack, video poker Jacks or Better et roulette européenne — un seul
solde, un seul classement, une seule règle : **jetons virtuels
uniquement**. Pas d'argent réel, pas d'achat, pas de compte, pas de
réseau. On ne peut jamais être ruiné pour de bon : la maison renfloue.
L'aléa vient du générateur matériel de l'ESP32.

Son identité visuelle est la machine elle-même : le cabinet de la machine
à sous est une **carte électronique** (trous de fixation, pistes, vias),
la table de blackjack est routée comme telle, la roue de la roulette est
montée sur un encodeur rotatif — et les rouleaux font tourner des glyphes
geek. Un réglage ramène les symboles classiques : l'habillage geek est un
cadeau, pas un mur.

---

## S'orienter

Tout se joue d'une main, et l'accueil dit toujours où l'on est.

| Touche | Action |
|---|---|
| `espace` / `entrée` | valider — lancer, distribuer, tirer, jouer la bille |
| **secouer l'appareil** | tire le levier : lance les rouleaux ou la bille |
| `←` `→` | changer la mise — ou déplacer le curseur dans une main |
| `↑` `↓` | naviguer à l'accueil, choisir un numéro à la roulette |
| `h` | aide du jeu pointé (paginée — les chevrons défilent) |
| `s` | réglages — globaux à l'accueil, du jeu en jeu |
| `l` | classement |
| `a` | à propos |
| `échap` | retour — marche **partout, tout le temps**, même en pleine main |
| `échap` (accueil) | quitter |

Sur l'appareil les flèches sont `,` `/` `;` `.` et retour est `` ` ``.
Une main qu'on quitte en cours n'est pas soldée : elle est gelée et vous
attend.

Après douze secondes d'inactivité (réglable), chaque jeu glisse dans un
**mode démo** gris, muet et gratuit — la boucle d'attente d'une borne
d'arcade — et rend la main au premier geste.

---

## Les écrans

| | |
|:--:|:--:|
| ![Accueil](docs/images/lobby.png) | ![Machine à sous](docs/images/slot.png) |
| **Accueil** — cinq jeux, un solde ; la mise appartient au couple (joueur, jeu) | **SLOTS** — cabinet-circuit, levier IMU dessiné à droite |
| ![En plein tour](docs/images/slot_spin.png) | ![Habillage classique](docs/images/slot_classic.png) |
| **En plein tour** — le bandeau du bas est une sonde d'oscilloscope : elle se brouille quand un rouleau se verrouille | **Réglage GLYPHS** — mêmes rouleaux, mêmes gains, symboles classiques |
| ![Machine vidéo](docs/images/video.png) | ![Lignes de gain](docs/images/video_lines.png) |
| **Machine vidéo 5×3** — cinq lignes, chevrons latéraux | **L'aide dessine les lignes** — un chevron ne s'explique pas tout seul |
| ![Blackjack](docs/images/blackjack.png) | ![Table vide](docs/images/bj_table.png) |
| **Blackjack** — 3:2, croupier reste sur tout 17, conseil de stratégie | **La table est un circuit** — vias, pistes à 45°, sérigraphie |
| ![Video poker](docs/images/poker.png) | ![Main conclue](docs/images/poker_result.png) |
| **Video poker** — Jacks or Better 9/6, cartes 38×54 : elles *sont* le jeu | **Main conclue** — la bande DRAW nomme ce que vous avez fait |
| ![Roulette](docs/images/roulette.png) | ![Bille lancée](docs/images/roulette_spin.png) |
| **Roulette européenne** — la roue en bande, sur encodeur rotatif | **Bille lancée** — elle cliquette à chaque case et rebondit avant de se poser |
| ![Célébration](docs/images/celeb_count.png) | ![Mode démo](docs/images/demo_poker.png) |
| **Gain** — panneau de célébration commun, le gain se décompte | **Mode démo** — tout passe au gris, les jetons ne bougent pas |
| ![Allumage](docs/images/boot_test.png) | ![À propos](docs/images/about.png) |
| **Allumage** — faux self-test, **chiffres vrais** (sautable, désactivable) | **À propos** — qui l'a fait, avec quoi, et ce qui ne tourne *pas* ici |

---

## Les jeux — et leurs chiffres exacts

Le taux de retour au joueur est ici un **fait calculé, pas un bouton de
réglage**. Chaque chiffre est exact — énuméré ou dérivé analytiquement —
et verrouillé par des tests natifs qui échouent si une bande ou une table
dérive.

| Jeu | Règles | RTP | Vérification |
|---|---|---|---|
| **Slots 3×1** | 8 symboles, une ligne, paire à gauche ×2 | **95,24 %** | probabilité par ligne, analytique, depuis les effectifs de bande |
| **Vidéo 5×3** | 5 lignes, bande propre (le jackpot doit rester trouvable) | **94,95 %** par ligne | même méthode — l'énumération ferait 33 M de cas |
| **Blackjack** | 3:2, croupier reste sur tout 17, double, pas de split | **95,81 %** | règles complètes testées, conseil de stratégie compris |
| **Video poker** | Jacks or Better **9/6**, royale à 800:1 en mise max | table 9/6 pleine | les **2 598 960** mains énumérées ; chaque fréquence du manuel correspond |
| **Roulette** | européenne, zéro simple, dix paris | **97,3 %** pour *chaque* pari | 37 cases × 10 paris énumérés : 972 973 ppm partout, exactement |

Le solde est commun aux jeux ; la **mise appartient au couple (joueur,
jeu)** et persiste. Renflouement : à zéro entre deux mains, la maison
remet +500 — c'est un jouet d'animation, pas une leçon sur la perte.

---

## Le matériel

| | |
|---|---|
| **MCU** | ESP32-S3 (StampS3), double cœur Xtensa LX7 @ 240 MHz |
| **Mémoire** | 8 Mo de flash, 512 Ko de SRAM, **pas de PSRAM** |
| **Écran** | ST7789, 1,14″, **240 × 135** — environ 25 × 14 mm à ~245 ppi |
| **Clavier** | 56 touches, contrôleur I²C **TCA8418** en 0x34 |
| **Audio** | haut-parleur 1 W — bande utile ≈ **800–2600 Hz**, mesurée |
| **IMU** | BMI270 — le levier « secouer pour lancer » |

Le faux self-test de l'allumage affiche ces mêmes chiffres : tous sont
vérifiés contre la définition de carte et les pilotes réellement
instanciés. Un faux écran de démarrage qui annonce du vrai matériel est
une carte d'identité ; un qui en invente n'est que du papier peint.

---

## Construire, simuler, flasher

```bash
brew install platformio sdl2
```

```bash
pio test -e test-native                     # 126 tests natifs, sans matériel
pio run -e sim && .pio/build/sim/program    # simulateur macOS, SDL, ×3
pio run -e cardputer-adv -t upload          # flash par USB
```

Tout le rendu passe par l'API `lgfx::` — jamais par le matériel — donc le
**même code** dessine sur l'appareil et sur le Mac. L'itération visuelle
se fait à la vitesse du simulateur ; l'appareil n'est flashé que pour ce
que le simulateur juge mal : couleurs réelles, lisibilité en main, son,
clavier, geste IMU.

Le simulateur produit aussi des captures déterministes :

```bash
.pio/build/sim/program --shot    <dir>      # une image
.pio/build/sim/program --frames  <dir> <n>  # une suite, pour un GIF
.pio/build/sim/program --screens <dir>      # une image par écran
python3 scripts/readme_images.py            # captures -> docs/images/
```

**Chaque image de ce README en sort.** Les cartes d'écran du design
system aussi : une maquette redessinée dérive toujours, une capture ne le
peut pas.

---

## Architecture

```
lib/core/   logique pure C++17 — zéro include Arduino/M5/lgfx, testée en
            natif. Rouleaux, tables de gains, économie, ET le mouvement :
            le rythme est de la logique, pas du dessin. Le temps entre par
            un paramètre `now`, jamais lu d'une horloge.
lib/hal/    frontières matérielles : écran (M5GFX / LovyanGFX), touches, aléa
lib/ui/     rendu, strictement via lgfx::
src/        main appareil et main simulateur, triés par #ifdef
design/     source de vérité unique de l'identité visuelle
test/       tests Unity de lib/core — 126 cas
```

`lib/ui/palette.h`, `symbols.h`, `font5x7.h`, `layout.h` sont **générés**
depuis `design/tokens.json` et `design/tools/art_*.py` : les éditer ne
sert à rien, ils sont écrasés. Le générateur valide l'art et échoue
plutôt que de produire du faux.

---

## Transparence

Ce projet est une **collaboration entre un humain et une IA, dite en
toutes lettres** : [Claude Code](https://claude.com/claude-code) est
l'agent de développement principal — architecte, implémenteur, opérateur
du simulateur, mainteneur de la documentation. Chaque ligne de C++ de ce
dépôt a été écrite par lui.

Ce n'est pas un numéro en solo. **Pierre CHATEL** est product owner,
directeur visuel, testeur et décideur final : la direction du projet, ses
garde-fous (jetons virtuels uniquement, jamais vraiment ruiné, l'identité
geek comme règle), chaque verdict visuel sur l'écran réel de l'appareil
et chaque correction de cap — « l'égaliseur n'a aucune amplitude, change
de registre », « l'arrêt de la roulette est trop franc » — sont de lui.
Le [journal de décisions](docs/DECISIONS.md) archive ce dialogue,
décision par décision, y compris les erreurs de l'IA et ce qu'elles ont
coûté.

La distinction qui compte : construit **avec** une IA, mais **rien d'IA
ne tourne sur l'appareil** — pas de réseau, pas de compte, jetons
virtuels, TRNG matériel. L'appareil le dit lui-même : touche `a` à
l'accueil.

---

## Documentation

| | |
|---|---|
| [CLAUDE.md](CLAUDE.md) | doctrine du projet, garde-fous, pièges matériels mesurés |
| [docs/DECISIONS.md](docs/DECISIONS.md) | le journal de décisions — D-001 à D-037, et ça continue |

---

## Licence

[MIT](LICENSE) — prenez, apprenez, construisez dessus.

La licence couvre **aussi l'identité visuelle** : palette, fonte 5×7 et
glyphes sont dans le dépôt, libres au même titre que le code. Il n'y a
pas de marque à protéger derrière ce nom.

Les bibliothèques tierces (M5Unified, M5GFX, LovyanGFX) gardent leurs
licences.
