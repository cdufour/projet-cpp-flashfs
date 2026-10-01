// ============================================================================
// test_crc.cpp — tests des briques FOURNIES (CRC32 et little-endian)
// ============================================================================
#include <string>

#include "Verif.hpp"
#include "flashfs/Crc32.hpp"
#include "flashfs/Octets.hpp"

// Tests des briques FOURNIES (CRC32, little-endian) : à conserver.

void vecteurDeReference()
{
    // « 123456789 » → CBF43926 : c'est la valeur de référence publiée du
    // CRC32 IEEE. Tout calcul de CRC32 correct doit la retrouver.
    const std::string s = "123456789";
    // reinterpret_cast : on présente les caractères de la chaîne comme des octets.
    VERIFIER_EGAL(flashfs::crc32(reinterpret_cast<const std::uint8_t*>(s.data()), s.size()), 0xCBF43926u);
    VERIFIER_EGAL(flashfs::crc32(nullptr, 0), 0u);  // le CRC d'une donnée vide vaut 0
}

void crcIncremental()
{
    // Calculer le CRC en deux morceaux (« 1234 » puis « 56789 ») doit donner
    // le même résultat qu'en une fois : c'est ce que fait le système de
    // fichiers (en-tête d'enregistrement, puis nom et données).
    const std::string s = "123456789";
    const auto* p = reinterpret_cast<const std::uint8_t*>(s.data());
    VERIFIER_EGAL(flashfs::crc32(p + 4, 5, flashfs::crc32(p, 4)), 0xCBF43926u);
}

void littleEndian()
{
    // 0x12345678 doit être rangé 78 56 34 12 (octet de poids faible d'abord),
    // puis relu à l'identique.
    std::uint8_t b[4];
    flashfs::ecrireU32(b, 0x12345678u);
    VERIFIER_EGAL(b[0], 0x78);
    VERIFIER_EGAL(b[3], 0x12);
    VERIFIER_EGAL(flashfs::lireU32(b), 0x12345678u);
}

int main()
{
    verif::lancer("vecteurDeReference", vecteurDeReference);
    verif::lancer("crcIncremental", crcIncremental);
    verif::lancer("littleEndian", littleEndian);
    return verif::resultat();
}
