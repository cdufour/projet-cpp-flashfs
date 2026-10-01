#include <iostream>

#include "flashfs/Version.hpp"

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "flashfs " << flashfs::version()
                  << " — système de fichiers pour mémoire flash simulée\n"
                  << "Usage : " << argv[0] << " <image> [options]   (voir l'énoncé)\n";
        return 1;
    }
    std::cerr << argv[0] << " : pas encore implémenté\n";
    return 1;
}
