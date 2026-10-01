#pragma once
// Interface IMPOSÉE — ne pas modifier sa partie publique.
// Toutes les mémoires flash du projet (FlashRam, FlashFichier, FlashInstable…)
// en héritent, et le système de fichiers ne connaît QUE cette interface.
//
// ============================================================================
// QU'EST-CE QU'UNE « INTERFACE » ?
//   Un CONTRAT : la liste de ce qu'une mémoire flash sait faire (lire,
//   programmer, effacer…), SANS dire comment. Les méthodes marquées `= 0`
//   (« virtuelles pures ») n'ont pas de code ici : chaque classe qui hérite
//   de BlockDevice doit les fournir. On ne peut donc pas créer un objet
//   « BlockDevice » tout seul, seulement une FlashRam, une FlashFichier…
//
// POURQUOI UNE INTERFACE ?
//   Le système de fichiers ne manipule qu'un « BlockDevice& » : il ignore s'il
//   parle à une mémoire en RAM, à un fichier ou à une mémoire qui va « couper
//   le courant ». On peut ainsi tester, faire tourner et torturer le MÊME code
//   sans le modifier. C'est le POLYMORPHISME : le bon code est choisi à
//   l'exécution, selon l'objet réel qui se cache derrière la référence.
//   (Au Projet C, le même découplage passait par des pointeurs de fonctions.)
// ============================================================================

#include <cstddef>  // std::size_t : type des tailles et des adresses (entier positif)
#include <cstdint>  // std::uint8_t (un octet), std::uint32_t (entier 32 bits sans signe)

namespace flashfs {

/// Mémoire flash de type NOR, vue par secteurs.
///
/// Règles physiques que TOUTE implémentation doit faire respecter :
///  1. la mémoire est découpée en nbSecteurs() secteurs de tailleSecteur() octets ;
///  2. un octet effacé vaut 0xFF ;
///  3. programmer() ne peut faire passer des bits que de 1 à 0 : écrire un octet
///     qui exigerait un passage 0 → 1 est refusé (exception) et rien n'est écrit ;
///  4. effacer(s) remet tout le secteur s à 0xFF et incrémente son compteur
///     d'usure ; un secteur déjà effacé enduranceMax() fois est usé (exception) ;
///  5. toute adresse ou tout secteur hors de la mémoire est refusé (exception).
class BlockDevice {
public:
    // Destructeur VIRTUEL : indispensable dans une classe de base destinée à
    // l'héritage. Quand on détruit une FlashRam à travers un pointeur de type
    // BlockDevice (c'est le cas du std::unique_ptr<BlockDevice> de main.cpp),
    // c'est bien le destructeur de FlashRam qui est appelé. Sans `virtual`,
    // seule la partie BlockDevice serait détruite : fuite de mémoire.
    // `= default` : « le destructeur standard me convient ».
    virtual ~BlockDevice() = default;

    // Géométrie : combien d'octets par secteur, combien de secteurs.
    // `const` à la fin = la méthode ne modifie pas l'objet (simple consultation).
    virtual std::size_t tailleSecteur() const = 0;
    virtual std::size_t nbSecteurs() const = 0;
    // Seule méthode avec du code ici : elle se déduit des deux précédentes,
    // quelle que soit l'implémentation.
    std::size_t taille() const { return tailleSecteur() * nbSecteurs(); }

    /// Copie n octets depuis l'adresse (absolue) `adresse` vers `dest`.
    // L'appelant fournit le tampon `dest` (au moins n octets) ; on y recopie les données.
    virtual void lire(std::size_t adresse, std::uint8_t* dest, std::size_t n) const = 0;

    /// Programme n octets à l'adresse `adresse` (règle 3 : bits 1 → 0 seulement).
    /// « Programmer » (et non « écrire ») : terme des fiches techniques flash. On ne
    /// peut qu'éteindre des bits : FF -> 5A accepté, puis 5A -> A5 refusé (0 -> 1).
    /// Pour une valeur quelconque : effacer() le secteur, puis programmer().
    // `const std::uint8_t* src` : les octets à programmer, que la méthode lit sans les modifier.
    virtual void programmer(std::size_t adresse, const std::uint8_t* src, std::size_t n) = 0;

    /// Efface le secteur d'indice `secteur` (règle 4).
    virtual void effacer(std::size_t secteur) = 0;

    /// Nombre d'effacements subis par le secteur (usure).
    virtual std::uint32_t nbEffacements(std::size_t secteur) const = 0;

    /// Nombre maximal d'effacements supportés par un secteur.
    virtual std::uint32_t enduranceMax() const = 0;

protected:
    // `protected` : accessible aux classes filles seulement.
    // Le constructeur n'est utilisable que par une fille (FlashRam…), jamais directement.
    BlockDevice() = default;
    // Une mémoire représente un composant matériel unique : on ne la copie pas.
    // `= delete` SUPPRIME la copie : écrire « FlashRam b = a; » ne compile pas.
    // (Deux copies d'une même puce divergeraient sans que personne ne s'en aperçoive.)
    BlockDevice(const BlockDevice&) = delete;
    BlockDevice& operator=(const BlockDevice&) = delete;
};

} // namespace flashfs
