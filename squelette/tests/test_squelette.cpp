// ============================================================================
// test_squelette.cpp — tests d'exemple FOURNIS avec le squelette
//
// COMMENT SE LIT UN FICHIER DE TESTS ?
//   C'est un petit programme. Chaque test est une fonction sans paramètre qui
//   fait des vérifications (VERIFIER, VERIFIER_EGAL, VERIFIER_LEVE, EXIGER :
//   voir Verif.hpp). Le main() lance chaque test avec verif::lancer(), puis
//   renvoie verif::resultat() : 0 si tout est vert.
//   On lance tous les fichiers de tests avec :   ctest --test-dir build
// ============================================================================
#include <string>
#include <type_traits>  // outils d'interrogation des types, à la compilation

#include "Verif.hpp"
#include "flashfs/BlockDevice.hpp"
#include "flashfs/Version.hpp"

// Fichier d'exemple : à conserver, puis ajoutez vos propres fichiers tests/test_*.cpp

// VÉRIFICATIONS À LA COMPILATION : si quelqu'un rend BlockDevice copiable, le
// projet ne compile plus. static_assert(condition) : si la condition est
// fausse, le programme ne compile même pas. std::is_copy_constructible_v<T>
// répond à la question « peut-on copier un T ? ».
static_assert(!std::is_copy_constructible_v<flashfs::BlockDevice>);
static_assert(!std::is_copy_assignable_v<flashfs::BlockDevice>);
// Le destructeur doit être virtuel (voir BlockDevice.hpp).
static_assert(std::has_virtual_destructor_v<flashfs::BlockDevice>);

// Vérifie que la chaîne de compilation et de test fonctionne de bout en bout.
void chaineDeTestFonctionne()
{
    VERIFIER_EGAL(std::string(flashfs::version()), "0.1");
}

int main()
{
    verif::lancer("chaineDeTestFonctionne", chaineDeTestFonctionne);
    return verif::resultat();
}
