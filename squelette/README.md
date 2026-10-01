# flashfs — squelette fourni

Pré-requis (VM Debian) :

```bash
sudo apt install cmake valgrind
```

Compilation et tests :

```bash
cmake -S . -B build              # configuration (une seule fois)
cmake --build build -j           # compilation (zéro warning : -Werror)
ctest --test-dir build --output-on-failure   # tous les fichiers de tests
./build/test_crc                 # un seul fichier de tests, avec son affichage
./build/flashfs                  # affiche l'usage
```

Chaque fichier `tests/test_xxx.cpp` est un petit programme qui devient
l'exécutable `build/test_xxx`. Les vérifications (`VERIFIER`,
`VERIFIER_EGAL`, `VERIFIER_LEVE`, `EXIGER`) sont décrites dans
`tests/Verif.hpp`.

Variantes :

```bash
cmake -S . -B build-asan -DSANITIZE=address && cmake --build build-asan -j   # fuites / débordements
cmake -S . -B build-tsan -DSANITIZE=thread  && cmake --build build-tsan -j   # concurrence (Phase 2)
for t in build/test_*; do valgrind -q --leak-check=full --error-exitcode=1 $t > /dev/null || echo "PROBLÈME : $t"; done
```

À remplacer par votre propre README en fin de projet.
