# FORMAT.md — format de flashfs sur la flash

> **FOURNI.** Ce document précise, octet par octet, la spécification cadre de
> l'énoncé. C'est votre **contrat** : votre code doit le respecter
> exactement, et le formateur le relit avec lui. Comme au Projet C, gardez-le
> ouvert pendant que vous codez le montage et l'écriture (étape 6).

## 1. Conventions

- Tous les entiers sont des `uint32` **little-endian**, écrits champ par
  champ avec `ecrireU32` / `lireU32`. On n'écrit jamais de `struct` brute.
- CRC : CRC32 IEEE (`crc32` fourni). Le CRC de `123456789` vaut `CBF43926`.
- Octet vierge : `0xFF`. Seule la programmation 1 → 0 est possible sans
  effacement.

## 2. En-tête de secteur (16 octets, à l'offset 0 du secteur)

| Offset | Taille | Champ | Valeur |
|---|---|---|---|
| 0 | 4 | `magic` | `"FFS1"` |
| 4 | 4 | `sequence` | ordre de mise en service, 1, 2, 3… Un numéro n'est jamais réattribué : un secteur effacé puis remis en service reçoit un **nouveau** numéro (plus grand déjà vu + 1) |
| 8 | 4 | `reserve` | `FFFFFFFF` |
| 12 | 4 | `crc32` | CRC des octets 0 à 11 |

## 3. Enregistrement (16 octets d'en-tête, puis nom, puis données)

| Offset | Taille | Champ | Valeur |
|---|---|---|---|
| 0 | 1 | `etat` | `FF` non validé, `00` validé (programmé **en dernier**) |
| 1 | 1 | `type` | `01` FICHIER, `02` SUPPRESSION |
| 2 | 1 | `lgNom` | 1 à 32 |
| 3 | 1 | `reserve` | `FF` |
| 4 | 4 | `sequence` | numéro global d'opération, croissant |
| 8 | 4 | `taille` | octets de données (`0` pour SUPPRESSION) |
| 12 | 4 | `crc32` | voir ci-dessous |
| 16 | `lgNom` | `nom` | sans zéro terminal |
| 16 + `lgNom` | `taille` | `donnees` | contenu complet du fichier |

**Octets couverts par le CRC, dans cet ordre** : octets 1 à 11 de l'en-tête
(`type`, `lgNom`, `reserve`, `sequence`, `taille`), puis le `nom`, puis les
`donnees`. Calcul incrémental : `crc32(nom+donnees, n, crc32(entete+1, 11))`.
L'octet `etat` est **exclu**, puisqu'il change après le calcul.

Les enregistrements se suivent sans alignement ni bourrage : le suivant
commence à `position + 16 + lgNom + taille`.

**Exemple** : premier fichier `a.txt` contenant `hi`, dans le secteur 0.

```
offset 0   46 46 53 31  01 00 00 00  ff ff ff ff  74 ce 0e e3   en-tête de secteur, sequence 1
offset 16  00 01 05 ff  01 00 00 00  02 00 00 00  6a db 0d e8   etat 00, FICHIER, lgNom 5, seq 1, taille 2
offset 32  61 2e 74 78 74  68 69                                "a.txt" puis "hi"
offset 39  ff ff ff …                                            zone libre
```

## 4. Règle de nommage

1 à 32 caractères parmi `[A-Za-z0-9._-]`, sensible à la casse. Tout autre
nom lève `NomInvalide`, à l'ouverture comme à l'écriture ou à la suppression.

## 5. Taille maximale d'un fichier

Un enregistrement tient toujours dans un seul secteur vide :
`taille max = tailleSecteur − 16 (secteur) − 16 (enregistrement) − lgNom`.

Géométrie par défaut (16 secteurs de 4096 octets) : **4064 − lgNom**, soit de
**4032 octets** (nom de 32 caractères) à **4063 octets** (nom d'un caractère).
Au-delà : `FichierTropGros`.

## 6. Écriture

1. Construire l'enregistrement en mémoire avec `etat = FF`, CRC calculé.
2. Programmer l'enregistrement entier à la fin de la zone écrite du secteur
   actif.
3. Programmer le **seul octet `etat`** de `FF` à `00`. L'index en mémoire
   n'est mis à jour qu'après cette étape.

Si l'enregistrement ne tient pas dans le secteur actif, on met en service le
secteur libre (ou sale, alors effacé d'abord) le moins usé (E2 ; le socle
prend simplement le premier), avec
`sequence = plus grande séquence de secteur + 1`. Un secteur reste toujours
en réserve pour la récupération d'espace (E2), qui recopie les fichiers
vivants du secteur le plus ancien **avec leur séquence d'origine**, puis
seulement l'efface.

## 7. Relecture au montage : cas normaux et anormaux

Chaque secteur est lu en entier, puis ses enregistrements un par un à partir
de l'offset 16.

| Situation rencontrée | Cause probable | Traitement |
|---|---|---|
| Secteur entièrement à `FF` | secteur vierge | **Libre** |
| Magic différent de `FFS1` ou CRC d'en-tête faux | coupure pendant l'en-tête, ou déchet | **Sale** : ignoré, effacé avant réutilisation |
| Moins de 16 octets avant la fin du secteur | secteur rempli | fin du secteur |
| 16 octets d'en-tête d'enregistrement tous à `FF` | fin de la zone écrite | fin du secteur, on écrira ici |
| `type` inconnu, SUPPRESSION avec `taille ≠ 0`, `lgNom` hors 1..32, ou nom + données qui débordent du secteur | longueurs non fiables | enregistrement **douteux** (ligne suivante). Les longueurs sont bornées **avant** toute lecture |
| `etat = FF` (non validé) | coupure pendant l'écriture : **normal** | ignoré, compté « non validé ». Secteur marqué **plein** |
| `etat = 00` mais forme ou CRC faux, ou `etat` ni `00` ni `FF` | **corruption** (la validation se fait en dernier) | ignoré, compté « corrompu », signalé par `fsck`. Secteur marqué **plein** |
| Enregistrement valide | — | retenu, on saute au suivant |

« Plein » veut dire qu'on arrête de lire ce secteur, car on ne peut plus se
fier à une longueur pour trouver l'enregistrement suivant, et qu'on n'y écrit
plus rien. Ce qui précède reste valable.

**Reconstruction de l'état** : les enregistrements valides de tous les
secteurs sont triés par `sequence` (tri stable), puis rejoués. FICHIER fixe la
version courante du nom, SUPPRESSION le retire. Deux enregistrements de même
séquence sont les deux copies d'une récupération interrompue, identiques :
l'un ou l'autre convient.

**Reprise** : prochaine séquence d'opération = plus grande séquence lue + 1.
Secteur actif = secteur utilisé de plus grande séquence de secteur.
