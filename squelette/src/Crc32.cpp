// FOURNI par le formateur : CRC32 IEEE, identique à votre cbx_crc32 du Projet C.
//
// ============================================================================
// Crc32.cpp — calcul du CRC32 par table précalculée
//
// Le principe mathématique (division de polynômes en arithmétique binaire)
// n'est pas l'objet du projet. Retenez la mécanique : pour aller vite, on
// précalcule une fois pour toutes une TABLE de 256 valeurs (une par valeur
// possible d'un octet), puis chaque octet des données se traite en une
// consultation de table et deux opérations sur les bits.
// ============================================================================
#include "flashfs/Crc32.hpp"

#include <array>  // std::array : tableau de taille fixe, connue à la compilation

namespace flashfs {

// Espace de noms SANS nom : ce qui est dedans n'est visible que dans ce
// fichier .cpp (équivalent du `static` du C). La table est un détail interne.
namespace {

// La table est calculée PAR LE COMPILATEUR, pendant la compilation :
//   - `constexpr` = « calculable à la compilation » ;
//   - [] { … }() est une lambda (petite fonction sans nom) aussitôt appelée
//     grâce aux () finales. Son résultat initialise TABLE.
// À l'exécution, la table est déjà prête : aucun calcul au démarrage.
constexpr std::array<std::uint32_t, 256> TABLE = [] {
    std::array<std::uint32_t, 256> t{};
    for (std::uint32_t i = 0; i < 256; ++i) {
        std::uint32_t c = i;
        // 8 tours, un par bit de l'octet. À chaque tour : si le bit de poids
        // faible est à 1, on décale et on applique le polynôme (XOR) ; sinon
        // on décale seulement. `? :` est le « si / sinon » en une expression.
        for (int k = 0; k < 8; ++k)
            c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        t[i] = c;
    }
    return t;
}();

} // namespace

std::uint32_t crc32(const std::uint8_t* donnees, std::size_t n, std::uint32_t crc)
{
    // Convention du CRC32 IEEE : on inverse tous les bits (~) au début et à
    // la fin. C'est aussi ce qui permet l'enchaînement incrémental : le
    // résultat précédent, ré-inversé, redonne l'état interne du calcul.
    crc = ~crc;
    // Pour chaque octet : on le combine (^ = XOR) avec l'état courant, on
    // garde 8 bits (& 0xFF) pour choisir une case de la table, et on la
    // combine avec l'état décalé de 8 bits.
    for (std::size_t i = 0; i < n; ++i)
        crc = TABLE[(crc ^ donnees[i]) & 0xFFu] ^ (crc >> 8);
    return ~crc;
}

} // namespace flashfs
