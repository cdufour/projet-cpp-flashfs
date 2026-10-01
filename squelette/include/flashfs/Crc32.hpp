#pragma once
// FOURNI par le formateur. Vous pouvez le compléter, pas le dégrader.
//
// ============================================================================
// LE CRC32 : UNE « EMPREINTE » DES DONNÉES
//
// Un CRC est un nombre calculé à partir d'une suite d'octets, un peu comme la
// clé d'un RIB ou d'un numéro de sécurité sociale. On le calcule à l'écriture
// et on le range à côté des données ; à la relecture, on le recalcule. Si les
// deux valeurs diffèrent, les données ont été abîmées (ou à moitié écrites).
// Un seul bit modifié suffit à changer le CRC.
//
// Dans le projet, chaque en-tête de secteur et chaque enregistrement porte
// son CRC32 : c'est ce qui permet de reconnaître une écriture interrompue par
// une coupure ou une corruption de la flash.
// ============================================================================

#include <cstddef>  // std::size_t
#include <cstdint>  // std::uint8_t, std::uint32_t

namespace flashfs {

/// CRC32 IEEE (polynôme réfléchi 0xEDB88320). Incrémental : passer le résultat
/// précédent dans `crc` pour enchaîner plusieurs blocs.
//
// « Incrémental » : pour calculer le CRC de deux morceaux A puis B comme s'ils
// n'en formaient qu'un, on écrit   crc32(B, nB, crc32(A, nA)).
// `std::uint32_t crc = 0` : le 3e argument est FACULTATIF ; s'il est omis, il vaut 0.
std::uint32_t crc32(const std::uint8_t* donnees, std::size_t n, std::uint32_t crc = 0);

} // namespace flashfs
