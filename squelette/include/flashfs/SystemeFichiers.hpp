#pragma once
// ============================================================================
// SystemeFichiers.hpp — FOURNI (étape 6 de la Phase 1)
//
// Le cœur du projet : un système de fichiers JOURNALISÉ. On ne corrige jamais
// rien sur la flash : chaque opération (« config.txt contient maintenant … »,
// « notes.txt est supprimé ») est AJOUTÉE à la suite, comme une ligne dans un
// journal de bord. Pour connaître un fichier, on relit le journal : la
// dernière mention gagne.
//
// CE QUI EST FOURNI ET CE QUI EST À VOUS :
//   - la partie PUBLIQUE est l'API minimale exigée : vous pouvez l'ENRICHIR
//     (ajouter des méthodes), pas la réduire ;
//   - la partie PRIVÉE est une SUGGESTION d'organisation : modifiez-la
//     librement (ajoutez, renommez, supprimez) ;
//   - tout le code (src/SystemeFichiers.cpp) est à écrire.
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <map>       // std::map : dictionnaire trié (clé → valeur)
#include <optional>  // std::optional : « une valeur, ou rien »
#include <string>
#include <vector>

#include "flashfs/BlockDevice.hpp"

namespace flashfs {

// DÉCLARATION ANTICIPÉE : « une classe Fichier existe ». Elle suffit pour
// DÉCLARER ouvrir() ci-dessous. Vous écrirez Fichier.hpp à l'étape 7 : d'ici
// là, SystemeFichiers compile sans lui. Le code qui APPELLE ouvrir() doit
// inclure "flashfs/Fichier.hpp".
class Fichier;

// Une ligne de liste() : un nom et une taille en octets.
struct InfoFichier {
    std::string nom;
    std::uint32_t taille;
};

/// Système de fichiers journalisé sur flash (format imposé : voir FORMAT.md).
/// Ne POSSÈDE PAS la flash : il en garde une référence, donc la flash doit
/// vivre plus longtemps que lui.
class SystemeFichiers {
public:
    // Constantes du format imposé.
    static constexpr std::size_t TAILLE_ENTETE_SECTEUR = 16;
    static constexpr std::size_t TAILLE_ENTETE_ENR = 16;
    static constexpr std::size_t LG_NOM_MAX = 32;

    // Efface tous les secteurs qui ne sont pas vierges.
    // `static` : s'appelle sans objet, SystemeFichiers::formater(flash).
    static void formater(BlockDevice& flash);

    // Le constructeur MONTE le système : il relit toute la flash (règles de
    // relecture de l'énoncé) et reconstruit l'index en mémoire.
    explicit SystemeFichiers(BlockDevice& flash);

    // Copie interdite : deux systèmes de fichiers sur la même flash
    // écriraient chacun de leur côté sans se voir, et la corrompraient.
    SystemeFichiers(const SystemeFichiers&) = delete;
    SystemeFichiers& operator=(const SystemeFichiers&) = delete;

    // Poignée d'ÉCRITURE (étape 7) : à implémenter une fois Fichier écrit.
    Fichier ouvrir(const std::string& nom);

    // Crée ou remplace un fichier : un enregistrement FICHIER, validé en deux temps.
    void ecrireFichier(const std::string& nom, const std::vector<std::uint8_t>& donnees);
    // Lève FichierIntrouvable si le nom est inconnu.
    std::vector<std::uint8_t> lireFichier(const std::string& nom) const;
    // Ajoute un enregistrement SUPPRESSION ; FichierIntrouvable si le nom est inconnu.
    void supprimer(const std::string& nom);
    bool existe(const std::string& nom) const;
    // Les fichiers courants, triés par nom.
    std::vector<InfoFichier> liste() const;

private:
    // --- SUGGESTION : types internes.

    // État d'un secteur : libre (tout à 0xFF), utilisé (en-tête valide), ou
    // sale (ni l'un ni l'autre, par exemple un en-tête coupé en plein milieu).
    enum class Etat { Libre, Utilise, Sale };
    struct Secteur {
        Etat etat = Etat::Libre;
        std::uint32_t sequence = 0;  // ordre de mise en service (1, 2, 3…)
        std::size_t fin = 0;         // position (dans le secteur) de la prochaine écriture
        bool plein = false;          // zone utile terminée par un enregistrement douteux
    };
    // Une entrée de l'INDEX : où trouver la version courante d'un fichier.
    struct Entree {
        std::size_t adresse;  // adresse de l'enregistrement sur la flash
        std::uint32_t taille;
        std::uint32_t sequence;
    };

    // --- SUGGESTION : fonctions internes.
    void monter();                                    // relire la flash, reconstruire l'index
    static void verifierNom(const std::string& nom);  // NomInvalide si le nom est refusé
    void mettreEnService();                           // ouvrir un nouveau secteur (DisquePlein sinon)
    // Écrire un enregistrement en deux temps ; renvoie son adresse.
    std::size_t ajouterEnregistrement(std::uint8_t type, const std::string& nom,
                                      const std::vector<std::uint8_t>& donnees);

    // --- SUGGESTION : les données.
    BlockDevice& flash_;                   // la flash (référence : on ne la possède pas)
    std::vector<Secteur> secteurs_;        // l'état de chaque secteur
    std::map<std::string, Entree> index_;  // nom → version courante (trié par nom)
    std::optional<std::size_t> actif_;     // secteur en cours d'écriture (vide sur une flash neuve)
    std::uint32_t prochaineSequence_ = 1;    // numéro de la prochaine opération
    std::uint32_t prochaineSeqSecteur_ = 1;  // numéro du prochain secteur mis en service

    // Phase 2 (concurrence) : ajoutez ici votre std::mutex. Déclarez-le
    // `mutable` pour pouvoir le verrouiller aussi dans les méthodes const.
};

} // namespace flashfs
