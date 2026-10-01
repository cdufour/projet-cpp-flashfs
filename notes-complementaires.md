# Notes complémentaires

Précisions sur l'énoncé et sur `FORMAT.md`, ajoutées pendant le projet à
partir de vos questions. Elles ne changent pas ce qui est demandé : elles
l'expliquent. La plus récente est en bas.

Pour une vue d'ensemble illustrée : [FlashFS en schémas](https://ajc.opusidea.fr/projet-cpp/schema/).

## Sommaire

1. [Le champ `sequence`](#1-le-champ-sequence) (01/10/2026)
2. [Le montage : collecter, trier, puis rejouer](#2-le-montage--collecter-trier-puis-rejouer) (01/10/2026)

---

## 1. Le champ `sequence`

Le format contient **deux** champs `sequence`, construits sur le même
principe : un numéro qui ne fait qu'augmenter, pour retrouver **l'ordre
chronologique** à partir de la flash seule.

### Pourquoi un tel numéro ?

Dans cette note, **redémarrage** veut dire redémarrage **de l'appareil**,
donc du programme, pas de la flash. À l'extinction :

| | Ce qui se passe | Dans le projet |
|---|---|---|
| **Flash** | **conservée** : en-têtes, enregistrements, données | l'objet `FlashRam` (ou le fichier image en E3) |
| **Mémoire du programme** | **perdue** : index des fichiers, secteur actif, compteurs | l'objet `SystemeFichiers` |

Au redémarrage, le programme repart de zéro et reconstruit tout à partir des
octets de la flash : c'est le **montage**, fait par le constructeur de
`SystemeFichiers`. Dans les tests, on le simule en détruisant le
`SystemeFichiers` puis en en créant un nouveau sur la **même** flash :
c'est le test de remontage exigé.

```cpp
FlashRam flash(8, 512);           // la puce : elle vit pendant tout le test
{
    SystemeFichiers fs(flash);    // l'appareil s'allume (montage)
    fs.ecrireFichier(...);
}                                 // fs détruit : l'appareil s'éteint, sa mémoire est perdue
SystemeFichiers fs2(flash);       // l'appareil se rallume : nouveau montage,
                                  // fs2 ne sait rien et relit tout depuis la flash
```

Au montage, votre programme n'a donc **rien en mémoire** : il ne connaît que
les octets de la flash. Or la **position** d'une donnée sur la flash ne dit
pas *quand* elle a été écrite. Le secteur 0 n'est pas forcément le plus
ancien, et un enregistrement placé loin n'est pas forcément le plus récent
(voir E2 plus bas). La séquence, elle, est écrite **dans** la donnée : elle
survit à l'extinction.

### 1.1 `sequence` de l'en-tête de SECTEUR (offset 4 du secteur)

Elle indique **dans quel ordre les secteurs ont été mis en service**.

**À l'écriture**, dans `mettreEnService()` :

- prendre le premier secteur libre ou sale (l'effacer s'il est sale) ;
- y écrire l'en-tête : `"FFS1"`, `sequence = prochaineSeqSecteur_`,
  `FFFFFFFF`, CRC ;
- incrémenter `prochaineSeqSecteur_`.

**Au montage** :

- **secteur actif** = le secteur *utilisé* qui a la **plus grande** séquence
  de secteur : c'est là qu'on continue d'écrire ;
- `prochaineSeqSecteur_ = cette plus grande valeur + 1`.

**En E2** (récupération d'espace) : le secteur à récupérer est le secteur
utilisé qui a la **plus petite** séquence, hors secteur actif. C'est le plus
ancien, celui qui contient le plus de versions périmées.

**« Un numéro n'est jamais réutilisé »** : chaque numéro n'est attribué
**qu'une fois**. Quand un secteur est effacé puis remis en service, il reçoit
un **nouveau** numéro (le plus grand + 1), jamais son ancien :

```
mise en service    secteur 0 → seq 1
mise en service    secteur 1 → seq 2
mise en service    secteur 2 → seq 3
E2 : secteur 0 récupéré puis effacé, remis en service
                   secteur 0 → seq 4   ✅ (et pas 1 ❌)
```

Avec 1, au prochain démarrage, le secteur 0 passerait pour le **plus
ancien** alors que c'est le **plus récent**. Le secteur 2 serait pris pour
le secteur actif, et la prochaine récupération viserait le secteur 0, celui
qu'on vient de remplir.

Dans le **socle** (sans E2), aucun secteur n'est effacé pour être
réutilisé : les séquences valent simplement 1, 2, 3… dans l'ordre où les
secteurs sont ouverts. Le champ sert à retrouver le secteur actif au
montage, comme dans l'exemple suivant.

#### Exemple dans le socle : le redémarrage de l'appareil

Une flash de 4 secteurs, au moment où l'on éteint l'appareil :

```
secteur 0 : 46 46 53 31  01 00 00 00  ff ff ff ff  74 ce 0e e3   "FFS1", sequence 1 — plein
secteur 1 : 46 46 53 31  02 00 00 00  ff ff ff ff  97 c9 81 6d   "FFS1", sequence 2 — à moitié rempli
secteur 2 : ff ff ff ff …                                         vierge
secteur 3 : ff ff ff ff …                                         vierge
```

Au montage, en lisant l'en-tête de chaque secteur :

1. secteurs 0 et 1 : **utilisés** (signature et CRC corrects), séquences 1
   et 2 ; secteurs 2 et 3 : **libres** ;
2. **secteur actif** = le secteur utilisé de plus grande séquence →
   **secteur 1**. L'écriture reprend à la fin de sa zone écrite ;
3. `prochaineSeqSecteur_` = 2 + 1 = **3**.

Quand le secteur 1 est plein, `mettreEnService()` prend le premier secteur
libre (le 2) et y écrit :

```
secteur 2 : 46 46 53 31  03 00 00 00  ff ff ff ff  09 c9 2b a1   "FFS1", sequence 3
```

#### Variante : le courant coupe pendant l'écriture de cet en-tête

```
secteur 2 : 46 46 53 31  03 00 ff ff  ff ff ff ff  ff ff ff ff   en-tête incomplet
```

Au redémarrage, le CRC du secteur 2 est faux : le secteur est **sale**. Il
n'est ni libre ni utilisé, et il est ignoré.

- Le secteur actif est toujours le **secteur 1** (séquence 2), et
  `prochaineSeqSecteur_` vaut 3.
- À la prochaine mise en service, le premier secteur libre **ou sale** est
  le 2 : on l'**efface**, puis on y écrit un en-tête complet de séquence 3.

**Le piège.** Chercher le secteur actif comme « le dernier secteur non
vierge » désignerait ici le secteur 2, dont l'en-tête est illisible. La
bonne règle est la seule qui marche dans tous les cas : *parmi les secteurs
dont l'en-tête est valide, celui de plus grande séquence.* C'est aussi elle
qui restera juste si vous faites l'extension E2, où un secteur effacé puis
réutilisé peut être le plus récent tout en étant le premier de la flash.

### 1.2 `sequence` d'un ENREGISTREMENT (offset 4 de l'enregistrement)

C'est le **numéro global de l'opération**, commun à toute la flash, tous
secteurs confondus. Chaque écriture de fichier ou suppression prend
`prochaineSequence_`, puis on l'incrémente.

**Au montage** :

1. relire tous les enregistrements **valides** de tous les secteurs ;
2. les **trier par séquence** (tri stable) ;
3. les **rejouer** dans cet ordre : FICHIER → `index_[nom] = …` ;
   SUPPRESSION → `index_.erase(nom)`. **La dernière opération gagne** ;
4. `prochaineSequence_ = plus grande séquence lue + 1`.

Pourquoi trier au lieu de lire dans l'ordre des secteurs ? Parce qu'en E2,
la récupération **recopie** les fichiers encore vivants d'un vieux secteur
vers le secteur actif, **en gardant leur séquence d'origine**. Une vieille
version se retrouve alors physiquement *après* des opérations plus
récentes. Seule la séquence dit la vérité.

```
seq 5   FICHIER     a.txt = "v1"
seq 9   FICHIER     a.txt = "v2"
seq 12  SUPPRESSION a.txt
→ après rejeu : a.txt n'existe pas, quel que soit l'ordre physique
```

**Pourquoi garder la séquence d'origine à la recopie ?** Si le courant coupe
pendant une récupération, la flash contient **deux copies identiques** (même
séquence, même contenu). Au rejeu, l'une ou l'autre convient : aucune copie
ne peut passer pour une version plus récente qu'une écriture faite
entre-temps.

### 1.3 Pièges fréquents

- **Repartir de 1 à chaque démarrage** : les nouvelles écritures auraient
  des numéros plus **petits** que les anciennes et perdraient au rejeu. Il
  faut initialiser les deux compteurs à partir de la flash : **max + 1**.
- **Remettre `prochaineSequence_` à 1 en changeant de secteur** : ce
  compteur est global. En E2, le seul cas où l'on réécrit une séquence
  existante est la recopie de récupération.
- **Confondre les deux compteurs** : la séquence de **secteur** avance à
  chaque ouverture de secteur, celle d'**enregistrement** à chaque
  opération.
- **Little-endian** : utiliser `ecrireU32` / `lireU32`. Dans l'exemple de
  `FORMAT.md`, la séquence 1 s'écrit `01 00 00 00`.

### 1.4 Comment le vérifier

Le **test de remontage** (exigé) : écrire, remplacer et supprimer plusieurs
fois, sur assez de données pour remplir 2 ou 3 secteurs, puis construire un
**nouveau** `SystemeFichiers` sur la même flash. Il doit retrouver
exactement les dernières versions **et pouvoir continuer à écrire**. Si vos
séquences sont fausses, c'est ce test qui échoue.

---

## 2. Le montage : collecter, trier, puis rejouer

Une question revient : *« Si je mets l'index à jour au fur et à mesure de la
lecture, une suppression peut être effacée par une écriture plus ancienne
lue après elle. Faut-il marquer le fichier supprimé, par exemple avec une
taille à 0 ? »*

La question est juste : ce problème existe. Mais la solution n'est pas la
taille à 0. Il faut **changer l'ordre dans lequel on rejoue**.

### 2.1 Le problème : rejouer dans l'ordre de lecture

L'approche naturelle est de tout faire en une seule boucle : pour chaque
secteur, pour chaque enregistrement valide, mettre à jour `index_` tout de
suite (FICHIER → ajout, SUPPRESSION → `erase`).

Cela revient à rejouer dans l'**ordre de lecture**, c'est-à-dire l'ordre
physique sur la flash. Or le format demande de rejouer dans l'**ordre
chronologique**, donné par la `sequence` (voir la note 1). Les deux ordres
ne sont pas forcément les mêmes.

**Exemple** (extension E2). Le secteur 0 a été récupéré, effacé, puis remis
en service : c'est maintenant le secteur **le plus récent**, alors que c'est
le premier de la flash.

```
secteur 0 (seq secteur 4, le plus récent)   seq 15  SUPPRESSION a.txt
secteur 2 (seq secteur 3)                   seq 10  FICHIER     a.txt = "v2"
```

Chronologiquement : `a.txt` a été écrit (seq 10), puis supprimé (seq 15).
Il **ne doit plus exister**.

En rejouant dans l'ordre de lecture (secteur 0, puis secteur 2) :

| Lu | Action | `index_` après |
|---|---|---|
| seq 15 SUPPRESSION a.txt | `erase("a.txt")` : rien à retirer | vide |
| seq 10 FICHIER a.txt | `index_["a.txt"] = …` | **a.txt présent** ❌ |

Le fichier supprimé **ressuscite**. Sa suppression a été appliquée avant
l'écriture qu'elle devait annuler.

> Dans le **socle** (sans E2), les secteurs sont remplis dans l'ordre 0, 1,
> 2… et ne sont jamais réutilisés : l'ordre de lecture coïncide alors avec
> l'ordre des séquences, et ce bug **ne se voit pas** dans vos tests. Il
> apparaît dès qu'un secteur est réutilisé (E2) ou que des enregistrements
> sont recopiés. Mieux vaut écrire tout de suite la version correcte : elle
> n'est pas plus longue.

### 2.2 La solution : deux temps

On sépare la **lecture** de la **reconstruction** :

1. **Collecter** : parcourir tous les secteurs et ranger chaque
   enregistrement valide (nom, type, séquence, adresse, taille) dans un
   `std::vector`, **sans toucher à l'index**.
2. **Trier, puis rejouer** : trier ce vecteur par `sequence`, puis le
   parcourir dans l'ordre et appliquer chaque opération à `index_`.

```cpp
struct Lu {                     // un enregistrement valide trouvé sur la flash
    std::string nom;
    std::uint8_t type;          // 0x01 = FICHIER, 0x02 = SUPPRESSION
    std::uint32_t sequence;
    std::size_t adresse;
    std::uint32_t taille;
};

std::vector<Lu> lus;
// 1. Collecter : pour chaque secteur utilisé, pour chaque enregistrement
//    valide → lus.push_back({...});   (aucune modification de index_ ici)

// 2. Trier par séquence (de la plus ancienne à la plus récente)
std::stable_sort(lus.begin(), lus.end(),
                 [](const Lu& a, const Lu& b) { return a.sequence < b.sequence; });

// 3. Rejouer dans l'ordre chronologique : la dernière opération gagne
for (const Lu& e : lus) {
    if (e.type == 0x01)         // FICHIER
        index_[e.nom] = Entree{e.adresse, e.taille, e.sequence};
    else
        index_.erase(e.nom);
}
```

Avec l'exemple précédent, la liste triée donne seq 10 (FICHIER) puis
seq 15 (SUPPRESSION) : `a.txt` est ajouté, puis retiré. ✅

Une fois la liste triée, **aucun enregistrement plus ancien ne peut arriver
après un plus récent** : la question « une ancienne écriture va-t-elle
écraser ma suppression ? » ne se pose plus. On n'a besoin d'aucun marquage
particulier.

Quelques remarques :

- **Pourquoi `stable_sort` ?** Après une récupération interrompue (E2), deux
  copies peuvent avoir la même séquence. Elles sont identiques, donc l'une ou
  l'autre convient, mais le tri stable garde un résultat prévisible.
- **Coût** : un vecteur de quelques centaines d'éléments à trier une seule
  fois, au démarrage. C'est négligeable.
- **Après le rejeu** : `prochaineSequence_ = plus grande séquence lue + 1`
  (c'est le dernier élément de la liste triée, si elle n'est pas vide).

### 2.3 Pourquoi pas « taille à 0 » ?

Parce qu'un **fichier vide** (0 octet) est un fichier tout à fait valable :
`ecrireFichier("vide.txt", {})` doit fonctionner, et `liste()` doit le
montrer. Si « taille 0 » voulait dire « supprimé », vous ne pourriez plus
distinguer les deux cas : un fichier vide disparaîtrait au remontage.

Règle générale : **ne jamais donner un sens caché à une valeur qui est déjà
valide** (taille 0, adresse 0, chaîne vide…). Si on a besoin d'une
information de plus, on ajoute un champ prévu pour ça.

### 2.4 Et si je tiens à tout faire en une seule passe ?

C'est possible, mais plus délicat. Il faut alors :

- garder pour chaque nom la **dernière séquence vue, y compris pour une
  suppression**, avec un champ dédié (par exemple `bool supprime` dans
  l'entrée) : on parle de « pierre tombale » ;
- **ignorer** tout enregistrement dont la séquence est plus petite que celle
  déjà connue pour ce nom ;
- en fin de montage, **retirer** de l'index toutes les entrées marquées
  supprimées.

Cela marche, mais c'est trois règles à ne pas oublier, contre un simple tri.
Préférez la solution en deux temps.

### 2.5 Comment le vérifier

- **Socle** : le test de remontage (note 1.4), avec au moins une
  suppression, et une réécriture d'un fichier après sa suppression. Après
  remontage, le fichier supprimé doit être absent, le fichier réécrit
  présent avec son **dernier** contenu.
- **Avec E2** : remplir la flash jusqu'à forcer une récupération, supprimer
  un fichier **après** la récupération, puis remonter : le fichier doit
  rester absent. C'est exactement le cas de l'exemple 2.1.

### 2.6 Deux passes, n'est-ce pas inefficace ?

Non :

- **La flash n'est lue qu'une seule fois.** La deuxième passe se fait en
  mémoire, sur un petit vecteur de **métadonnées** (nom, type, séquence,
  adresse, taille). Les données des fichiers ne sont ni copiées ni relues.
- **Ordre de grandeur** : 8 secteurs de 4 096 octets, un enregistrement
  d'au moins 17 octets, soit au plus environ 2 000 entrées. Les trier prend
  quelques dizaines de microsecondes et une centaine de Ko de mémoire.
- **Le montage n'a lieu qu'une fois**, au démarrage : ce n'est pas une
  boucle critique.
- **La version en une passe (2.4) a aussi sa seconde passe**, simplement
  cachée : parcourir l'index à la fin pour retirer les entrées marquées
  supprimées, après les avoir gardées en mémoire pendant tout le montage.
- **Même complexité** : trier coûte n log n, remplir une `std::map` aussi.
  Le tri n'est pas plus lent ; il est plus simple et plus sûr.

De vrais systèmes de fichiers pour flash font de même : JFFS2, sous Linux,
relit toute la flash au montage et reconstruit son état en mémoire à partir
de numéros de version.

Règle générale : écrire d'abord la version **simple et correcte**, et
n'optimiser que si une **mesure** montre un vrai problème.
