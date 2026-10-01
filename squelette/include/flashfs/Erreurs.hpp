#pragma once
// FOURNI par le formateur. Vous pouvez l'enrichir (nouvelles exceptions, fonctions), pas le dégrader.
//
// ============================================================================
// LES ERREURS DU PROJET (les « exceptions »)
//
// En C, une fonction signale une erreur par une valeur de retour (-1, NULL…)
// que l'appelant doit penser à tester. En C++, on peut aussi LEVER une
// exception :   throw DisquePlein("plus de place");
// L'exception remonte alors automatiquement les appels de fonctions, jusqu'au
// premier endroit qui l'ATTRAPE :   try { … } catch (const DisquePlein& e) { … }
// Impossible de l'ignorer par oubli : si personne ne l'attrape, le programme s'arrête.
//
// POURQUOI UNE HIÉRARCHIE (des classes qui héritent les unes des autres) ?
// Pour pouvoir attraper plus ou moins large :
//
//   std::runtime_error                 (fournie par la bibliothèque standard)
//    ├── ErreurFlash                   « problème de la mémoire »
//    │    ├── AdresseInvalide
//    │    ├── ProgrammationInterdite
//    │    ├── SecteurUse
//    │    └── CoupureCourant
//    └── ErreurFs                      « problème du système de fichiers »
//         ├── FichierIntrouvable, NomInvalide, FichierTropGros,
//         └── DisquePlein, ImageInvalide, ErreurHote
//
//   catch (const CoupureCourant&) → attrape seulement les coupures ;
//   catch (const ErreurFlash&)    → attrape TOUTES les erreurs de mémoire ;
//   catch (const std::exception&) → attrape tout (chaque erreur « est une » exception).
//
// Chaque exception porte un message, récupérable par e.what().
// ============================================================================

#include <stdexcept>  // std::runtime_error, la classe de base des erreurs « d'exécution »

namespace flashfs {

// Toutes ces classes ont la même forme : `struct` (une classe dont tout est
// public), qui hérite de sa mère, et une ligne `using …` qui reprend le
// constructeur de la mère (celui qui reçoit le message). Aucun autre code
// n'est nécessaire : c'est le TYPE de l'exception qui porte l'information.

// --- Erreurs matérielles (mémoire flash)
struct ErreurFlash : std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct AdresseInvalide : ErreurFlash {  // adresse ou secteur hors de la mémoire (règle 5)
    using ErreurFlash::ErreurFlash;
};
struct ProgrammationInterdite : ErreurFlash {  // un bit devrait passer de 0 à 1 (règle 3)
    using ErreurFlash::ErreurFlash;
};
struct SecteurUse : ErreurFlash {  // effacement au-delà de l'endurance (règle 4)
    using ErreurFlash::ErreurFlash;
};
struct CoupureCourant : ErreurFlash {  // coupure simulée par FlashInstable
    using ErreurFlash::ErreurFlash;
};

// --- Erreurs du système de fichiers
struct ErreurFs : std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct FichierIntrouvable : ErreurFs {  // lecture ou suppression d'un nom inconnu
    using ErreurFs::ErreurFs;
};
struct NomInvalide : ErreurFs {  // nom vide, trop long ou avec un caractère interdit
    using ErreurFs::ErreurFs;
};
struct FichierTropGros : ErreurFs {  // le contenu ne tient pas dans un secteur
    using ErreurFs::ErreurFs;
};
struct DisquePlein : ErreurFs {  // plus aucune place
    using ErreurFs::ErreurFs;
};
struct ImageInvalide : ErreurFs {  // fichier image absent, illisible ou mal formé
    using ErreurFs::ErreurFs;
};
struct ErreurHote : ErreurFs {  // fichier de la machine hôte illisible / non inscriptible
    using ErreurFs::ErreurFs;
};

} // namespace flashfs
