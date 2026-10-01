#pragma once
// `#pragma once` (en tête de chaque .hpp) : empêche d'inclure deux fois le même
// en-tête dans un fichier, ce qui provoquerait des déclarations en double.

namespace flashfs {
// Numéro de version du programme, affiché par l'aide (`flashfs` sans argument).
// Déclaration seulement : le code est dans src/Version.cpp.
const char* version();
}
