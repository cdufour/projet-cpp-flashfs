# Projet C++ — « FlashFS » : système de fichiers pour mémoire flash simulée

**POEI — Linux, Langages & Systèmes (LLS · EXPLEO)** · AJC FORMATION
Module **Projet C++** · **2 jours / 14 heures** (jeudi 1er et vendredi 2 octobre 2026) ·
Travail en **binôme**, en autonomie encadrée

---

## Contexte général

Vous êtes développeur embarqué. Votre produit (un compteur, une chaudière,
un boîtier télématique…) stocke sa configuration et ses journaux dans une
petite **mémoire flash NOR** soudée sur la carte. Il n'y a ni disque dur ni
système d'exploitation pour la gérer : c'est à votre logiciel d'en masquer
les contraintes physiques.

Une mémoire flash a trois défauts qui la distinguent d'une RAM :

1. **On n'écrit pas où l'on veut.** Un octet effacé vaut `0xFF`, et
   programmer ne peut faire passer des bits que de **1 à 0**. Pour remettre un
   bit à 1, il faut **effacer un secteur entier** (plusieurs Kio d'un coup).
   Modifier un fichier « sur place » est donc impossible : on écrit la
   nouvelle version ailleurs, et l'ancienne devient périmée.
2. **Elle s'use.** Chaque secteur supporte un nombre limité d'effacements.
   Si l'on efface toujours le même secteur, il meurt ; il faut **répartir
   l'usure**.
3. **Le courant peut être coupé à tout moment**, y compris au milieu d'une
   écriture. Au redémarrage, le système doit retrouver un état **cohérent** :
   chaque fichier contient soit son ancienne version intacte, soit la
   nouvelle complète, jamais un mélange.

Votre mission est d'écrire **FlashFS**, dans l'esprit des systèmes de
fichiers embarqués réels (littlefs, SPIFFS, JFFS2) :

- une **mémoire flash simulée**, qui fait respecter les règles physiques
  ci-dessus, stockée en RAM (pour les tests) ou dans un fichier image (pour
  la persistance) ;
- un **système de fichiers journalisé** par-dessus : créer, lire, remplacer,
  supprimer et lister des fichiers ;
- la **résistance aux coupures de courant**, démontrée par des coupures
  simulées, déterministes et rejouables ;
- la **récupération d'espace et la répartition de l'usure** ;
- l'**accès concurrent** depuis plusieurs threads ;
- en extension, une **image sur disque** et un **shell** en ligne de commande
  pour manipuler le tout.


Le projet se déroule en **deux phases** :

| Phase   | Contenu                                                                 | Jour       |
| ------- | ----------------------------------------------------------------------- | ---------- |
| Phase 1 | Flash simulée, format sur flash, système de fichiers, poignée (*handle*) `Fichier` : une **bibliothèque testée** | Journée 1 |
| Phase 2 | Coupures de courant, concurrence, finalisation, rendu (puis extensions) | Journée 2 |


**Méthode de travail.** Vous travaillez en binôme, en toute autonomie. Vous
êtes libres de vos choix de conception, dans le respect des éléments
**imposés** signalés comme tels. Le formateur répond aux questions, peut
épauler un binôme en difficulté ou faire un point collectif sur une notion.

**Consignes transverses (non négociables).**

- **Squelette fourni** par le formateur (dossier `squelette/`) : vous
  partez de lui. Il contient le `CMakeLists.txt`, l'interface imposée
  `BlockDevice`, le **format détaillé** (`FORMAT.md`), l'en-tête
  `SystemeFichiers.hpp`, des **briques utilitaires déjà écrites** (la
  hiérarchie d'exceptions `Erreurs.hpp`, le CRC32 et la lecture/écriture
  little-endian), un petit outil de tests (`tests/Verif.hpp`) et des tests
  d'exemple. Standard **C++17**,
  compilation avec `-Wall -Wextra -Wpedantic -Wshadow -Werror` : **zéro
  warning**, puisque tout warning est une erreur.
- **Linux (VM Debian)** : le projet doit compiler et passer ses tests sous
  Linux avec g++. Ce qui ne marche que sous Windows ou Code::Blocks ne compte
  pas. Pré-requis : `sudo apt install cmake valgrind`.
- **Gestion de la mémoire :**
  - **aucun `new` ni `delete` dans votre code**. La propriété passe par des
    objets (`std::unique_ptr`, `std::vector`, `std::string`, flux) qui
    libèrent eux-mêmes ce qu'ils détiennent (RAII) ;
  - `valgrind --leak-check=full` doit être propre sur chaque exécutable de
    tests (et sur le shell si vous faites l'extension E3). Pour tous les
    passer d'un coup :
    `for t in build/test_*; do valgrind -q --leak-check=full --error-exitcode=1 $t > /dev/null || echo "PROBLÈME : $t"; done`
- **Tests** : chaque fichier `tests/test_xxx.cpp` est un **petit programme**
  avec son `main()`, qui devient l'exécutable `build/test_xxx`. Les
  vérifications viennent de l'outil fourni `tests/Verif.hpp` (voir
  « Écrire un test » ci-dessous). `ctest` lance tous les fichiers de tests.
  Les tests **exigés** sont ceux de la checklist finale ; les autres listes
  de tests de l'énoncé sont des suggestions.
- **Jalons** : chaque demi-journée se termine par un **commit + tag**
  (`v0.1`, `v0.2`, `v0.3`, `v1.0`) poussé sur GitLab.
- **Dépôt** : chaque binôme crée sur **GitLab** un projet nommé `flashfs` et
  **invite le formateur en Reporter** dès la création. Chaque membre commite
  sur sa branche `dev/<prénom>` ; les fusions vers `main` passent par Merge
  Request.


---

## Périmètre : socle obligatoire et extensions

Deux jours, c'est court. L'énoncé décrit le projet **complet**, mais il se
découpe en deux niveaux, **annoncés dès le départ** :

- le **socle** est exigé de tous les binômes, sans exception. C'est le
  contrat minimal pour que le projet soit considéré comme **terminé**. Les
  quatre jalons (`v0.1` à `v1.0`) sont calés sur lui ;
- les **extensions** sont au choix du binôme, sans obligation de tout
  traiter. Elles **majorent** la note, et ne compensent jamais un socle
  incomplet.

Chaque étape de l'énoncé porte l'étiquette **[SOCLE]** ou **[EXTENSION]**.

| Socle (obligatoire) | Extensions (au choix, par ordre de rentabilité) |
| --- | --- |
| Flash simulée en RAM et ses règles physiques (exceptions et CRC32 fournis) | **E1 — `fsck`** : vérification de cohérence, commande et code 7 |
| Format sur flash imposé, montage, écriture, lecture, suppression, remontage | **E2 — Récupération d'espace et répartition de l'usure** |
| Poignée `Fichier` : RAII, move-only | **E3 — Persistance et shell**, en deux niveaux : (1) image sur disque, shell minimal, démo `--coupure` ; (2) shell et image complets |
| Validation en deux temps et **test de coupures exhaustif** | **E4 — Bonus** : cache template, coupure pendant un effacement, CI GitLab, Doxygen… |
| Accès concurrent (un `std::mutex`) et test sous TSan | |

**Sans l'extension E2**, une flash pleine lève simplement `DisquePlein` :
c'est accepté pour le socle.

**Le socle est une bibliothèque testée**, sans programme exécutable à écrire :
la preuve se fait par les tests (`ctest`, test de coupures, Valgrind, TSan),
que le formateur relance sur votre branche `main`. Le shell, qui rend le projet manipulable à la main,
est l'extension E3.

**Les invariants ne bougent pas**, quel que soit votre niveau d'ambition :
zéro warning, aucun `new` / `delete`, Valgrind propre, tests verts, un
commit et un tag par demi-journée, et les deux membres commitent.

**En cas de retard**, sacrifiez les extensions, **jamais le socle ni sa
qualité**. Un socle complet et impeccable vaut mieux qu'un projet où tout
est commencé et rien n'est terminé. En cas de doute sur ce qui est attendu,
demandez au formateur : la réponse tient en une phrase, alors qu'une
mauvaise interprétation peut coûter une demi-journée.

---

## Vue d'ensemble — architecture

```text
[E3] main.cpp (orchestration : options, création du périphérique, lancement du shell)
[E3]  └─ Shell — table de commandes  std::map<std::string, std::function<…>>  (lambdas)
tests/ ──► SystemeFichiers — formater, monter, ouvrir, lire, supprimer, liste,
        │                  [E3 : statistiques]  [E1 : verifier]  [Phase 2 : thread-safe]
        ├─ Fichier — poignée move-only (RAII) : écriture tamponnée, validée à la fermeture
        ├─ Format sur flash : en-têtes de secteur + enregistrements (CRC32, LE)
        └─ BlockDevice (interface IMPOSÉE, fournie)
             ├─ FlashRam       — mémoire en RAM (tests)
             ├─ FlashFichier   — mémoire persistée dans un fichier image [E3]
             └─ FlashInstable  — décorateur : coupe le courant à la N-ième écriture (Phase 2)
```

Le système de fichiers **ne connaît que l'interface** `BlockDevice` : il
fonctionne à l'identique sur les trois mémoires. C'est le même découplage
que l'abstraction de source du Projet C, avec un outil C++ différent
(héritage et fonctions virtuelles au lieu de pointeurs de fonctions).

**Vérification du socle** par le formateur (voir l'étape 6 de la Phase 2 et l'annexe A.0) :

```text
build depuis zéro ──► ctest vert ──► test Coupures (N positions, toutes cohérentes) ──► Valgrind ──► TSan
```

**Session type avec l'extension E3** (voir l'annexe A.1 et suivantes) :

```text
créer l'image ──► write / ls / read ──► coupure pendant un write ──► redémarrage ──► données cohérentes
```

---

## Composants et notions mobilisées

Chaque composant réinvestit une notion vue en Programmation C++, Modern C++
et Multithreading (Exos 1 à 10). Le tableau sert de checklist de couverture :

| Composant | Rôle | Notions mobilisées | Niveau |
| --- | --- | --- | --- |
| `Erreurs.hpp` | Hiérarchie d'exceptions du projet | Lever, attraper, traduire en codes retour *(Partie 04, Exo 5)* | **Fourni** |
| `FlashRam` | Flash en RAM, fait respecter les règles physiques | Classe, encapsulation, `std::vector`, `override` *(Parties 01-03, 10)* | Socle |
| `FlashFichier` | Flash persistée dans un fichier image | RAII sur un `std::fstream`, sérialisation binaire *(Partie 05)* | E3 |
| `FlashInstable` | Décorateur qui simule une coupure de courant | Composition + héritage, **propriété par `std::unique_ptr`**, transfert de propriété (`std::move`) *(Partie 09)* | Socle |
| `Crc32`, `Octets.hpp` | Intégrité et encodage portable | Le `cbx_crc32` du Projet C, en C++ | **Fourni** |
| `SystemeFichiers` | Le cœur : format sur flash, montage, écriture journalisée | Conteneurs STL (`map`, `vector`), algorithmes et lambdas *(Parties 07-08, Exo 8)* | Socle |
| `Fichier` | Poignée de fichier ouvert | **Règle de 5, type move-only** (`= delete` sur la copie), destructeur `noexcept` *(Parties 02, 09, 10)* | Socle |
| `Shell` | Interpréteur de commandes | `std::map` de `std::function` et lambdas (successeur C++ de la table de pointeurs de fonctions du Projet C) | E3 |
| Concurrence (Phase 2) | Plusieurs threads sur le même système de fichiers | `std::thread`, `std::mutex`, `std::lock_guard` *(MultiThreading, Exo 10)* ; `std::shared_mutex` en bonus | Socle |
| Tests | Petits programmes de test (`Verif.hpp` fourni) lancés par `ctest`, dont un test de coupures exhaustif | Tests unitaires, déterminisme | Socle |
| Outillage | CMake, Valgrind, ASan, TSan, gdb | Debug *(module du 29/09)* | Socle |

---

## Le format sur flash — spécification cadre (IMPOSÉE)

Ce format est **imposé** : il porte la leçon du projet. Ses détails (ordre
exact des octets couverts par le CRC, règle de nommage, taille maximale,
traitement de chaque cas anormal, exemple octet par octet) sont précisés
dans **`FORMAT.md`, fourni dans le squelette**. Tous les entiers sont stockés en **little-endian**,
champ par champ. Comme au Projet C, on n'écrit jamais une `struct` brute.

### Secteurs

```
Secteur (tailleSecteur octets)
┌────────────────────────────────────────────────────────────────┐
│ EN-TÊTE DE SECTEUR (16 octets)                                  │
│   magic[4]   = "FFS1"                                           │
│   sequence   : uint32   ordre de mise en service du secteur (≥ 1)│
│   reserve    : uint32   = 0xFFFFFFFF                            │
│   crc32      : uint32   CRC32 des 12 octets précédents          │
├────────────────────────────────────────────────────────────────┤
│ ENREGISTREMENT 1                                                │
│ ENREGISTREMENT 2                                                │
│ …                                                               │
│ zone libre (0xFF jusqu'à la fin du secteur)                     │
└────────────────────────────────────────────────────────────────┘
```

Un secteur est dans l'un de ces trois états :

- **libre** : entièrement à `0xFF` ;
- **utilisé** : en-tête valide (magic correct, CRC correct) ;
- **sale** : ni l'un ni l'autre (par exemple un en-tête à moitié écrit
  pendant une coupure). Un secteur sale devra être effacé avant d'être
  réutilisé.

### Enregistrements

Chaque opération qui modifie le système de fichiers **ajoute** un
enregistrement à la suite dans le secteur actif. On ne modifie jamais un
enregistrement existant, à une exception près : l'octet `etat`.

```
Enregistrement (16 octets d'en-tête + nom + données)
  etat      : uint8    0xFF = non validé ; 0x00 = validé (programmé EN DERNIER)
  type      : uint8    0x01 = FICHIER (contenu complet) ; 0x02 = SUPPRESSION
  lgNom     : uint8    longueur du nom (1 à 32)
  reserve   : uint8    = 0xFF
  sequence  : uint32   numéro d'ordre global de l'opération (croissant)
  taille    : uint32   nombre d'octets de données (0 pour SUPPRESSION)
  crc32     : uint32   CRC32 de type, lgNom, reserve, sequence, taille, nom et données
  nom       : lgNom octets (sans zéro terminal)
  donnees   : taille octets
```

**L'astuce de validation.** Un enregistrement s'écrit en deux temps :

1. on programme l'en-tête (avec `etat = 0xFF`), le nom et les données ;
2. **puis** on programme le seul octet `etat` à `0x00`.

Passer de `0xFF` à `0x00` est autorisé (bits de 1 à 0). Si le courant est
coupé avant l'étape 2, l'enregistrement reste « non validé » : à la
relecture, il est ignoré, et l'état précédent du fichier est conservé. C'est
ce qui rend chaque écriture **atomique** (tout ou rien).

**Règles de relecture (montage).**

- Dans un secteur utilisé, on lit les enregistrements les uns après les
  autres à partir de l'octet 16.
- Un en-tête d'enregistrement entièrement à `0xFF` marque la **fin de la
  zone écrite**.
- Un enregistrement **non validé**, ou dont le CRC est faux, ou dont la
  longueur déborde du secteur, **termine la zone utile du secteur**. On ne
  peut pas se fier à sa longueur pour sauter par-dessus. Le secteur est
  alors considéré comme **plein** : on n'y écrira plus.
- L'état du système de fichiers s'obtient en **rejouant les enregistrements
  valides dans l'ordre croissant de `sequence`** :
  - `FICHIER` fixe la version courante de ce nom ;
  - `SUPPRESSION` retire ce nom.
- Un fichier fait au plus `tailleSecteur − 32 − lgNom` octets : un
  enregistrement tient toujours dans un seul secteur.

---

# PHASE 1 — Flash simulée et système de fichiers (journée 1)

> Objectif : une **bibliothèque** qui fonctionne, testée et sans fuite.
> Tout se vérifie par des tests (`tests/test_*.cpp`, lancés par `ctest`).

## Arborescence cible

```
flashfs/
├── CMakeLists.txt            ← FOURNI
├── FORMAT.md                 ← FOURNI : le format détaillé, octet par octet
├── README.md
├── include/flashfs/
│   ├── BlockDevice.hpp       ← FOURNI, interface imposée
│   ├── Erreurs.hpp           ← FOURNI
│   ├── FlashAbstraite.hpp    ← facultatif (conseil de l'étape 3, utile surtout avec E3)
│   ├── FlashRam.hpp
│   ├── FlashFichier.hpp      ← extension E3
│   ├── FlashInstable.hpp     ← Phase 2
│   ├── Crc32.hpp             ← FOURNI (+ src/Crc32.cpp)
│   ├── Octets.hpp            ← FOURNI, lecture/écriture little-endian
│   ├── SystemeFichiers.hpp   ← FOURNI : API publique exigée, partie privée modifiable
│   ├── Fichier.hpp
│   └── Shell.hpp             ← extension E3
├── src/                      ← un .cpp par classe (+ main.cpp du squelette, complété en E3)
└── tests/                    ← Verif.hpp (FOURNI), test_flash.cpp, test_fs.cpp, test_coupures.cpp, …
```

## Écrire un test

Pas de bibliothèque de tests à apprendre : un fichier de tests est un
programme ordinaire. Chaque test est une **fonction**, et le `main()` les
lance une par une. Quatre vérifications sont fournies par `tests/Verif.hpp` :

| Vérification | Sens |
| --- | --- |
| `VERIFIER(condition)` | la condition doit être vraie |
| `VERIFIER_EGAL(a, b)` | `a` doit valoir `b` (les deux valeurs sont affichées en cas d'échec) |
| `VERIFIER_LEVE(instruction, TypeException)` | l'instruction doit lever cette exception |
| `EXIGER(condition)` | comme `VERIFIER`, mais arrête le test en cours en cas d'échec |

```cpp
#include "Verif.hpp"
#include "flashfs/FlashRam.hpp"
#include "flashfs/Erreurs.hpp"

void flashNeuveAFF()
{
    flashfs::FlashRam f(4, 256);
    std::uint8_t o;
    f.lire(0, &o, 1);
    VERIFIER_EGAL(o, 0xFF);
    VERIFIER_LEVE(f.effacer(4), flashfs::AdresseInvalide);
}

int main()
{
    verif::lancer("flashNeuveAFF", flashNeuveAFF);   // affiche [ OK ] ou [ ÉCHEC ]
    return verif::resultat();                        // 0 si tout est vert
}
```

C'est le principe de `assert()` du C, en mieux : un échec affiche le
fichier, la ligne et les valeurs, et les vérifications suivantes continuent.
Une exception imprévue fait échouer le test (sans arrêter les suivants) :
pour vérifier qu'une instruction **ne lève pas** d'exception, il suffit de
l'écrire.

Un nouveau fichier `tests/test_xxx.cpp` est pris en compte au prochain
`cmake --build`. On lance :

```bash
ctest --test-dir build --output-on-failure   # tous les fichiers de tests
./build/test_flash                            # un seul, avec tout son affichage
```

## Étape 1 — Mise en place (première heure) [SOCLE]

1. Créez le projet GitLab `flashfs`, invitez le formateur en **Reporter**,
   copiez-y le contenu de `squelette/`, puis créez vos branches
   `dev/<prénom>`.
2. Vérifiez la chaîne complète sur la VM Debian **avant d'écrire une ligne** :

   ```bash
   cmake -S . -B build && cmake --build build -j && ctest --test-dir build
   ```

3. Lisez `BlockDevice.hpp` en entier : c'est votre contrat.

## Étape 2 — Les exceptions (`Erreurs.hpp`, FOURNI) [SOCLE]

La hiérarchie d'exceptions est **fournie** : lisez-la. Toutes dérivent de
`std::runtime_error`. À vous de les **lever** au bon endroit avec un message
explicite, et de les **attraper** (dans les tests, et dans le shell de
l'extension E3, qui les traduit en codes retour).
Vous pouvez en ajouter.

| Exception | Levée quand… |
| --- | --- |
| `ErreurFlash` (base des erreurs matérielles) | — |
| `AdresseInvalide` | adresse ou secteur hors de la mémoire |
| `ProgrammationInterdite` | une écriture exige un bit 0 → 1 |
| `SecteurUse` | effacement au-delà de l'endurance |
| `CoupureCourant` | coupure simulée (Phase 2) |
| `ErreurFs` (base des erreurs du système de fichiers) | — |
| `FichierIntrouvable` | nom inconnu |
| `NomInvalide` | nom vide, trop long (> 32) ou avec un caractère hors `[A-Za-z0-9._-]` |
| `FichierTropGros` | le contenu ne tient pas dans un secteur |
| `DisquePlein` | plus de place |
| `ImageInvalide` | fichier image absent, illisible ou de mauvais format (extension E3) |
| `ErreurHote` | fichier de la machine hôte illisible ou non inscriptible (`import` / `export`, extension E3) |

Chaque message doit être explicite : écrivez « adresse 70000 hors de la
flash (65536 octets) » plutôt que « erreur ».

## Étape 3 — `FlashRam` et les règles physiques [SOCLE]

Implémentez `FlashRam`, une mémoire flash stockée dans un `std::vector` :

```cpp
FlashRam(std::size_t nbSecteurs, std::size_t tailleSecteur, std::uint32_t enduranceMax = 1000);
```

> **Pourquoi « programmer » et pas « écrire » ?** C'est le terme consacré
> des mémoires flash, qu'on retrouve dans les fiches techniques des puces
> (commandes *Page Program* et *Sector Erase*). Il désigne une opération
> **plus limitée** qu'une écriture ordinaire : elle ne sait que faire passer
> des bits de 1 à 0. Pour obtenir une valeur quelconque, il faut **effacer**
> le secteur (tout revient à 1), puis **programmer**. Le couple
> `programmer` / `effacer` rappelle cette contrainte à chaque appel.
>
> | Action | Octet avant | Demandé | Résultat |
> | --- | --- | --- | --- |
> | programmer | `FF` = 1111 1111 | `5A` = 0101 1010 | ✅ `5A` : on n'a fait qu'éteindre des bits |
> | programmer | `5A` = 0101 1010 | `50` = 0101 0000 | ✅ `50` : encore des bits éteints |
> | programmer | `50` = 0101 0000 | `A5` = 1010 0101 | ❌ refusé : il faudrait rallumer des bits (0 → 1) |
> | effacer le secteur, puis programmer | `FF` | `A5` | ✅ `A5` |
>
> C'est aussi ce qui rend possible la validation en deux temps du format :
> l'octet `etat` passe de `FF` à `00` par une simple programmation.

Une flash neuve est entièrement à `0xFF`, avec des compteurs d'usure à 0.
Elle fait respecter les **cinq règles** documentées dans `BlockDevice.hpp`.
Une programmation refusée ne doit **rien** écrire, même partiellement :
vérifiez d'abord, écrivez ensuite.

> **Conseil de conception.** Les règles physiques sont les mêmes pour
> `FlashRam` et `FlashFichier` (extension E3). Plutôt que de les dupliquer, placez-les dans
> une classe de base intermédiaire (par exemple `FlashAbstraite`) qui
> implémente `lire` / `programmer` / `effacer` en vérifiant les règles, et
> qui délègue le stockage brut à des méthodes virtuelles protégées que les
> classes dérivées implémentent. C'est le patron « méthode template ».
> Pour le socle seul, où `FlashRam` est la seule flash, cette classe n'est
> pas nécessaire : `FlashRam` peut hériter directement de `BlockDevice`.
> Vous pourrez introduire `FlashAbstraite` plus tard, si vous faites E3.

**Tests suggérés** (`tests/test_flash.cpp`, non exigés : utiles pour avancer sereinement) :

- une flash neuve est à `0xFF` ;
- programmer puis relire ;
- reprogrammer `0xF0` par-dessus `0xFF` fonctionne ;
- reprogrammer `0x0F` par-dessus `0xF0` lève `ProgrammationInterdite` et
  laisse l'octet intact ;
- l'effacement remet `0xFF` et incrémente le compteur ;
- l'endurance dépassée lève `SecteurUse` ;
- une adresse hors limites lève `AdresseInvalide`, y compris un débordement
  en fin de mémoire (`adresse + n > taille`).

## Étape 4 — CRC32 et octets little-endian (FOURNIS) [SOCLE]

Ces deux briques sont **fournies**, avec leurs tests (`tests/test_crc.cpp`).
Vous les avez déjà écrites en C au Projet C :

- `crc32(donnees, n, crc = 0)` : CRC32 IEEE, **incrémental**. Pour enchaîner
  deux blocs, on passe le résultat du premier en troisième argument ;
- `ecrireU32` / `lireU32` : un `uint32_t` en little-endian dans un tampon.

## Étape 5 — Lire `FORMAT.md` (FOURNI) [SOCLE]

Vous avez déjà rédigé ce genre de document au Projet C : celui-ci est
**fourni**. Lisez-le en entier avant l'étape 6. Il précise l'ordre exact
des octets couverts par le CRC, la règle de nommage, la taille maximale d'un
fichier, et le traitement de chaque cas anormal à la relecture.

C'est votre **contrat** : le formateur relit votre code avec lui. Son
exemple octet par octet (le fichier `a.txt` contenant `hi`) vous servira à
vérifier vos premières écritures sur la flash.

**Tag `v0.1`** (fin de matinée) : `FlashRam` et ses règles physiques.

## Étape 6 — `SystemeFichiers` : formater, monter, écrire, lire, supprimer [SOCLE]

L'en-tête `include/flashfs/SystemeFichiers.hpp` est **fourni**, pour que vous
passiez votre temps sur le code plutôt que sur le découpage de la classe :

- sa **partie publique** est l'API exigée. Vous pouvez l'enrichir, pas la
  réduire ;
- sa **partie privée** est une suggestion d'organisation (état des
  secteurs, index, secteur actif, fonctions internes). Modifiez-la
  librement ;
- tout le fichier `src/SystemeFichiers.cpp` est à écrire.

```cpp
class SystemeFichiers {
public:
    static void formater(BlockDevice& flash);        // efface tous les secteurs
    explicit SystemeFichiers(BlockDevice& flash);    // MONTE : relit la flash (règles de relecture)

    Fichier ouvrir(const std::string& nom);          // poignée d'ÉCRITURE (étape 7)
    void ecrireFichier(const std::string& nom, const std::vector<std::uint8_t>& donnees);
    std::vector<std::uint8_t> lireFichier(const std::string& nom) const;
    void supprimer(const std::string& nom);
    bool existe(const std::string& nom) const;
    std::vector<InfoFichier> liste() const;           // {nom, taille}, triée par nom
};
```

`Fichier` n'y est que **déclaré** (`class Fichier;`) : l'étape 6 compile
sans `Fichier.hpp`. Vous implémenterez `ouvrir()` à l'étape 7.

- **Propriété de la mémoire.** Le système de fichiers **ne possède pas** la
  flash : il en garde une référence (une association, au sens UML). La
  flash doit donc vivre plus longtemps que lui. Documentez-le.
- **Montage.** Le système de fichiers garde en mémoire un **index**
  (`std::map` nom → position, taille et séquence de la version courante),
  reconstruit en rejouant les enregistrements. Il ne relit la flash que
  pour les données.
- **Écriture d'un fichier.** Elle ajoute un enregistrement `FICHIER`
  (contenu complet) validé en deux temps. Si le secteur actif n'a plus
  assez de place, on met en service un **nouveau secteur** : le premier
  secteur libre ou sale, qu'on efface s'il est sale, puis on y programme un
  en-tête avec la séquence de secteur suivante. (Choisir le secteur **le
  moins usé** relève de l'extension E2.)
- **Suppression.** Elle ajoute un enregistrement `SUPPRESSION`. Supprimer un
  fichier inexistant lève `FichierIntrouvable`.
- **Dans le socle**, quand aucun secteur n'est disponible, lever
  `DisquePlein`. L'extension E2 récupérera l'espace des versions périmées.
  La flash doit compter au moins 3 secteurs : un actif, un de réserve (E2)
  et un à récupérer.

**Test exigé** (`tests/test_fs.cpp`) : **démonter puis remonter** une
autre instance sur la même flash, après des écritures, des remplacements et
une suppression, et retrouver exactement les mêmes fichiers.

*Tests suggérés* : fichier vide, nom invalide, fichier trop gros, remplir la
flash jusqu'à `DisquePlein` (sans l'extension E2).

## Étape 7 — La poignée `Fichier` (RAII et move-only) [SOCLE]

Une **poignée** de fichier (en anglais, *file handle*) est l'objet par
lequel on agit sur un fichier ouvert, sans être le fichier lui-même. Vous
en avez manipulé au Projet C : le `FILE *` renvoyé par `fopen()`, ou le
descripteur `int` renvoyé par `open()`. La poignée `Fichier` joue le même
rôle, avec deux progrès qui sont l'objet de cette étape :

| | `FILE *` en C | `Fichier` en C++ |
| --- | --- | --- |
| Fermeture | `fclose()` à ne pas oublier | **automatique** quand la variable disparaît (RAII) |
| Copie | le pointeur se copie : deux copies désignent le même fichier, et un double `fclose()` plante | **interdite** (ne compile pas) : on ne peut que **transférer** la poignée (`std::move`) |

`ouvrir()` renvoie une poignée d'**écriture** (la lecture passe par
`lireFichier()`) :

```cpp
{
    Fichier f = fs.ouvrir("config.txt");
    f.ecrire("vitesse=3\n");
    f.ecrire("mode=eco\n");
}   // ← fin de portée : le fichier est validé AUTOMATIQUEMENT (un seul enregistrement)
```

Exigences :

- une seule méthode d'ajout, `ecrire(const std::string& texte)`, qui
  complète le contenu ;
- les données sont **tamponnées en mémoire** et le fichier
  n'est écrit sur la flash qu'**à la fermeture**, en un seul
  enregistrement. Un fichier est donc toujours entièrement ancien ou
  entièrement nouveau ;
- `fermer()` valide explicitement et **peut lever une exception**
  (`DisquePlein`, `CoupureCourant`…) ;
- le **destructeur** ferme le fichier s'il ne l'a pas déjà été. Un
  destructeur ne doit **jamais** laisser échapper d'exception : il
  l'attrape et la signale sur `std::cerr`. C'est pourquoi un code soigneux
  appelle `fermer()` explicitement ;
- `Fichier` est **non copiable** (deux copies valideraient deux fois) mais
  **déplaçable par construction** (`Fichier b = std::move(a);`) : on peut
  le renvoyer depuis une fonction ou le ranger dans un `std::vector` avec
  `push_back`. Après un déplacement, l'objet d'origine est vide et sa
  destruction ne fait rien.

**La règle de 5** : vous déclarez explicitement les cinq opérations
spéciales. Dans le socle, quatre sont décidées simplement :

| Opération | Dans le socle |
| --- | --- |
| constructeur de copie | `= delete` |
| affectation par copie | `= delete` |
| constructeur de déplacement | **à écrire** (`noexcept`) : reprendre le contenu, vider la source |
| affectation par déplacement (`f = std::move(g);`) | `= delete` (l'écrire est un bonus E4) |
| destructeur | **à écrire** : fermer si ouvert, sans laisser échapper d'exception |

**Tests exigés** : validation automatique en fin de portée ; un `Fichier`
déplacé ne valide qu'une fois ; vérification **à la compilation** que
`Fichier` n'est pas copiable
(`static_assert(!std::is_copy_constructible_v<Fichier>)`).

*Test suggéré* : aucun enregistrement sur la flash tant que le fichier est
ouvert.

**Tag `v0.2`** (fin de journée 1) : `SystemeFichiers` et `Fichier` avec
leurs tests (dont le remontage), **Valgrind propre** sur les tests.

---

# PHASE 2 — Coupures, concurrence, finalisation (journée 2)

> Objectif : prouver que le système résiste à une coupure de courant à
> n'importe quel instant et qu'il supporte plusieurs threads (le matin).
> L'après-midi sert à consolider le socle, à le documenter, puis aux
> extensions.

## Étape 1 — `FlashInstable` : simuler une coupure de courant [SOCLE]

`FlashInstable` est un **décorateur**. C'est une `BlockDevice` qui
**possède** une autre `BlockDevice` et lui transmet toutes les opérations,
jusqu'à la coupure :

```cpp
FlashInstable(std::unique_ptr<BlockDevice> interne, std::uint64_t coupureAuProgramme);
std::unique_ptr<BlockDevice> recupererInterne();   // « on rebranche la carte »
```

- Lors du **N-ième appel** à `programmer()` (N = `coupureAuProgramme`,
  compté à partir de 1), seule la **première moitié** des octets demandés
  est réellement programmée, puis `CoupureCourant` est levée. C'est une
  écriture déchirée, comme dans la réalité.
- Après la coupure, **toute** opération lève `CoupureCourant` : la carte
  est éteinte.
- `recupererInterne()` **transfère la propriété** de la mémoire interne à
  l'appelant (`std::move`), qui peut remonter un système de fichiers dessus
  pour simuler le redémarrage.

Avec l'extension E3, l'option `--coupure N` du shell enveloppera l'image
dans une `FlashInstable`.

## Étape 2 — Le test de coupures exhaustif [SOCLE]

C'est le test central du projet. Il doit se trouver dans le fichier
**`tests/test_coupures.cpp`** (exécutable `build/test_coupures`), car c'est
sous ce nom que le formateur le lance :

```text
Scénario S : une séquence fixe d'au moins 20 opérations, mélangeant
créations, remplacements et suppressions.
  - socle : flash assez grande pour que S tienne sans récupération d'espace
    (par exemple 8 secteurs de 1024 octets) ;
  - avec l'extension E2 : flash réduite (par exemple 6 secteurs de 512 octets)
    pour que S la remplisse plusieurs fois et traverse des récupérations.

Pour N = 1, 2, 3, … :
    flash ← FlashInstable(FlashRam neuve, coupure au programme N)
    formater, puis jouer S jusqu'à ce que CoupureCourant soit levée à l'opération k
    (si S se termine sans coupure : c'est terminé, toutes les coupures possibles ont été testées)
    ram ← flash.recupererInterne()          // on rebranche la carte
    fs ← SystemeFichiers(*ram)              // on remonte
    VÉRIFIER : l'ensemble {nom → contenu} de fs est égal
               soit à l'état attendu AVANT l'opération k,
               soit à l'état attendu APRÈS l'opération k
    VÉRIFIER : fs.verifier() est cohérent        (si vous avez fait l'extension E1)
    VÉRIFIER : on peut encore écrire un fichier et le relire
```

Les états attendus sont calculés par un **oracle** : un simple
`std::map<std::string, std::string>` mis à jour à chaque opération réussie
de S. Le test doit parcourir **toutes** les positions de coupure possibles
du scénario (plusieurs dizaines), vérifier qu'il y en a bien eu, et
**afficher leur nombre** : c'est la preuve visible que toutes les coupures
ont été essayées.

C'est **le cœur du projet** : un socle sans ce test vert n'est pas un socle.

## Étape 3 — Accès concurrent [SOCLE]

`SystemeFichiers` devient **utilisable depuis plusieurs threads** :

- **un seul `std::mutex`** protège l'index et la flash. Chaque méthode
  publique le prend au début avec un `std::lock_guard`, qui le rend tout
  seul à la sortie de la méthode, même en cas d'exception (RAII) ;
- attention : une méthode publique qui en appelle une autre ne doit pas
  reprendre le verrou qu'elle tient déjà (le thread se bloquerait contre
  lui-même). Séparez les méthodes publiques (qui verrouillent) des
  fonctions internes (qui ne verrouillent pas) ;
- un `Fichier` en écriture tamponne ses données **sans verrou** : seule la
  validation (`fermer()`) prend le verrou ;
- deux validations concurrentes du même nom : la dernière validée gagne,
  sans corruption.

**Test exigé** (`tests/test_concurrence.cpp`) : **2 threads écrivains**,
chacun réécrivant 100 fois son propre fichier, et **1 thread lecteur** qui
liste et relit en boucle. À la fin, chaque fichier contient sa dernière
version. Dans un thread, utilisez `VERIFIER` (pas `EXIGER`). Le test doit
passer sous **ThreadSanitizer sans aucune alerte** :

```bash
cmake -S . -B build-tsan -DSANITIZE=thread && cmake --build build-tsan -j
ctest --test-dir build-tsan --output-on-failure
```

**Tag `v0.3`** (fin de matinée du jour 2) : `FlashInstable`, test de
coupures exhaustif vert, accès concurrent et test TSan propre.

## Étape 4 — Finaliser le socle [SOCLE]

L'après-midi du jour 2 commence par la consolidation, **avant** toute
extension :

1. passer `valgrind --leak-check=full` sur les tests, puis construire et
   lancer les tests sous ASan (`-DSANITIZE=address`) et TSan
   (`-DSANITIZE=thread`) ; corriger ce qui sort ;
2. vérifier que le code respecte `FORMAT.md`, et écrire le `README.md` (compilation,
   architecture, choix, limites connues, extensions réalisées) ;
3. rejouer, sur un clone neuf de `main`, le parcours de vérification de
   l'étape 6.

Ensuite seulement, les extensions (étape 5).

**Tag `v1.0`** (fin de journée 2) : socle complet et propre, `README.md`
finalisé, et les extensions réalisées, sur une branche `main` propre. **C'est
ce tag qui est évalué** : rien d'autre n'est à livrer ensuite (étape 6).

## Étape 5 — Extensions [EXTENSION]

À n'aborder **qu'une fois le socle terminé et fusionné sur `main`** (étape 4).
Elles sont indépendantes les unes des autres. Chaque extension réalisée est
testée, documentée (dans le `README.md`) comme le reste du projet, puis
fusionnée sur `main` par Merge Request. Une extension inachevée, ou qui casse
un test, n'est pas fusionnée : elle reste sur votre branche `dev/*` (non
évaluée), et le `README.md` peut la mentionner. Le tag `v1.0` se pose en
dernier, extensions comprises.

### E1 — `fsck` : vérifier la cohérence

`SystemeFichiers::verifier()` renvoie un rapport (et, avec l'extension E3,
la commande `fsck` du shell l'affiche) :

- nombre de secteurs libres, utilisés et sales ;
- nombre d'enregistrements valides, **non validés** (écritures
  interrompues), et de versions périmées ;
- pour chaque fichier courant, **relecture complète et contrôle du CRC**
  des données ;
- verdict : **cohérent** ou **incohérent**.

Une écriture interrompue par une coupure est **normale et sans danger**.
Elle est signalée, mais le verdict reste cohérent. Un enregistrement
**validé** dont le CRC est faux est en revanche une corruption, et le
verdict est alors incohérent (code 7).

Tests : fichier **`tests/test_fsck.cpp`**. Ajoutez aussi l'appel à `verifier()` dans votre
test de coupures.

### E2 — Récupération d'espace et répartition de l'usure

Quand il faut mettre en service un secteur et qu'il n'en reste qu'**un seul**
libre ou sale, ce dernier est une **réserve** : on récupère d'abord de la
place.

1. La **victime** est le secteur utilisé le plus ancien (plus petite
   `sequence` de secteur), hors secteur actif.
2. Ses enregistrements **encore vivants** (les versions courantes) sont
   **recopiés** vers le secteur actif, en utilisant la réserve si
   nécessaire. La copie conserve la `sequence` d'origine.
3. Puis la victime est **effacée** : elle redevient libre.
4. On recommence si nécessaire. S'il n'y a plus rien à récupérer, on lève
   `DisquePlein`.

Pourquoi c'est sûr face aux coupures : la victime n'est effacée **qu'après**
que ses données vivantes ont été recopiées et validées. Si le courant est
coupé entre les deux, les données existent en double avec la même
`sequence`, ce qui est sans conséquence au remontage. Votre test de coupures
exhaustif doit donc **traverser des récupérations d'espace**.

**Usure.** Au lieu du premier secteur libre (socle), on met en service le
secteur libre ou sale **le moins usé** (le moins effacé, voir
`nbEffacements`). Comme on récupère toujours le plus ancien, l'usure se
répartit. **Test
attendu** : sur 8 secteurs, écrire 3 000 fois un fichier de 200 octets, puis
vérifier que l'écart entre le secteur le plus usé et le moins usé est
inférieur ou égal à 2 effacements. (Avec E3 niveau 2, la commande `stats`
affiche cette usure.)

Tests : fichier **`tests/test_usure.cpp`**. Réduisez la flash du test de coupures pour qu'il
traverse des récupérations d'espace.

### E3 — Persistance et shell

Comme les trames au Projet C, cette extension se fait en deux niveaux. Le
niveau 1 suffit à la valoriser.

#### Niveau 1 — Image sur disque, shell minimal, démo `--coupure`

##### La persistance : `FlashFichier`

La flash est persistée dans un **fichier image brut** : exactement
`nbSecteurs × tailleSecteur` octets, le contenu de la flash et rien d'autre.

- `FlashFichier::creer(chemin, nbSecteurs, tailleSecteur)` crée une image
  neuve (tout à `0xFF`) ; `FlashFichier::ouvrir(chemin, nbSecteurs,
  tailleSecteur)` ouvre une image existante et lève `ImageInvalide` si elle
  est absente ou si sa taille ne correspond pas à la géométrie. Les deux
  renvoient un `std::unique_ptr<FlashFichier>`.
- Les compteurs d'usure ne sont **pas persistés** au niveau 1 : ils
  repartent de 0 à chaque ouverture. C'est sans conséquence tant que
  l'extension E2 n'est pas faite. Le niveau 2 ajoute une image avec
  en-tête et usure persistée.
- L'objet garde le `std::fstream` ouvert pendant toute sa vie (RAII) et
  **répercute chaque programmation et chaque effacement dans le fichier
  immédiatement** (`flush`). Ainsi, un processus tué brutalement laisse une
  image cohérente avec ce qui a été écrit.

##### Le shell

```
flashfs <image> [--creer] [--secteurs N] [--taille-secteur T] [--endurance E] [--coupure N]
```

- `--creer` crée une image neuve, sinon l'image existante est ouverte. La
  géométrie est donnée par `--secteurs` et `--taille-secteur` (par défaut 16
  secteurs de 4096 octets). Au niveau 1, il faut donc rouvrir une image avec la
  géométrie qui a servi à la créer.
- Les commandes sont lues sur l'**entrée standard**, une par ligne. Cela
  permet aussi bien l'usage interactif que les scripts
  (`flashfs disque.img < scenario.txt`). Le prompt `flashfs> ` n'est
  affiché que si l'entrée standard est un terminal (`isatty(0)`).

| Commande | Effet | Niveau d'E3 |
| --- | --- | --- |
| `ls` | liste les fichiers (nom, taille), triés par nom | 1 |
| `write <nom> <texte…>` | crée ou remplace le fichier avec le reste de la ligne | 1 |
| `read <nom>` | affiche le contenu | 1 |
| `rm <nom>` | supprime | 1 |
| `quit` (ou fin de l'entrée) | — | 1 |
| `import <nom> <chemin hôte>` | copie un fichier de la machine hôte dans la flash | 2 |
| `export <nom> <chemin hôte>` | copie un fichier de la flash vers l'hôte | 2 |
| `format` | efface toute la flash | 2 |
| `help` | liste les commandes | 2 |
| `stats` | secteurs libres / utilisés / sales, usure min / max / moyenne, espace libre, nombre de fichiers | 2 |
| `fsck` | rapport de cohérence | E1 |

- **`--coupure N`** : l'image est enveloppée dans une `FlashInstable`. À la
  coupure, le shell s'arrête immédiatement avec le code **6**, et l'image
  contient exactement l'état de la flash à cet instant. C'est la **démo
  coupure au shell** (annexe A.4).

- Le shell est une **table de commandes** : une `std::map` qui associe
  le nom de chaque commande à une `std::function`, remplie de lambdas. Par
  exemple, `std::map<std::string, std::function<int(const Ligne&)>>`, où
  la lambda renvoie son code retour. Aucune chaîne de `if` / `else if` sur
  le nom de la commande.
- Une commande en erreur affiche un message sur `stderr` et le shell
  continue. En fin d'exécution, **le code retour du programme est celui de
  la dernière erreur** (0 si aucune).
- **Codes retour normalisés** (le code 3 relève du niveau 2, le code 7 de
  l'extension E1) : `0` succès, `1` usage, `2` image invalide,
  `3` E/S (fichier hôte, ou erreur matérielle de la flash), `4` fichier
  introuvable ou nom invalide, `5` disque plein ou
  fichier trop gros, `6` coupure de courant, `7` incohérence détectée par
  `fsck`. La correspondance exception → code est faite **en un seul
  endroit**.

Test suggéré : un script du shell passé sous Valgrind.

#### Niveau 2 — Shell et image complets

- Commandes `import`, `export`, `format`, `stats` et `help` (voir le tableau
  du niveau 1), avec l'exception `ErreurHote` et le code retour 3.
  `SystemeFichiers` gagne `statistiques()` et une méthode `formater()` qui
  efface puis remonte.
- **Image avec en-tête et usure persistée** : la géométrie est lue dans
  l'image, sans avoir à la redonner en option.

  ```
  Fichier image
    en-tête (16 octets) : magic "FIMG", tailleSecteur u32, nbSecteurs u32, enduranceMax u32
    compteurs d'usure   : nbSecteurs × uint32
    contenu de la flash : nbSecteurs × tailleSecteur octets
  ```

  `FlashFichier::ouvrir(chemin)` lève `ImageInvalide` si le magic est faux
  ou si la taille ne correspond pas à l'en-tête.

### E4 — Bonus

- **`CacheLRU<Cle, Valeur>`** : un template de cache des derniers blocs
  lus, avec un test sur deux types différents ;
- **coupure pendant un effacement** : `FlashInstable` coupe aussi au N-ième
  `effacer()` en laissant un secteur à moitié effacé (qui devient sale) ;
- **`--coupure-aleatoire <graine>`** : position de coupure tirée par
  `std::mt19937`, reproductible avec la même graine ;
- **pipeline GitLab CI** : build, tests, tests sous ASan et sous TSan ;
- **documentation Doxygen** des en-têtes publics ;
- **répertoires** : noms avec `/`, et `ls <préfixe>` ;
- **visualiseur Python** qui lit l'image et dessine l'usure par secteur ;
- **`Fichier` en lecture** : `ouvrir(nom, Mode::Lecture)`, `lireTout()`,
  et `ecrire()` qui lève une exception ;
- **affectation par déplacement de `Fichier`** : `f = fs.ouvrir("autre");`
  valide d'abord le fichier que `f` tenait, puis reprend le nouveau
  (attention à `f = std::move(f);`). Avec son test ;
- **`ecrire()` pour des octets** : une surcharge
  `ecrire(const std::vector<std::uint8_t>&)` ;
- **verrou lecteurs / écrivain** : remplacer le `std::mutex` par un
  `std::shared_mutex`. Les lectures prennent un verrou **partagé**
  (`std::shared_lock`, plusieurs lecteurs ensemble), les modifications un
  verrou **exclusif** (`std::unique_lock`). Test : 4 écrivains et 2 lecteurs,
  toujours sous TSan ;
- **CRC d'en-tête séparé** : permet de sauter un enregistrement dont seules
  les données sont corrompues, au lieu de perdre la fin du secteur.

## Étape 6 — Rendu final [SOCLE]

**Pas de démonstration ni de vidéo.** Le rendu, c'est la **branche `main`**
de votre dépôt GitLab, taguée **`v1.0`**, en fin de journée 2. Le formateur
la clone et vérifie lui-même le travail. Aucune livraison n'est attendue
après la fin du module : le code n'évolue plus après le tag `v1.0`.

Une branche `main` **propre**, c'est :

- **tout est fusionné** : le travail des branches `dev/<prénom>` est arrivé
  sur `main` par Merge Request. Ce qui n'est que sur une branche `dev/*`
  n'est pas évalué ;
- le **dernier commit de `main` porte le tag `v1.0`**, poussé sur GitLab
  (`git push origin v1.0`) ;
- **aucun fichier parasite** : pas de dossier `build*/`, d'image `*.img`, de
  fichiers d'éditeur (`.vscode/`, `.idea/`), d'exécutable ni de fichier
  temporaire. Le `.gitignore` du squelette les exclut déjà : ne le
  contournez pas ;
- **pas de code mort** : pas de gros blocs commentés, d'affichages de
  débogage oubliés ni de fichiers de brouillon ;
- un **`README.md` à jour** : compilation, architecture, choix, limites
  connues, **état du socle** et extensions réalisées.

**Ce que le formateur lance**, sur un clone neuf de `main`, sous Debian :

1. build depuis zéro (`cmake` + `cmake --build`), zéro warning ;
2. `ctest` ;
3. `./build/test_coupures`, qui doit afficher le nombre de positions de
   coupure essayées, toutes cohérentes ;
4. Valgrind sur les tests, puis les tests sous ASan et TSan ;
5. les extensions décrites dans le `README.md`, s'il y en a.

Faites ce parcours vous-mêmes avant de poser le tag, dans un dossier vide :

```bash
git clone https://gitlab.com/<groupe>/flashfs.git verif && cd verif
cmake -S . -B build && cmake --build build -j && ctest --test-dir build
./build/test_coupures
```

Si une partie du socle n'est pas terminée, dites-le **honnêtement** dans le
`README.md` : ce qui marche, ce qui manque, et comment vous l'auriez
terminé. Ce qui manque est compté comme manquant, mais un `README.md`
lucide est apprécié, alors qu'un problème caché ne l'est pas.

---

## Checklist finale du projet

**Socle (tout est exigé) :**

- [ ] Dépôt GitLab : branches par membre, fusions par MR, tags `v0.1` → `v1.0`
- [ ] Build Linux depuis zéro : `cmake` + `cmake --build`, **zéro warning** (`-Werror`)
- [ ] **Aucun `new` ni `delete`** dans `src/` et `include/`
- [ ] `ctest` vert, avec au moins : **`Coupures`** (exhaustif, nombre de coupures affiché), **`Concurrence`**, remontage, `Fichier` (fin de portée, déplacement)
- [ ] **Valgrind : 0 fuite** sur les tests
- [ ] **ThreadSanitizer : 0 alerte** sur les tests (dont `Concurrence` : 2 écrivains, 1 lecteur, un `std::mutex`)
- [ ] `Fichier` non copiable (vérifié par `static_assert`), déplaçable, validé en fin de portée
- [ ] Le code respecte `FORMAT.md` (fourni)
- [ ] `README.md` : usage, architecture, limites, **extensions réalisées**
- [ ] Branche `main` propre (tout fusionné, aucun fichier parasite), tag `v1.0` sur son dernier commit

**Extensions (cochez celles réalisées) :**

- [ ] E1 `fsck` (`tests/test_fsck.cpp`, code 7)
- [ ] E2 récupération d'espace et usure (`tests/test_usure.cpp`, test de coupures qui la traverse)
- [ ] E3 niveau 1 : image sur disque, shell minimal (`ls`, `write`, `read`, `rm`), `--creer`, `--coupure`, codes retour
- [ ] E3 niveau 2 : `import`, `export`, `format`, `stats`, `help`, image avec en-tête FIMG
- [ ] E4 bonus : …

---

## Rappels et pièges

- **Octet `etat` en dernier.** Si vous le programmez avec l'en-tête, une
  coupure pendant les données laisse un enregistrement « validé » mais
  tronqué. Au montage, son CRC faux le fait ignorer quand même : le test de
  coupures **reste vert**, il ne suffit donc pas à prouver que l'ordre est
  bon. La différence se voit avec `fsck` (extension E1) : un enregistrement
  non validé est une simple coupure, un enregistrement validé au CRC faux
  est une **corruption** (code 7). Écrire `etat` en dernier, c'est ce qui
  permet de distinguer les deux.
- **Vérifier avant d'écrire.** Une programmation refusée
  (`ProgrammationInterdite`) ne doit rien modifier. Contrôlez tous les
  octets, puis écrivez.
- **Débordement d'adresse.** Testez `adresse + n > taille`, pas seulement
  `adresse >= taille`.
- **Destructeur et exceptions.** Un destructeur qui laisse échapper une
  exception provoque `std::terminate`. Attrapez tout dans `~Fichier()`.
- **Objet déplacé.** Après `Fichier b = std::move(a);`, `a` ne doit plus
  rien valider à sa destruction. Pensez à « vider » la source dans le
  constructeur de déplacement (et dans l'affectation par déplacement, si
  vous faites ce bonus).
- **Référence vers la flash.** Le système de fichiers garde une référence :
  détruire la flash avant lui est un comportement indéfini. ASan le détecte
  (`-DSANITIZE=address`).
- **Pas de `struct` brute sur la flash** : sérialisez champ par champ, en
  little-endian (même piège qu'au Projet C).
- **Longueurs corrompues.** À la relecture, ne faites jamais confiance à
  `taille` ou `lgNom` avant d'avoir vérifié qu'ils restent dans le secteur,
  sous peine de lecture hors limites sur une image abîmée.
- **Portabilité Windows → Linux.** N'oubliez aucun `#include` (`<array>`,
  `<algorithm>`, `<cstdint>`…) : MSVC et MinGW en incluent certains
  implicitement, g++ non.
- **Limite assumée du format.** Le CRC d'un enregistrement couvre à la fois
  l'en-tête et les données. Si un octet est altéré hors coupure (corruption
  de la flash), on ne peut plus se fier à la longueur de l'enregistrement :
  la fin du secteur concerné est perdue. `fsck` doit alors signaler une
  incohérence (code 7). Un CRC séparé pour l'en-tête lèverait cette limite
  (voir les extensions).
- **Verrou repris deux fois.** Avec un `std::mutex`, une méthode publique
  verrouillée qui appelle une autre méthode publique verrouillée bloque le
  programme pour toujours (le test ne se termine jamais). Symptôme :
  `ctest` s'arrête sur un délai dépassé.
- **TSan et `std::cout`.** Écrire sur `std::cout` depuis plusieurs threads
  sans verrou mélange les lignes. Dans les tests de concurrence, évitez les
  affichages.

---

# Annexe — Exemples d'utilisation et de sorties

> Ces sorties proviennent d'une implémentation de référence. La mise en
> forme est libre ; **l'information affichée et les codes retour sont
> imposés**.

## A.0 — Vérification du socle (par les tests)

```
$ ctest --test-dir build
…
100% tests passed, 0 tests failed out of 8

$ ./build/test_coupures
[coupures] 137 positions de coupure testées, toutes cohérentes
[ OK    ] Coupures.toutesLesPositionsDeCoupure
Tous les tests sont verts.

$ valgrind --leak-check=full ./build/test_fs
…
==213== All heap blocks were freed -- no leaks are possible
==213== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

Le nombre de fichiers de tests et de coupures dépend de votre scénario ;
l'implémentation de référence couvre aussi les extensions.

---

**Les exemples suivants (A.1 à A.6) relèvent de l'extension E3** (le shell).

Codes retour : `0` succès, `1` usage, `2` image invalide, `3` E/S, `4`
introuvable ou nom invalide, `5` disque plein ou fichier trop gros, `6`
coupure de courant, `7` incohérence (`fsck`).

## A.1 — Aide et usage

```
$ flashfs
flashfs 0.1 — système de fichiers pour mémoire flash simulée
Usage : flashfs <image> [options]   (commandes lues sur l'entrée standard)

Options :
  --creer               crée une image neuve (écrase l'existante)
  --secteurs N          nombre de secteurs (défaut 16)
  --taille-secteur T    taille d'un secteur en octets (défaut 4096)
  --endurance E         effacements max par secteur (défaut 1000)
  --coupure N           coupure de courant à la N-ième programmation
$ echo $?
1
```

## A.2 — Session type (script sur l'entrée standard)

*Exemples produits avec E3 niveau 2 (image avec en-tête, commandes `import` et `stats`). Au niveau 1, on ajoute la géométrie aux options quand on rouvre une image existante.*

```
$ cat session.txt
write config.txt vitesse=3 mode=eco
write notes.txt bonjour
import reglages.txt reglages.txt
ls
read config.txt
write config.txt vitesse=4 mode=turbo
read config.txt
rm notes.txt
ls
stats

$ flashfs disque.img --creer < session.txt
reglages.txt : 33 octets importés
  config.txt                          18 octets
  notes.txt                            7 octets
  reglages.txt                        33 octets
3 fichier(s)
vitesse=3 mode=eco
vitesse=4 mode=turbo
  config.txt                          20 octets
  reglages.txt                        33 octets
2 fichier(s)
secteurs     : 16 (libres 15, utilisés 1, sales 0)
usure        : min 0, max 0, moyenne 0.00 effacement(s)
fichiers     : 2 (53 octets)
espace libre : 60992 octets
$ echo $?
0
```

L'espace libre exclut le secteur de réserve.

## A.3 — Erreurs

```
$ printf "read absent.txt\nwrite nom/invalide x\n" | flashfs disque.img
ERREUR : fichier 'absent.txt' introuvable
ERREUR : nom invalide 'nom/invalide' (1 à 32 caractères parmi [A-Za-z0-9._-])
$ echo $?
4

$ echo "import gros.bin gros.bin" | flashfs disque.img      # gros.bin : 5000 octets
ERREUR : 'gros.bin' : 5000 octets, maximum 4056
$ echo $?
5

$ echo ls | flashfs pas_une_image.img                   # image à en-tête (E3)
ERREUR : 'pas_une_image.img' n'est pas une image flashfs (magic FIMG attendu)
$ echo $?
2
```

## A.4 — La démo coupure de courant

*E3 niveau 1, sauf la commande `fsck` (extension E1).*

```
$ echo "write config.txt vitesse=5 mode=boost" | flashfs disque.img --coupure 1
ERREUR : coupure de courant simulée à la programmation n°1
$ echo $?
6

$ printf "read config.txt\nfsck\n" | flashfs disque.img      # redémarrage
vitesse=4 mode=turbo                                         ← l'ancienne version, intacte
secteurs        : libres 15, utilisés 1, sales 0
enregistrements : 5 valides, 3 périmés, 1 non validé(s) (écriture interrompue, ignorée)
fichiers        : 2 relus, 0 corrompu(s)
verdict         : COHÉRENT
$ echo $?
0

$ echo "write config.txt vitesse=5 mode=boost" | flashfs disque.img   # on recommence, sans coupure
$ printf "read config.txt\nfsck\n" | flashfs disque.img
vitesse=5 mode=boost
secteurs        : libres 14, utilisés 2, sales 0                    ← le secteur coupé est « plein »
enregistrements : 6 valides, 4 périmés, 1 non validé(s) (écriture interrompue, ignorée)
fichiers        : 2 relus, 0 corrompu(s)
verdict         : COHÉRENT
```

## A.5 — Corruption détectée par `fsck` (extension E1)

Ici, un octet de données du fichier `reglages.txt` a été altéré dans
l'image avec `dd`. Ce n'est pas une coupure : la flash elle-même est
corrompue.

```
$ echo fsck | flashfs abime.img
secteurs        : libres 14, utilisés 2, sales 0
enregistrements : 3 valides, 1 périmés, 0 non validé(s) (écriture interrompue, ignorée)
fichiers        : 2 relus, 1 corrompu(s)
  ! secteur 0, position 92 : enregistrement validé mais invalide
verdict         : INCOHÉRENT
$ echo $?
7
```

## A.6 — Répartition de l'usure (extensions E2 et E3)

```
$ yes "write compteur.dat 0123456789" | head -2000 > usure.txt ; echo stats >> usure.txt
$ flashfs usure.img --creer --secteurs 8 --taille-secteur 1024 < usure.txt
secteurs     : 8 (libres 1, utilisés 7, sales 0)
usure        : min 8, max 9, moyenne 8.75 effacement(s)
fichiers     : 1 (10 octets)
espace libre : 96 octets
```

Deux mille réécritures du même fichier se sont réparties sur les 8
secteurs : aucun secteur n'est plus usé que les autres.

