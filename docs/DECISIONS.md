# Journal de décisions

Une entrée par décision actée avec Pierre. Les plus récentes en haut.

## D-009 — 2026-08-11 — « SLOTS », et densité de l'écran

Retour de Pierre sur la première maquette : nom trop précieux, écran trop
vide. Deux décisions :

- Le module s'appelle **SLOTS**, pas « Neon Reels ». Le nom du casino
  porte la personnalité ; les modules se nomment platement (SLOTS,
  BLACKJACK, VIDEO POKER).
- **Le cabinet est une carte électronique** : trous de fixation aux
  angles, pistes sortant des hublots, vias, pistes dans la marge gauche.
  C'est ce qui distingue cette machine d'un cabinet de casino générique
  et qui ancre l'univers maker jusque dans le décor.
- **Le levier est dessiné**, hors cabinet à droite, avec trois états
  (repos / en course / tiré). Il donne un corps au geste IMU : secouer
  l'appareil actionne un objet visible, pas une fonction abstraite.
- **Les hublots sont plus hauts que les symboles** (74 px pour 48) :
  on voit arriver les symboles voisins, comme sur une vraie machine.
- L'accueil montre un **aperçu des rouleaux** dans la ligne du jeu et un
  **compteur de jackpot** en bas, au lieu d'un sous-titre décoratif.

## D-008 — 2026-08-11 — Direction artistique v1

- **Style : pixel-art néon** — pixel-art assumé (fidèle à l'écran et à
  l'esprit maker), palette néon saturée. Se quantifie bien en RGB565.
- **Palette : « nuit d'arcade »** — fond très sombre, néons saturés
  (cyan, magenta, jaune, vert acide) réservés aux symboles et effets.
- **Glyphes des rouleaux** : univers geek/maker/Cardputer, puisés dans
  trois familles — électronique/maker, rétro-computing & gaming, geek
  pop. (Famille code/terminal écartée.) **Jackpot : le space invader.**
- **Typo : pixel font assumée**, chiffres tabulaires pour solde/gains.
- **Langue : anglais** (SPIN, BET, JACKPOT…) — ASCII pur, pas d'efontJA.
- **Animation : escalade selon le gain** — base mécanique crédible,
  effets proportionnels au gain, déluge réservé au jackpot.
- **Écran de jeu : cabinet stylisé** — la machine dessinée comme un
  objet (cadre, hublots, LED, levier évoqué), écrin du geste IMU.

## D-007 — 2026-08-11 — Design system sur claude.ai/design

L'identité visuelle vit dans un projet claude.ai/design de type
**design system** (nom provisoire « Geek Casino »), synchronisé avec le
dépôt (dossier `design/`). Il sert de référence canonique : fondations
(palette, typo, motion), symboles, et maquettes d'écran 240×135. La
transposition en code lgfx (RGB565, bitmaps, primitives) reste validée
par captures sim puis écran physique.

## D-006 — 2026-08-11 — RTP réaliste ~95 %

Option « RTP généreux (> 100 %) » explicitement décochée. La table de
gains vise un retour joueur ~95 %, style vrai casino. La valeur exacte
sera fixée avec la table de gains et vérifiée par tests statistiques
(simulation massive en natif, bornes de confiance).

## D-005 — 2026-08-11 — Levier IMU et mode démo/attract dès la v1

- Tirer le levier en secouant/inclinant l'appareil (BMI270), le clavier
  restant toujours utilisable en parallèle.
- Après inactivité, mode attract : la machine joue toute seule (rouleaux,
  sons, animations). Servira aussi à générer les GIF de publication.

## D-004 — 2026-08-11 — Solde persistant + renflouement

Solde de jetons sauvegardé en NVS (`Preferences`). En cas de ruine, la
machine recrédite un petit solde (délai ou animation dédiée) : jamais de
cul-de-sac, esprit jouet. Modalités exactes du renflouement à définir au
moment de l'économie.

## D-003 — 2026-08-11 — MVP 3 rouleaux / 1 ligne, cœur paramétré

Le MVP affiche 3 rouleaux et une seule ligne de paiement (symboles ~48 px,
lisibles sur l'écran de 25 mm). Le cœur (rouleaux, table de gains) est
paramétré en nombre de rouleaux et de lignes pour ouvrir plus tard les
formats 3×3 / 5 lignes ou 4+ rouleaux sans refonte.

## D-002 — 2026-08-11 — Slot d'abord, architecture multi-jeux

Une interface « jeu » commune et un écran d'accueil existent dès la
structure initiale, même avec une seule entrée (le slot). Extension
future possible : blackjack, vidéo-poker…

## D-001 — 2026-08-11 — Cadre du projet

Jouet hors-ligne, jetons virtuels uniquement, pas d'argent réel, pas
d'achat, pas de compte, pas de réseau. Stack : PlatformIO + pioarduino
(Arduino-ESP32 3.x), M5Cardputer/M5Unified/M5GFX, pas de LVGL. Trois
environnements dès le premier jour (cardputer-adv, sim, test-native).
Identité visuelle propre, entièrement à définir — aucune reprise de
Daoa Mini.
