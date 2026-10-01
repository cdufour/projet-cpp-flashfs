# Squelette FOURNI du projet FlashFS : à conserver tel quel en Phase 1.
# Vous pouvez le compléter (nouvelles cibles, options), pas le dégrader.
#
# ============================================================================
# CMake : la « recette de fabrication » du projet.
# Il décrit QUOI construire (une bibliothèque, un programme, des tests) et
# AVEC QUELLES OPTIONS. Deux commandes suffisent ensuite :
#     cmake -S . -B build          # préparer (une fois) dans le dossier build/
#     cmake --build build -j       # compiler (-j : en parallèle)
# ============================================================================
cmake_minimum_required(VERSION 3.18)
project(flashfs VERSION 0.1 LANGUAGES CXX)

# Norme C++17, obligatoire, sans extensions propres à g++ (code portable).
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Produit compile_commands.json, utilisé par les éditeurs (VS Code…) pour l'analyse du code.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Par défaut, compilation « Debug » : informations de débogage pour gdb.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# -DSANITIZE=address  → AddressSanitizer (+ UBSan)
# -DSANITIZE=thread   → ThreadSanitizer (Phase 2, concurrence)
# Les « sanitizers » instrumentent le programme pour détecter à l'exécution
# les accès mémoire illégaux (address) ou les accès concurrents non protégés
# (thread). Exemple : cmake -S . -B build-tsan -DSANITIZE=thread
set(SANITIZE "" CACHE STRING "Sanitizer : address | thread | vide")

# Bibliothèque système des threads (std::thread en a besoin sous Linux).
find_package(Threads REQUIRED)

# Options de compilation appliquées à chacune de nos cibles (bibliothèque, programme, tests)
# -Wall -Wextra -Wpedantic -Wshadow : le compilateur signale tout code douteux.
# -Werror : chaque avertissement devient une ERREUR. D'où « zéro warning ».
function(flashfs_options cible)
  target_compile_options(${cible} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Werror)
  if(SANITIZE STREQUAL "address")
    target_compile_options(${cible} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${cible} PRIVATE -fsanitize=address,undefined)
  elseif(SANITIZE STREQUAL "thread")
    target_compile_options(${cible} PRIVATE -fsanitize=thread)
    target_link_options(${cible} PRIVATE -fsanitize=thread)
  endif()
endfunction()

# --- Bibliothèque : tout src/ sauf main.cpp (un nouveau .cpp est pris en compte
#     automatiquement au prochain `cmake --build`)
# Tout le code « métier » est rangé dans une bibliothèque, partagée par le
# programme ET par les tests : les tests exercent exactement le même code.
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM LIB_SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)
add_library(flashfs_lib STATIC ${LIB_SOURCES})
target_include_directories(flashfs_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(flashfs_lib PUBLIC Threads::Threads)
flashfs_options(flashfs_lib)

# --- Exécutable : le shell flashfs (main.cpp + la bibliothèque)
add_executable(flashfs src/main.cpp)
target_link_libraries(flashfs PRIVATE flashfs_lib)
flashfs_options(flashfs)

# --- Tests : chaque fichier tests/test_xxx.cpp est un petit programme (avec
#     son propre main) qui devient l'exécutable build/test_xxx, déclaré à
#     ctest sous le nom test_xxx. Un nouveau fichier test_*.cpp est pris en
#     compte automatiquement au prochain `cmake --build`.
#     Les vérifications viennent de tests/Verif.hpp (fourni).
enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/test_*.cpp)
foreach(source ${TEST_SOURCES})
  get_filename_component(nom ${source} NAME_WE)   # tests/test_crc.cpp → test_crc
  add_executable(${nom} ${source})
  target_link_libraries(${nom} PRIVATE flashfs_lib)
  flashfs_options(${nom})
  add_test(NAME ${nom} COMMAND ${nom})
endforeach()
