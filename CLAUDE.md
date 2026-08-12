# Silicon Casino — casino de poche pour M5Stack Cardputer ADV

Jeu de casino hors-ligne pour Cardputer ADV (ESP32-S3, 240×135, 56 touches,
IMU BMI270, haut-parleur 1 W). Nom public acté : **« Silicon Casino »**
(D-036) — « Casino » en toutes lettres pour la recherche M5Burner, le
silicium pour l'identité maker. « Geek Casino » reste le nom de code du
dépôt.

## Garde-fous — non négociables sans décision explicite de Pierre

- **Jetons virtuels uniquement.** Pas d'argent réel, pas d'achat, pas de
  compte, pas de réseau (ni WiFi ni BLE). C'est un jouet et un exercice
  d'animation, pas un produit de jeu d'argent.
- Le joueur ne peut jamais être définitivement ruiné : renflouement
  automatique (voir décisions).
- Aucune reprise de matière depuis Daoa Mini (palette, design system,
  lettermark, masques, corpus). La méthode oui, la matière non.
- **Chaque jeu porte l'identité geek.** C'est une règle, pas une option :
  un jeu qui ressemble à n'importe quel casino n'a pas sa place ici.
  Deux façons de l'appliquer, selon ce que le geek touche :
  - **Par le décor** (chrome, dos de cartes, sérigraphie, cabinet) quand
    les éléments de jeu doivent rester lisibles — cartes du blackjack,
    numéros d'une roulette. Aucun réglage : c'est acquis.
  - **Par les éléments de jeu eux-mêmes** (symboles des rouleaux, faces
    de dés) quand le geek les remplace. Là, **un réglage doit permettre
    de revenir au jeu classique** : le joueur qui ne lit pas nos glyphes
    ne doit pas être exclu. Voir le réglage GLYPHS des machines à sous.

## Rôles

Pierre est product owner, directeur visuel, testeur et décideur final.
Claude Code est l'agent de développement principal — architecte,
implémenteur, opérateur du simulateur, mainteneur de la documentation.
**La transparence sur ce point fait partie du produit** (doctrine reprise
de Daoa Mini) : le dépôt public le dit ici, l'appareil le dit dans son
écran About (touche A à l'accueil), avec la distinction qui compte —
construit AVEC une IA, mais rien d'IA ne tourne SUR l'appareil.
Travail en petites étapes validées visuellement : simulateur d'abord,
appareil ensuite. Toute hypothèse douteuse se tranche par une mesure ou
un test, pas par une affirmation.

## Décisions actées

Journal complet dans [docs/DECISIONS.md](docs/DECISIONS.md). Résumé :

- Slot d'abord, **architecture multi-jeux prévue** (interface « jeu »
  commune + écran d'accueil).
- MVP : **3 rouleaux, 1 ligne** ; cœur paramétré (nb rouleaux / lignes)
  pour ouvrir 3×3 ou plus ensuite.
- **Solde persistant** (NVS) + **renflouement** en cas de ruine. Le solde
  est **partagé** entre les jeux ; la **mise** appartient au couple
  (joueur, jeu) et vit dans un bloc de sauvegarde séparé.
- **Levier IMU** (secouer/incliner) et **mode démo/attract** dès la v1.
- **RTP réaliste ~95 %** — mesuré à **95,24 %** par énumération exacte.

## Équilibrage — où vit quoi

Le **vocabulaire** (quels symboles existent) est généré depuis l'art vers
`lib/core/symbol_ids.h`. L'**équilibrage** est écrit à la main : ce sont
deux choses différentes, elles ne doivent pas se mélanger.

**Une seule table de gains** pour tous les formats — `Paytable`, dans
`paytable.cpp`, indexée `pay[symbole][nombre d'alignés]`. Une seule
fonction de calcul, `evaluateLine()`. Ne jamais recréer une table par
format : la duplication ne casse rien, elle rend le jeu incohérent en
silence (dette D-017, soldée par D-019).

Les **bandes** restent distinctes par format (`reels.cpp` pour le 3×1,
`multiline.cpp` pour le vidéo) : c'est un choix de jeu assumé, le jackpot
serait introuvable sur cinq rouleaux avec la bande du 3×1.

Toute retouche de la bande ou de la table **doit** être revalidée :

```bash
pio test -e test-native -f test_paytable
```

`exactLineRtp()` calcule le RTP **analytiquement** : la probabilité
d'aligner k symboles ne dépend que des effectifs des bandes, donc
l'énumération est inutile (elle serait de 33 millions de cas à 5
rouleaux). C'est un nombre exact, jamais une estimation. Les tests
refusent tout RTP hors de [93 %, 97 %], et deux d'entre eux comparent le
résultat au millionième à sa valeur d'avant l'unification.
Un `static_assert` casse la compilation si l'art change le nombre de
symboles : il faut alors refaire l'équilibrage, pas rallonger le tableau.

## Identité visuelle — design system

Direction actée (D-007, D-008, D-009) : **pixel-art néon**, palette
« nuit d'arcade », glyphes geek/maker, interface **anglaise**, animation
en **escalade selon le gain**. Le jackpot est le **space invader**.

Le **cabinet est une carte électronique** (trous de fixation, pistes,
vias) et le **levier est dessiné à droite**, hors cabinet : le geste IMU
actionne un objet visible. Les modules se nomment platement — **SLOTS**,
pas de nom poétique ; c'est « Silicon Casino » qui porte l'identité.

Le design system vit sur claude.ai/design, projet « Geek Casino — Design
System » — son identifiant est dans `private/design-system.md`, hors
dépôt car celui-ci est public. Il se régénère depuis le dépôt :

```
design/tokens.json        palette + géométrie — SOURCE DE VÉRITÉ
design/tools/art_*.py     glyphes 16x16 et fonte 5x7 — SOURCE DE VÉRITÉ
design/tools/gen.py       produit les cartes ET les en-têtes C++
design/build/             cartes générées (non versionné)
lib/ui/palette.h          GÉNÉRÉ — couleurs RGB565
lib/ui/symbols.h          GÉNÉRÉ — glyphes indexés
lib/ui/font5x7.h          GÉNÉRÉ — fonte bitmap
```

```bash
.pio/build/sim/program --screens captures/screens   # rafraîchit les captures
python3 design/tools/gen.py                          # puis les cartes
python3 scripts/readme_images.py                     # puis les images README
```

Le README (front page GitHub, anglais + version française) suit la même
doctrine : **toutes ses images sortent du simulateur** via
`scripts/readme_images.py` (`captures/` n'est pas versionné,
`docs/images/` l'est). Après tout changement d'écran visible, refaire la
chaîne complète ci-dessus.

Les cartes d'écran **embarquent les captures du simulateur**, jamais des
maquettes redessinées : une seconde implémentation de l'affichage dérive
toujours (l'accueil a annoncé « deux jeux à venir » pendant que cinq
tournaient). Lancer `--screens` **avant** `gen.py`, sinon les cartes
affichent « capture manquante » plutôt que du faux.

**Ne jamais éditer `lib/ui/palette.h`, `symbols.h`, `font5x7.h`** : ils
sont écrasés. Corriger la source, régénérer. `gen.py` valide l'art
(dimensions, clés de couleur) et échoue plutôt que de produire du faux.

Règles issues du design system :
- Un symbole se dessine en **16×16 puis s'affiche à l'échelle 3** (48 px).
  Grille grossière = lisibilité à 245 ppi, pas nostalgie.
- Le **texte est à l'échelle 2 minimum** (14 px). L'échelle 1 fait 7 px :
  illisible en main, même pour une mention secondaire.
- Les couleurs affichées sont les **valeurs quantifiées RGB565**, jamais
  les valeurs d'origine — le design system montre ce que l'écran fera.
- Le **mode démo se grise d'un bloc** : `ui::desaturate()` convertit
  l'écran fini en luminance, dans `drawApp()`. Ne jamais griser couleur par
  couleur — ce qu'on dessine ensuite en oubliant l'aide reste en couleurs.
- Le **hublot est plus haut que le symbole** : les voisins de la bande
  doivent se voir. Toute colonne de rouleau se dessine **découpée** au
  hublot, sinon elle déborde sur le bandeau de crédits.
- **Aucun glyphe `$`** dans la fonte : garde-fou, pas oubli.
- Une animation d'intensité **garde sa marge au repos**. Un analyseur de
  spectre a échoué ici pour l'avoir oubliée : ses barres remplissaient
  déjà la hauteur, donc l'à-coup ne se voyait pas (D-033). Ce qui doit
  exploser doit d'abord être plat.

## Architecture

```
lib/core/   Logique pure C++17 — AUCUN include Arduino/M5/lgfx.
            Testée par test-native. Rouleaux, table de gains, économie,
            ET le mouvement : le rythme est de la logique, pas du dessin.
            `scope` en est l'exemple limite — une trace d'oscilloscope
            écrite SANS état, entièrement déduite de l'instant et des
            rouleaux, donc testable colonne par colonne.
            Le temps entre par un paramètre `now`, jamais lu d'une horloge
            interne — c'est ce qui rend les transitions testables et les
            captures reproductibles.
lib/hal/    Frontières matérielles : hal_display.h (M5GFX ou LovyanGFX
            selon la cible), hal_keys.h (enum Key), aléa injecté.
lib/ui/     Rendu, uniquement via l'API lgfx:: (jamais le matériel).
src/        main appareil (#ifdef ARDUINO) et main simulateur
            (#ifdef SIM_BUILD) — les gardes font le tri, pas de filtre.
sim/ n'existe pas : le main simulateur vit dans src/main_sim.cpp.
test/       Tests Unity de lib/core.
```

## Trois environnements PlatformIO

- `cardputer-adv` — firmware (pioarduino, Arduino-ESP32 3.x).
- `sim` — binaire macOS, LovyanGFX + SDL2 (`Panel_sdl`), fenêtre ×3.
  Options : `--shot <dir>` capture déterministe (graine fixe) puis quitte ;
  `--frames <dir> <n>` suite d'images pour GIF.
- `test-native` — tests Unity de la logique pure.

```bash
pio run -e sim && .pio/build/sim/program          # simulateur
pio test -e test-native                            # tests
pio run -e cardputer-adv                           # firmware
pio run -e cardputer-adv -t upload                 # flash
```

## Pièges matériels vérifiés (projet précédent, mesures réelles)

- **Rendu** : tout dessiner dans un sprite plein écran 16 bits (64,8 Ko)
  puis `pushSprite` d'un bloc. Le RGB565 est stocké **octets inversés** —
  `readRect` le renvoie ainsi, et le tampon de `getBuffer()` aussi. Tout
  post-traitement pixel à pixel doit faire le `bswap16`, sinon on obtient
  du magenta uniforme au lieu d'un gris (mesuré, 2026-08-12).
- **Couleurs lgfx** : les surcharges sont typées — un littéral nu
  (`0x18E3u`, uint32) est interprété **RGB888**, seuls les `uint16_t`
  passent par RGB565. Toujours des `constexpr uint16_t` nommées, jamais
  de littéral couleur inline. (Constaté sur capture, 2026-08-11.)
- **Fontes** : DejaVu = ASCII pur, aucun accent. Latin accentué → `efontJA`
  (bitmap, ~2 Mo). Minimum lisible sur l'écran physique : **12 px**.
- **Son** : le HP ne restitue rien sous ~400 Hz ; composer entre **800 et
  2600 Hz**. Un jeu émet des `Cue` ; **`core::takeAppCue(App&)` est le
  seul point de vidange** — poker et roulette sont restés muets parce que
  leurs files n'étaient drainées nulle part, sans que rien ne plante. Pas de `tone()` : sinusoïdes PCM courtes à décroissance
  exponentielle via `Speaker.playRaw(..., 22050, ...)`. Volume réglable,
  son désactivable.
- **Init** : aucune API M5 dans un constructeur global (M5 pas encore
  initialisé). Appliquer le volume au moment de jouer, pas à la
  construction. **Même `&M5Cardputer.Display` est interdit en global** :
  Display est un membre-référence, le lire avant l'init de M5 stocke du
  garbage et `pushSprite` crashe (panic mesuré, 2026-08-11). Sprite sans
  parent + `pushSprite(&M5Cardputer.Display, 0, 0)` au moment du push.
- **Clavier** : TCA8418 (I²C), `Keyboard.keysState()`. Flèches en
  caractères : `,`=gauche `/`=droite `;`=haut `.`=bas ; plus `enter` et
  espace.
- **Aléa** : `esp_random()` (TRNG) injecté en pointeur de fonction ;
  PRNG déterministe en sim/test. Jamais de `% n` naïf (biais de modulo) —
  voir `core::drawBelow`.
- **Mémoire** : 8 Mo flash, 512 Ko SRAM, **pas de PSRAM**. Si WiFi/BLE un
  jour (a priori jamais ici) : `board_build.partitions = max_app_8MB.csv`.
  Persistance via `Preferences` (NVS).

## Méthode

1. Question de périmètre avant de coder ce qui change l'architecture.
2. Logique testée d'abord (rouleaux, gains, économie), animation ensuite.
3. Captures du simulateur à chaque étape ; flash appareil seulement pour
   ce que le sim juge mal : couleurs réelles, lisibilité, son, réactivité
   clavier, gestes IMU.
4. Tenir le journal [docs/DECISIONS.md](docs/DECISIONS.md) à jour.
