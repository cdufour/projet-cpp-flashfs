# Notes complémentaires

Précisions sur l'énoncé et sur `FORMAT.md`, ajoutées pendant le projet à
partir de vos questions. Elles ne changent pas ce qui est demandé : elles
l'expliquent. La plus récente est en bas.

## Sommaire

1. [Le champ `sequence`](#1-le-champ-sequence) (01/10/2026)

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
