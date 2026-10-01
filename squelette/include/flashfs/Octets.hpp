#pragma once
// FOURNI par le formateur. Vous pouvez le compléter, pas le dégrader.
// Lecture / écriture d'entiers en little-endian, indépendamment de la machine hôte.
//
// ============================================================================
// POURQUOI CES DEUX FONCTIONS ?
//   Un entier de 32 bits occupe 4 octets. Dans quel ORDRE les ranger ? Les
//   processeurs ne sont pas d'accord entre eux (« endianness »). Si l'on
//   recopiait la mémoire telle quelle, une image écrite sur une machine
//   pourrait être illisible sur une autre.
//   On fixe donc une convention, le « little-endian » (octet de poids faible
//   en premier), et on range les octets UN PAR UN, explicitement :
//
//       0x12345678  →  78 56 34 12      (en mémoire, dans cet ordre)
//
//   Même règle qu'au Projet C : on n'écrit jamais une `struct` brute sur le
//   support, on sérialise champ par champ.
//
// `inline` : fonctions assez courtes pour être définies dans le .hpp ; le mot-clé
// autorise leur présence dans plusieurs fichiers .cpp qui incluent cet en-tête.
// ============================================================================

#include <cstdint>  // std::uint8_t, std::uint32_t

namespace flashfs {

// Range `v` dans les 4 octets pointés par `p`, octet de poids faible d'abord.
inline void ecrireU32(std::uint8_t* p, std::uint32_t v)
{
    // `v >> 8` décale de 8 bits vers la droite : l'octet suivant arrive en
    // position basse. static_cast<std::uint8_t> ne garde que les 8 bits du bas.
    p[0] = static_cast<std::uint8_t>(v);        // 0x12345678 → 0x78
    p[1] = static_cast<std::uint8_t>(v >> 8);   //            → 0x56
    p[2] = static_cast<std::uint8_t>(v >> 16);  //            → 0x34
    p[3] = static_cast<std::uint8_t>(v >> 24);  //            → 0x12
}

// Opération inverse : reconstitue l'entier à partir des 4 octets.
inline std::uint32_t lireU32(const std::uint8_t* p)
{
    // Chaque octet est replacé à sa position (<< décale vers la gauche),
    // puis les quatre sont assemblés par un OU bit à bit (|).
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

} // namespace flashfs
