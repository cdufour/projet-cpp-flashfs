# Projet C++ — FlashFS

Projet en binôme du cursus LLS / EXPLEO (AJC Formation), les 1er et 2 octobre
2026 : un système de fichiers qui résiste aux coupures de courant, sur une
mémoire flash simulée.

## Contenu

| Élément | Rôle |
| --- | --- |
| [`projet-cpp-flashfs-enonce.md`](projet-cpp-flashfs-enonce.md) | L'énoncé complet : à lire en entier avant de commencer |
| [`squelette/`](squelette/) | Le point de départ de votre projet : à copier dans votre dépôt GitLab `flashfs` (étape 1 de la Phase 1) |
| [`groupes.txt`](groupes.txt) | La composition des binômes |
| [`notes-complementaires.md`](notes-complementaires.md) | Précisions ajoutées pendant le projet, à partir de vos questions |

D'autres ressources pourront être ajoutées ici pendant le projet.

## Démarrer

```bash
git clone https://github.com/cdufour/projet-cpp-flashfs.git
cp -r projet-cpp-flashfs/squelette/. <votre-depot-flashfs>/
cd <votre-depot-flashfs>
cmake -S . -B build && cmake --build build -j && ctest --test-dir build
```

Pour récupérer plus tard les ajouts de ce dépôt : `git pull` dans
`projet-cpp-flashfs/`.
