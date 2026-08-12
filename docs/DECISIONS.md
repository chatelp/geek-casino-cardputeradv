# Journal de décisions

Une entrée par décision actée avec Pierre. Les plus récentes en haut.

## D-033 — 2026-08-12 — L'analyseur de spectre remplacé par une trace d'oscilloscope

Le bandeau de D-032 ne marchait pas sur l'appareil : Pierre a constaté
« quasi aucune amplitude » sur les à-coups. Diagnostic : en gonflant le
socle des barres pour qu'elles soient bien visibles, on leur avait retiré
la marge dont l'à-coup avait besoin. Les barres du centre montaient déjà à
cinq crans sur cinq — le coup n'avait plus nulle part où aller.

Ce n'est pas un défaut de réglage, c'est le registre : **des barres assez
grosses pour être lues occupent la hauteur qu'il faudrait garder libre**.
Toute correction aurait rendu le repos illisible pour rendre le coup
visible, ou l'inverse.

D'où le changement complet demandé par Pierre : une **trace
d'oscilloscope**. Au repos elle est presque plate — un tiers de la
demi-hauteur, plafond explicite dans le code — et chaque verrouillage de
rouleau y injecte une **salve** qui sature l'amplitude, passe au magenta,
puis **défile vers la gauche** à un pixel toutes les quatre millisecondes.
On voit l'événement naître au bord droit et traverser. Trois rouleaux font
trois salves qui se suivent ; cinq au format vidéo. Le contraste est
désormais structurel et non obtenu à force de réglage — et un test vérifie
que la salve dépasse le repos d'un facteur deux au moins.

C'est aussi plus juste : le cabinet est une carte électronique (D-009), et
ce qu'on pose sous une carte, c'est une sonde. Un réticule discret pose la
trace sur un instrument plutôt que de la laisser flotter.

Même architecture que ce qu'elle remplace : **pur**, sans état ni horloge,
tout se déduit des rouleaux. `lib/core/spinband.*` est supprimé.

## D-032 — 2026-08-12 — Un analyseur de spectre au bas des machines à sous ; les chiffres du démarrage deviennent vrais

**Le bandeau du bas ne servait qu'à écrire « SPINNING ».** Vingt pixels de
haut sur toute la largeur, pendant tout le tour, pour un mot qui ne dit
rien que les rouleaux ne disent déjà. Il devient un **analyseur de
spectre**, façade de juke-box : vingt-quatre barres segmentées, cyan en
bas, ambre au milieu, magenta aux pointes, avec des témoins de crête qui
planent puis retombent.

Il **raconte le tour** au lieu de le commenter : l'entrain descend à
chaque rouleau verrouillé (100, 78, 62 pour trois rouleaux), et chaque
verrouillage porte un **coup** qui envoie toutes les barres au plafond
avant de retomber en 260 ms. Sur la machine vidéo, cinq rouleaux font
cinq à-coups. Pendant la célébration, la hauteur suit le palier — même
escalier que le son et l'animation (D-008) : un petit gain ne doit pas
faire le même bruit visuel qu'un jackpot.

**Tout est pur** (`lib/core/spinband.h`) : aucun état, aucune horloge
interne. La hauteur d'une barre est une fonction de (barre, instant,
entrain), et l'entrain se **déduit des rouleaux eux-mêmes** — chacun
connaît son instant d'arrêt (`t0 + dur`), donc le nombre de rouleaux en
vol et la fraîcheur du dernier verrouillage se calculent sans rien
mémoriser. Même le témoin de crête, qui semble demander une mémoire, est
le maximum des six derniers paliers : puisque la fonction est pure, on
peut simplement les redemander. Six tests, dont un qui vérifie qu'un
palier ne bouge pas entre deux images voisines — sinon le bandeau
grésillerait au lieu d'onduler.

Le `kMsgY` du format vidéo passe de 121 à **120** : la dernière rangée de
symboles finit à 119, et ce pixel récupéré donne cinq crans au lieu de
quatre, soit la même lecture qu'au 3x1.

**Le test de démarrage prend le temps d'être lu.** Huit lignes en 750 ms
faisaient moins de 100 ms chacune : de quoi voir défiler, pas de quoi
lire. La phase passe à 1560 ms, soit une ligne toutes les 163 ms, et les
deux phases de bruit rendent 190 ms — le total ne monte que de 2650 à
3160 ms, et n'importe quelle touche saute toujours la séquence. La
dernière ligne obtient enfin **son** « OK » : le compte s'arrêtait au
nombre de lignes, elle restait donc bloquée sur son compteur et le test se
terminait sans jamais conclure.

**Les chiffres de l'écran de démarrage sont maintenant vérifiés.** Le test
est faux — c'est le principe — mais les valeurs annoncées ne le sont plus :
240 MHz, 8 Mo de flash sans PSRAM, BMI270, TCA8418 en I²C à 0x34, et
64800 octets de mémoire vidéo (240 × 135 × 2, exactement notre sprite).
Tout est relu dans la définition de carte `m5stack-stamps3` et dans les
pilotes qu'instancient M5GFX et M5Cardputer.

Une seule mention ne tenait pas : « ST7789**V2** ». M5GFX instancie un
`Panel_ST7789` et ne connaît aucune révision — la mention est retirée. Un
faux écran de démarrage qui annonce du matériel inexistant n'est que du
décor ; celui qui annonce le vrai est une carte d'identité, et c'est
nettement plus geek.

## D-031 — 2026-08-12 — La touche retour marche toujours ; le gris de la démo s'applique à l'écran fini

**On ne pouvait pas sortir d'une main.** Poker, blackjack et roulette
refusaient la touche retour en cours de partie, « pour ne pas solder une
mise engagée ». Le raisonnement était faux, et le code le prouve : les
sessions sont créées une seule fois dans `newApp()`, et `tickApp()` comme
`driveDemo()` n'avancent que le jeu **affiché**. Une main qu'on quitte est
donc gelée telle quelle et attend au retour — rien n'est soldé, rien n'est
perdu. Le blocage ne protégeait rien ; il enfermait le joueur, et sans
message, puisqu'une touche refusée ne fait rien du tout.

La règle est désormais uniforme sur les cinq jeux : **la touche retour
marche toujours**. Pas de remboursement non plus, sinon elle deviendrait
une annulation gratuite — voir une mauvaise donne, sortir, revenir,
redonner. Deux tests : l'un sort des cinq jeux en pleine main, l'autre
vérifie qu'on retrouve la même main et le même solde au retour. Le premier
échoue bien si l'on remet la garde (`Expected 2 Was 12`, écran resté sur
le poker).

**Le gris de la démo se posait couleur par couleur.** Chaque écran portait
ses aides `A()`/`D()`, et tout ce qu'on dessinait ensuite en les oubliant
restait en couleurs : le décor de circuit imprimé du poker, son curseur,
son bouton `DRAW`. Une règle qu'il faut se rappeler d'appliquer à chaque
trait finit toujours par être oubliée quelque part — c'est la même famille
que les files de sons non drainées (D-030).

`ui::desaturate()` convertit **l'écran fini** en luminance perçue
(Rec.601), une fois, dans `drawApp()`. La conversion porte sur le
résultat, plus sur l'intention : rien de ce qui est à l'écran ne peut y
échapper, y compris ce qu'on dessinera plus tard sans y penser. Mesure sur
capture : **0 pixel coloré sur 32 400** dans les quatre écrans de démo,
les écrans normaux inchangés.

**Piège confirmé au passage** : le tampon du sprite 16 bits stocke le
RGB565 **octets inversés**. La première version lisait le mot tel quel et
donnait un écran magenta uniforme au lieu d'un gris. Même piège que celui
déjà noté pour `readRect`, à un autre endroit — il est maintenant écrit
dans `painter.cpp` là où on peut le rencontrer.

## D-030 — 2026-08-12 — Un seul point de vidange des sons ; le design system montre des captures, plus des maquettes

**Poker et roulette étaient muets.** Les deux jeux poussaient bien leurs
signaux sonores, mais **personne ne lisait leurs files** : chaque main
drainait `takeCue`, `takeVideoCue`, `takeBjCue` — et s'était arrêté là
quand les deux derniers jeux sont arrivés. Rien ne plantait, rien
n'échouait aux tests : le son manquait, simplement. C'est la forme la
plus coûteuse de bug, celle qui ne fait aucun bruit — au sens propre ici.

Correctif : **un seul point de vidange**, `takeAppCue(App&)`, que les deux
mains appellent. Ajouter un jeu sans l'y brancher devient impossible à
oublier, puisqu'il n'y a plus qu'un endroit où le brancher. Un test
parcourt **les cinq jeux** et exige d'entendre quelque chose dans chacun ;
il aurait attrapé la régression le jour où elle est née.

**La bille cliquette** (`Cue::Tick`, 2400 Hz, 14 ms — le son le plus court
du jeu), une fois par case franchie, mais **pas plus d'une fois toutes les
70 ms** : au lancement la bille passe une case toutes les 8 ms, sans ce
plafond on n'entendrait qu'un buzz. Le ralentissement s'entend, et c'est
tout l'intérêt d'une roulette.

**Le design system montrait des maquettes redessinées.** Les cartes
d'écran étaient repeintes en Python, à côté du vrai moteur de rendu :
elles ont dérivé sans prévenir — l'accueil y annonçait encore « une
entrée jouable, deux à venir » alors que cinq jeux tournent. Une seconde
implémentation de l'affichage ne pouvait que mentir tôt ou tard.

Le design system **embarque désormais les captures du simulateur**
(BMP → PNG en Python pur, sans outil externe, en data URI). Le simulateur
partage son code de rendu avec le firmware : la carte « Écrans de
l'appareil » ne peut donc plus diverger de l'appareil. Vingt-cinq écrans
y sont groupés — allumage, accueil, les cinq jeux, aides, mode démo.

**La palette se lit depuis les tokens.** Elle était énumérée dans une
liste écrite à la main : les trois verts de circuit imprimé sont restés
invisibles pendant tout le développement du blackjack. Même classe de
faute que la double table de gains (D-017) — une source de vérité
recopiée n'est plus une source de vérité.

## D-029 — 2026-08-12 — Sauvegarde unifiée, boot d'arcade, célébrations

**Sauvegarde consolidée.** Pierre ayant accepté de perdre ses
préférences, le bloc annexe disparaît : il n'existait que pour ne pas
invalider les classements. Tout revient dans **une seule structure** —
un magic, une version, une somme de contrôle, un chemin de lecture et
d'écriture. Surtout, **la mise vit désormais DANS le joueur**
(`Player::bet[5]`), là où elle appartient : plus de table parallèle à
garder synchrone, et changer de joueur change ses mises sans une ligne de
code supplémentaire.

**Lignes de réglages nommées.** Elles ont glissé trois fois en ajoutant
des options, et à chaque fois des tests ont cassé sur des index recomptés
à la main. Un `enum GlobalRow` remplace les nombres, dans le code comme
dans les tests. Un nom ne glisse pas.

**Séquence d'allumage façon vieille borne** (réglage BOOT FX) : bruit
multicolore, barres de couleur avec déchirures de balayage, faux test
mémoire en phosphore vert, puis le nom qui émerge du bruit. 2,65 s, et
n'importe quelle touche la saute — on ne fait pas attendre quelqu'un qui
sait ce qu'il veut. Le bruit est **déterministe** (hachage de x, y et du
numéro d'image) : mêmes captures, donc mêmes tests possibles.

**Secouer lance aussi la bille** de la roulette. Le geste vaut partout où
il a un sens physique — lancer des rouleaux, lancer une bille — mais pas
aux jeux de cartes, où « secouer pour distribuer » ne veut rien dire.

**Célébrations partout.** Blackjack, video poker et roulette reçoivent le
panneau des machines à sous, avec le décompte du gain. Le palier vient de
ce que le jeu considère comme remarquable : blackjack 3:2 et quinte
royale au sommet, plein de roulette aussi ; douzaine et brelan au milieu.
Le panneau gagne un **intitulé** — « FULL HOUSE », « BLACKJACK! », le nom
du pari — parce que dans ces jeux le *comment* compte autant que le
combien.

## D-028 — 2026-08-11 — Mode démo dans les cinq jeux, et réglable

Demande de Pierre : même déclenchement et mêmes couleurs que les slots,
plus deux réglages.

**Changement d'architecture** : le délai n'appartient plus à chaque jeu,
il est **commun à l'objet**. Un seul compteur d'inactivité dans `App`
arme tous les jeux ; auparavant chaque jeu tenait le sien, et quatre
compteurs auraient divergé. `Game` et `VideoGame` reçoivent un drapeau
`demoArmed` au lieu de décider seuls.

- **Blackjack, video poker et roulette** ont leur démo, avec une
  stratégie volontairement simple : la démo *montre* le jeu, elle ne
  cherche pas à bien jouer. Le poker garde les rangs appariés, le
  blackjack tire sous 17, la roulette change de pari à chaque tour pour
  montrer l'éventail.
- **Gratuite et muette** partout : aucun jeton ne bouge, aucune file de
  son ne se remplit. Vérifié par un test qui parcourt **les cinq jeux**.
- **Gris** partout, cartes comprises — une carte en couleurs au milieu
  d'un écran gris casserait le message.
- **Deux réglages** : DEMO MODE (on/off) et DEMO AFTER (10 / 20 / 30 /
  60 / 120 / 300 s). Le délai est grisé quand la démo est coupée : un
  réglage sans effet doit se voir comme tel.

**Persistance sans casse, encore** : les deux réglages auraient grossi
`Settings`, donc `SaveData`, donc invalidé le classement. Ils rejoignent
le bloc annexe déjà séparé — celui des mises — qui devient le bloc des
préférences.

Nuance conservée volontairement : un tour de démo **en cours** finit sa
course en gris quand le joueur reprend la main. Couper net un rouleau en
pleine rotation serait plus déroutant que de le laisser se poser. Mon
premier test l'exigeait trop strictement ; c'est l'assertion que j'ai
corrigée, pas le comportement.

## D-027 — 2026-08-11 — Roulette européenne

Cinquième jeu. Un seul zéro, 37 cases.

- **La roue suit l'ordre PHYSIQUE réel** (0, 32, 15, 19, 4, 21…), pas
  l'ordre numérique. C'est ce qui rend le ralenti crédible : les cases
  qui défilent sont celles qu'on verrait vraiment passer. Un test vérifie
  l'invariant du plan de roue — **deux cases voisines sont toujours de
  couleurs opposées**, le zéro mis à part.
- **Rendue en BANDE horizontale**, pas en rond : 37 cases en cercle sont
  illisibles sur 240 px. Le mouvement réutilise `ReelMotion`, avec une
  durée propre (3,2 s) — une bille tourne bien plus longtemps qu'un
  rouleau, et ça ne coûte rien puisque le joueur a tout décidé avant.
- **Un seul pari à la fois**, choisi aux flèches. Placer des jetons sur un
  tapis demanderait un curseur en deux dimensions, désagréable au clavier.
  Dix paris : rouge, noir, pair, impair, 1-18, 19-36, trois douzaines,
  plein.
- Geek **par le décor** : la bande est montée sur un **encodeur rotatif**
  (crans entre les cases, corps et pattes dorées sous la bande). Les
  numéros restent des numéros.

**Le fait vérifié exactement** : toutes les mises rendent **36/37 =
97,297 %**, au millionième — y compris chacun des 37 pleins pris
séparément. C'est une propriété du jeu, pas un réglage : si une seule
mise différait, un joueur pourrait trouver l'avantage. Le zéro est le
seul endroit où la maison gagne, et c'est testé.

Le test du hall a été **généralisé** au passage : il vérifiait un compte
figé de quatre jeux, il vérifie maintenant que chaque entrée ouvre le bon
écran et possède son aide. Ajouter un jeu ne casse plus le test ; en
oublier le câblage le casse.

## D-026 — 2026-08-11 — Video poker (Jacks or Better 9/6)

Quatrième jeu, validé par Pierre. Barème **9/6 « full pay »** — celui qui
donne les ~99,5 % de retour du video poker bien joué, et la meilleure
variété face aux 95 % des machines à sous.

- **Jeu UNIQUE de 52 cartes**, pas le sabot de quatre du blackjack.
  Distribuer d'un sabot changerait le jeu en silence : seize rois
  possibles au lieu de quatre. Type `Deck` ajouté à `cards.h`.
- **L'échange puise dans le même jeu** que la donne : une carte déjà en
  main ne peut pas revenir. Testé — aucun doublon après tirage.
- **La quinte royale paie 800 à la mise maximale** au lieu de 250. C'est
  la signature du jeu : la seule raison de miser gros. La mise s'affiche
  en magenta quand elle est maximale.
- **Curseur à six positions** : les cinq cartes puis une case DRAW. Sans
  elle il faudrait une touche de plus, et le Cardputer n'en a pas
  d'évidente qui soit libre.

**Vérification exhaustive du classement** : les 2 598 960 mains de cinq
cartes sont énumérées et leurs effectifs comparés aux valeurs de
référence — 4 quintes royales, 36 quintes flush, 624 carrés, 3744 fulls,
5108 couleurs, 10200 quintes, 54912 brelans, 123552 doubles paires. Si un
seul diffère, le classement est faux quelque part. Le retour sans échange
vaut 33,60 % : le plancher du jeu avant toute décision du joueur.

Cas piégeux couverts : l'As compte **haut et bas** (A-2-3-4-5 est une
quinte, A-2-3-4-5 assortie est une quinte flush et non une royale), et
K-A-2-3-4 n'en est **pas** une — l'As ne fait pas le tour.

Le hall passe à quatre entrées : les sous-titres ne tenaient plus sur la
ligne (« VIDEO POKER » à l'échelle 2 mange déjà la place), ils vivent
maintenant dans un bandeau qui ne décrit que l'entrée pointée.

## D-025 — 2026-08-11 — Deux repères d'interface corrigés

- **« V MORE » se lisait « appuyez sur V ».** J'avais écrit V et ^ faute
  de flèches dans la fonte 5×7 ; Pierre a naturellement cherché la touche
  V. Le défilement fonctionnait (il était testé) — c'était l'indication
  qui mentait. Remplacé par de **vrais chevrons dessinés** aux extrémités
  du bandeau, **permanents** : un repère qui clignote se rate.
- **Les titres d'aide débordaient sous les pastilles de pagination.**
  Corrigé durablement plutôt qu'en raccourcissant au cas par cas :
  `drawHeader` connaît la place réservée et **découpe** le titre. Un titre
  trop long est coupé net ; le repère de page reste lisible quoi qu'il
  arrive au texte.

Prochains jeux validés par Pierre : **roulette européenne** et **video
poker**.

## D-024 — 2026-08-11 — Cartes fantômes : mémoire non initialisée

Pierre : « c'est quoi l'écran bizarre au lancement du blackjack avec
toutes les cartes affichées ? » — un vrai bug, et de la pire espèce.

`Hand::n` et `Shoe::ready` n'avaient **aucune valeur par défaut**. Une
session déclarée sans initialisation (`BjSession bj;` membre de `App`)
partait donc avec un nombre de cartes tiré de la pile mémoire, et l'écran
dessinait jusqu'à douze cartes qui n'existaient pas. Pire : un sabot qui
se prétend `ready` distribue depuis un tableau **jamais mélangé**.

Ce défaut est traître parce qu'il dépend de l'état de la pile : il peut
ne jamais se voir en test, apparaître sur l'appareil, et disparaître au
recompilage suivant.

- Valeurs par défaut ajoutées à `Hand`, `Shoe` et `GridOutcome` (même
  classe de défaut côté video slot, corrigée par prévention).
- `newBjSession` vide explicitement les deux mains.
- **Test écrit AVANT le correctif** : il remplit la mémoire de `0xA5`
  puis construit la session **par défaut** (sans parenthèses — avec `()`
  le compilateur mettrait tout à zéro et le test ne prouverait rien). Il
  a bien échoué sur « 165 cartes en main » avant correction.

## D-023 — 2026-08-11 — Le geek au blackjack, aides paginées, règle du geek

**Règle de projet actée** (inscrite dans les garde-fous de CLAUDE.md) :
chaque jeu porte l'identité geek, mais de deux façons distinctes selon ce
qu'elle touche. Par le **décor** quand les éléments de jeu doivent rester
lisibles (cartes, numéros) — acquis, sans réglage. Par les **éléments de
jeu** quand le geek les remplace (glyphes des rouleaux, faces de dés) —
et alors **un réglage doit ramener le jeu classique**.

**Blackjack — le décor.** Le tapis vert d'une table de casino et le
vernis épargne d'un circuit imprimé sont exactement le même vert : c'est
ce jeu de mots qui porte le geek sans toucher aux figures.

- Première version : aplat vert. Jugée insuffisante par Pierre, à raison
  — un aplat n'est pas un circuit. Ce qui fait reconnaître une carte à un
  maker, c'est le **maillage de vias** du plan de masse, les **pistes qui
  cassent à 45°** (jamais à 90) et les **pastilles dorées** de finition.
- **Zone d'exclusion** sous la sérigraphie : ni via ni piste sous un
  marquage. C'est la règle d'un vrai fondeur, et le texte y gagne.
- Empreintes de composants là où les cartes se posent, avec repère
  broche 1 et pastilles de brasage.
- **Dos de carte** dessiné dans le design system : circuit à vias et
  invader en médaillon. C'est le dos qu'on voit le plus souvent, donc
  c'est lui qui porte l'identité.

**Aides paginées** (↑/↓), avec pastilles de pagination et chevron. Un
écran de 240×135 ne peut pas expliquer une table de gains ET cinq lignes
de paiement. Le video slot gagne surtout une page où **les cinq lignes
sont dessinées** sur des grilles 5×3 miniatures — un chevron ne
s'explique pas en mots.

## D-022 — 2026-08-11 — La célébration de gain devient un vrai moment

Retour de Pierre après essai : « le gain s'affiche très brièvement ».
Diagnostic juste — un petit gain durait **400 ms** et se contentait d'un
texte dans le bandeau. Ce n'était pas une célébration, c'était un
clignotement.

- **Durées revues** : 1200 / 1800 / 2600 / 4000 ms au lieu de
  400 / 900 / 1600 / 3000. Un test refuse tout palier gagnant sous une
  seconde.
- **Panneau en surimpression** par-dessus le jeu, qui s'ouvre
  verticalement sur les 12 % premiers de la célébration.
- **Le gain se décompte** de 0 au total, avec décélération — c'est le
  vrai cœur de l'effet, et le procédé classique des machines réelles. Le
  décompte s'achève à 55 % de la durée : le joueur doit avoir le temps de
  **lire** le total, pas seulement de le voir arriver. Testé : le
  compteur ne recule jamais, ne dépasse jamais, et atteint le total
  **exactement**.
- **Escalade par l'intensité** autant que par la durée (D-008) : rien au
  petit gain hormis le panneau ; rayons dès le palier moyen ; étincelles
  6 / 12 / 18 ; secousse du panneau sur les gros gains, à l'arrivée
  seulement. Pastille du multiplicateur dès le palier moyen — elle dit
  *pourquoi* le gain est gros.
- **Une seule implémentation** (`lib/ui/celebration.*`) pour les deux
  machines. Dupliquer un effet visuel produirait exactement la dérive
  silencieuse qu'on vient de supprimer sur les tables de gains.

Correction en cours de route : les rayons partaient du centre, donc
étaient **entièrement cachés derrière le panneau** — il ne reste que
36 px de marge. Ils partent maintenant des bords vers l'extérieur, seule
place réellement disponible.

## D-021 — 2026-08-11 — Une mise par joueur ET par jeu

Le solde est partagé — c'est ce qui fait un casino. **La mise ne l'est
pas** : elle appartient au couple (joueur, jeu).

- **Par jeu**, parce que les mises n'ont pas la même portée : au format
  vidéo une mise de 5 en engage 25. Passer d'une table à l'autre ne doit
  pas changer l'enjeu à l'insu du joueur.
- **Par joueur**, parce que deux personnes n'ont pas le même appétit. Un
  nouveau joueur repart sur la mise par défaut, pas sur celle du
  précédent.
- `pushEconomy` ne copie plus que le **solde** ; chaque jeu garde son
  `betIndex`, ramené à ce que le solde permet.

**Persistance sans casse** : la table 8 joueurs × 3 jeux vit dans un bloc
`BetMemory` **séparé** de `SaveData`, avec sa propre somme de contrôle.
La mettre dans `SaveData` aurait imposé de bumper la version, donc
d'invalider les sauvegardes existantes et d'effacer le classement de
Pierre. Le bloc est facultatif à la lecture : une sauvegarde d'avant
cette fonctionnalité se charge normalement, avec les mises par défaut.

L'accueil affiche désormais le jackpot **du jeu sélectionné**, calculé
sur la mise de ce jeu — une mise commune n'existe plus.

## D-020 — 2026-08-11 — Le réglage de mise : rendu visible, et réparé

Question de Pierre : « il faut un système pour changer la mise sur les 3
jeux ? » Il existait (←/→ partout) mais il était invisible **et faux**.

- **Bug corrigé — le gain du format vidéo suivait la mise affichée**, pas
  la mise engagée : monter la mise pendant la rotation multipliait le
  gain sans avoir rien payé de plus. La mise par ligne est désormais
  figée au lancement (`perLineStake`) et relue au règlement. Test à
  l'appui : on monte la mise au maximum en pleine rotation, le gain reste
  celui de la mise de départ.
- **La mise ne bouge plus pendant qu'un tour est en cours**, dans les
  deux machines. Changer l'enjeu après avoir vu une partie du résultat
  n'a pas de sens.
- **La montée de mise connaît le coût réel du tour** : `raiseBetFor(e,
  lignes)`. Le format vidéo engage cinq mises ; l'ancienne version
  autorisait une mise de 10 avec 30 jetons, puis la faisait retomber en
  silence au lancement.
- **Affordance ajoutée** : deux chevrons encadrent la mise, et ils
  **disparaissent** quand elle n'est pas réglable (rotation en cours,
  main de blackjack engagée, mode démo). La règle se lit sans un mot.
  Les trois pages d'aide mentionnent désormais `</>`.
- Au blackjack, l'écran montre la mise **réglable** entre deux mains et
  la mise **engagée** pendant la main (en magenta si doublée).

Bug attrapé en route : une lecture hors limites (segfault) dans la page
d'aide du blackjack, parce que le nombre de règles était codé en dur et
qu'une règle avait été retirée. Le compte est maintenant déduit du
tableau — le genre d'erreur qui ne pardonne pas sur microcontrôleur.

## D-019 — 2026-08-11 — Table de gains unifiée (dette D-017 soldée)

Il y avait deux structures disant la même chose : `three[8] + two` pour le
3×1, `pay[8][6]` pour le 5×3. Deux endroits à modifier pour un seul
réglage, **et rien qui casse si on en oublie un** — le jeu devenait
simplement incohérent, en silence. C'est ce silence qui rendait la dette
dangereuse, pas la duplication elle-même.

- **Une seule forme** : `pay[symbole][nombre d'alignés]`. Le 3×1 y range
  ses valeurs en [2] et [3], le vidéo en [3], [4], [5].
- **Une seule fonction de calcul de gain** : `evaluateLine()`. Le 3×1
  l'appelle une fois, le 5×3 cinq fois. `evaluateGrid` ne fait plus que
  recomposer chaque ligne et la lui confier.
- **Un seul calcul de RTP**, analytique, généralisé aux bandes
  différentes d'un rouleau à l'autre. L'énumération récursive des 32³
  combinaisons disparaît : elle donnait le même nombre, plus lentement.
- **Ce qui reste distinct, volontairement** : les deux BANDES. Le format
  vidéo garde la sienne parce que le jackpot serait sinon introuvable sur
  cinq rouleaux. Un choix de jeu, pas un doublon — et c'est maintenant le
  seul, donc il ne peut plus être confondu avec un oubli.

**Preuve que rien n'a bougé** : deux tests comparent le RTP à sa valeur
d'avant fusion, en millionièmes — 952393 pour le 3×1, 949470 pour le
vidéo. Une unification qui aurait changé le jeu les ferait tomber.

## D-018 — 2026-08-11 — Les trois jeux à l'écran

Les deux nouveaux jeux sont jouables, l'accueil compte trois entrées.

- **Le solde vit désormais dans `App`**, pas dans chaque jeu. Trois jeux
  qui tiendraient chacun leur compte divergeraient en silence ; le
  va-et-vient est explicite (`pushEconomy` / `pullEconomy`) et testé :
  jouer à la vidéo puis passer au blackjack conserve le solde exact.
- **Format vidéo** : zéro chrome comme demandé. HUD en surimpression,
  chevrons latéraux pour les cinq lignes, tracé de la ligne gagnante en
  reliant les cellules — un chevron ne se lit pas autrement sur grille.
  Les lignes gagnantes **défilent une à une** : cinq allumées ensemble ne
  se lisent pas. La mise affichée est le **total engagé** (mise × 5) ;
  afficher la mise par ligne obligeait à un calcul mental.
- **Blackjack** : distribution progressive (une carte toutes les 260 ms),
  carte du croupier cachée jusqu'à son tour, croupier qui tire une carte
  toutes les 620 ms. Choix HIT / STAND / DOUBLE aux flèches. On ne peut
  pas quitter une main en cours : elle se solderait sans que le joueur
  voie le résultat de sa mise.
- **Réglage blackjack : HINTS** — un point vert marque le coup de la
  stratégie de base. Il conseille, il ne joue jamais. La fonction est
  testée contre les cas contre-intuitifs de la table de référence
  (16 contre 5 → rester, 12 contre 3 → tirer).
- Le geste IMU vaut pour les **deux** machines à sous, pas au blackjack.
- Les cartes ont un dos en motif de circuit, cohérent avec le cabinet.

## D-017 — 2026-08-11 — Format vidéo 5×3 retenu, 5 lignes

Pierre : « finalement en 5x3 il reste de la place pour du chrome, comme
la poignée ». **Vérifié au pixel** plutôt qu'estimé — maquette à l'appui :
grille 172 px, colonne de 46 px pour le levier, marge gauche conservée
pour les pistes. Le 3×2 avec levier est même moins équilibré (la grille
se colle à gauche). **Le 5×3 est retenu.**

- **5 lignes** : centre, haut, bas, et les deux chevrons. Sans les
  chevrons, trois lignes parallèles se liraient comme trois machines
  côte à côte. Aucune ligne en double (testé).
- **Bande PROPRE au format vidéo** — décision de fond : avec la bande du
  3×1, cinq invaders tomberaient 1 fois sur **33 millions**, un jackpot
  décoratif. L'invader y gagne une seconde position (7/6/5/4/3/3/2/2).
  Résultat : jackpot 1 tour sur **210 000**, et surtout **4 alignés 1 sur
  14 000** — c'est celui-là qui fera les vrais moments.
- **RTP par ligne : 94,95 %**, calculé analytiquement. 32⁵ = 33 millions
  de combinaisons rendent l'énumération inutile : l'espérance d'une ligne
  ne dépend que des effectifs de la bande. Table résolue par optimisation
  sous contrainte (croissance stricte en longueur ET en rang).
- **Mise par ligne** : la mise affichée est engagée sur chacune des cinq
  lignes.
- Le test de convergence a été **rendu honnête** : l'écart-type d'une
  ligne vaut 27 fois son espérance, il faudrait 1,5 million de tours pour
  un intervalle de ±1 point. Le test vérifie donc la fréquence de gain et
  la distribution des symboles (variance faible), puis le RTP dans une
  bande de 3 sigma explicitement justifiée — au lieu d'une tolérance
  large sans raison affichée.
- **Dette assumée** : `multiline.*` est un module distinct du 3×1 plutôt
  qu'une généralisation. Le 3×1 est flashé et joué ; on ne le refactorise
  pas sous les pieds de Pierre pour ajouter un format. Fusion à faire.

## D-016 — 2026-08-11 — Blackjack, cadence démo, pistes, formats

- **Cadence de la démo espacée** : elle enchaînait sans répit car
  l'inactivité restait vraie une fois le tour fini. Un intervalle propre
  (`kAttractIntervalMs`, 5 s) sépare désormais deux tours de démo.
- **Pistes de circuit tout autour** : marges gauche ET droite (elles
  passent derrière le levier, comme sous un composant réel) et surtout
  **sur la carte**, autour des hublots — c'est là qu'elles ont le plus de
  sens puisque le cabinet EST le circuit. Toujours en ink700 sur ink900 :
  présentes, jamais concurrentes du contenu.
- **Blackjack : logique complète et testée** (`cards.*`, `blackjack.*`).
  Sabot de 4 jeux remélangé à 25 % de pénétration, croupier S17, 3:2 sur
  blackjack, doublement sur deux cartes, ni split ni assurance. L'arrondi
  du 3:2 va au joueur. **RTP mesuré : 95,81 %** avec une stratégie naïve
  (tirer sous 17) sur 20 000 mains. Même garde-fou de renflouement.
- **Formats de grille — constat, pas opinion** : sur un écran deux fois
  plus large que haut, grandir en hauteur coûte cher (symboles à 32 px)
  et grandir en largeur ne coûte rien. Le **3x3 est le pire des deux
  mondes** : il impose 32 px sans utiliser la largeur (42 % occupée).
  Les vrais candidats sont **3x2 / 2 lignes** (symboles gardés à 48 px)
  et **5x3 / video slot** (72 % de largeur occupée). Carte comparative
  « Formats de grille » dans le design system.

## D-015 — 2026-08-11 — Démo en niveaux de gris, annulation de saisie

- La démo n'est plus une teinte plate : **rampe de 3 gris** appliquée par
  luminance à chaque couleur (`kSymbolPaletteGray`, générée par gen.py).
  Les glyphes gardent leur volume — contour sombre, corps moyen, éclats
  clairs — au lieu de devenir des silhouettes. Seuils choisis pour
  répartir les 20 teintes plutôt que d'en tasser la moitié au milieu.
- Les trois gris sont **exportés nommés** (`kGrayDark/Mid/Light`) :
  indexer la rampe dépendrait de l'ordre alphabétique des clés d'art.
- Bug appareil corrigé : Échap pendant la saisie du nom était **avalé par
  le filtre de caractères** (le backtick partait dans `feedNameChar`).
  L'annulation est désormais interceptée avant le filtre, et testée.

## D-014 — 2026-08-11 — Retours du premier test en main

Quatre retours de Pierre après manipulation de l'appareil :

- **Volume par défaut : bas** (1/3). L'objet ne crie pas à la première
  prise en main.
- **La démo est muette et monochrome.** Aucun son en mode attract — la
  démo attire l'œil, elle n'impose rien à la pièce. Et tout l'écran passe
  en **gris clair** (glyphes, cadre, lampes, ligne, pommeau) avec le mot
  DEMO : le monochrome EST le message, impossible de croire à une partie.
  Le premier geste du joueur rend les couleurs. Testé : un tour de démo
  complet n'émet aucun son.
- **Le flou de rotation n'est plus un arc-en-ciel** : les bandes de
  couleur dominante sont remplacées par les **glyphes eux-mêmes étirés en
  traînées verticales** (4 rangées de l'art échantillonnées, chacune
  étirée sur un quart de symbole). Les couleurs et silhouettes restent
  celles des symboles qui passent — l'œil lit « vite », pas « rayures ».
- **Citron redessiné** : ovale pointu aux deux bouts, ombre ambrée.
- Bug de navigation corrigé : l'aide ouverte depuis l'accueil revenait
  sur le jeu lancé. Elle revient désormais d'où elle a été ouverte
  (`helpReturn`), testé.

## D-013 — 2026-08-11 — Son, geste IMU, persistance — et premier flash

- **Son** : composition dans `core/sound.*` (paliers alignés sur ceux de
  l'animation), synthèse PCM dans le main appareil — sinusoïdes à
  décroissance exponentielle, `playRaw` 22 050 Hz. La contrainte
  « 800–2600 Hz » est un TEST : toute note hors bande casse la suite.
  Les trois arrêts de rouleaux montent (1300/1550/1850 Hz) — la cascade
  s'entend. Pas de son au simulateur : il se juge sur l'appareil.
- **Geste IMU** : détection de secousse par écart à 1 g, hystérésis
  (redescendre au calme avant de redéclencher) + délai de garde 500 ms.
  Réglée par tests (`test_shake`), pas au jugé. Secouer = Confirm sur
  l'écran de jeu uniquement.
- **Persistance NVS** : la sauvegarde porte roster + réglages, signée
  (FNV-1a) et bornée ; toute corruption est rejetée EN BLOC (testé octet
  par octet). Écriture throttlée à 2 s, jamais pendant une rotation.
- **Mode démo gratuit** : découvert par un test — l'attract misait les
  jetons du joueur. Désormais tirage réel, jetons intouchés.
- **Premier flash réel** : panic au boot — `M5Canvas canvas(&M5Cardputer.
  Display)` en global lit un membre-référence avant l'init de M5 (ordre
  des globaux indéfini). Piège documenté dans CLAUDE.md ; le sprite est
  désormais sans parent, la destination passée au push.

## D-012 — 2026-08-11 — Aide, réglages, habillage classique, joueurs

Demandes de Pierre, intégrées dans une couche « app » pure et testée :

- **Page d'aide par jeu (touche H)** : pour SLOTS, la table des gains
  avec chaque glyphe geek à côté de son équivalent **classique** —
  cerises, citron, orange, prune, pastèque, cloche, BAR, 7 — dessinés en
  16×16 et en correspondance stricte rang par rang (validée par gen.py).
- **Habillage par jeu (touche S en jeu)** : GEEK ou CLASSIC. Même index,
  même bande, même gain — seul le dessin change.
- **Réglages généraux (touche S à l'accueil)** : son, volume (3 crans),
  joueur courant, reset du classement (double pression, désarmé par
  toute navigation).
- **Multi-joueurs + leaderboard (touche L)** : jusqu'à 8 joueurs, nom
  demandé au premier lancement (A–Z 0–9, 8 caractères), bascule par les
  réglages, un nom existant bascule au lieu de dupliquer. Classement par
  solde puis meilleur gain, persistant.
- L'écran d'accueil est réellement implémenté (il n'était qu'une
  maquette) : sélection de jeu, solde du joueur, jackpot courant.

## D-011 — 2026-08-11 — Animation : le rythme est de la logique

Le jeu tourne dans le simulateur. Choix structurant : **le mouvement vit
dans `lib/core`, pas dans `lib/ui`**. La courbe d'arrêt, la cascade et la
machine à états sont testées sans écran (`test_motion`) ; `lib/ui` ne fait
qu'afficher la position que `core` calcule. Le temps entre par un
paramètre `now`, jamais lu depuis une horloge interne — c'est ce qui rend
chaque transition vérifiable et les captures reproductibles.

- **Décélération cubique + dépassement** de 0,55 symbole, nul en fin de
  course : le rouleau tombe *exactement* sur sa cible. Testé pour les 32
  positions — une dérive d'un centième désalignerait l'affichage en
  permanence.
- **Cascade** : 900 ms pour le premier rouleau, +450 ms par rouleau. Un
  test refuse que deux rouleaux s'arrêtent ensemble.
- **Paliers de célébration** : 0 / 400 / 900 / 1600 / 3000 ms, strictement
  croissants (testé).
- **Flou de vitesse** — découvert en regardant l'animation, pas prévu :
  au-delà de 1,2 symbole par image, afficher les glyphes produit du
  scintillement, pas de la vitesse. Le rouleau bascule sur des bandes de
  la couleur dominante de chaque symbole, défilant ~5× moins vite que le
  rouleau réel. Mensonge assumé : l'œil lit « très vite » et ne peut pas
  compter. La couleur dominante est calculée par `gen.py` depuis l'art.
- **Mode démo** après 12 s d'inactivité.
- Le firmware et le simulateur partagent le même `core::Game` : seuls
  l'horloge, l'aléa et le clavier changent.

## D-010 — 2026-08-11 — Équilibrage : bande, table de gains, RTP

Le cœur jouable est écrit et testé. Chiffres **mesurés, pas visés** :

- **RTP exact : 95,24 %** — calculé par énumération des 32³ combinaisons,
  pas par simulation. C'est un nombre déterministe, pas une estimation.
- **Tours gagnants : 17,58 %.**
- Bande de **32 positions**, partagée par les 3 rouleaux. Effectifs :
  résistance 8, LED 7, puce 6, disquette 4, manette 3, CRT 2, d20 1,
  invader 1. C'est la bande qui fixe les probabilités, pas la table.
- Gains pour 3 identiques : 8 / 12 / 20 / 50 / 100 / 250 / 400 / **1200**
  (invader). Deux identiques **en tête** : 2. Lecture de gauche à droite,
  comme sur une machine réelle.
- Deux positions voisines de la bande sont toujours différentes : un arrêt
  imprécis d'un cran ne doit pas changer le résultat de façon invisible.
- Échelle de mises 1 / 2 / 5 / 10 / 25 / 50, défaut 5. La mise **suit le
  solde vers le bas** au lieu de bloquer le joueur.
- **Renflouement à 500** dès que la plus petite mise est hors de portée.
  Un test joue 200 000 tours à la mise maximale et vérifie que la machine
  ne refuse jamais de jouer (garde-fou D-004).

Les maquettes ont été corrigées pour être arithmétiquement vraies :
3 puces à la mise 5 rapportent 100 (et non 250), le jackpot 6000.

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
