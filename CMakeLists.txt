cmake_minimum_required(VERSION 3.10)
project(ChessProject)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Ustawienie standardu C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dodajemy folder core do ścieżek wyszukiwania nagłówków (#include "board.hpp" zadziała wszędzie)
include_directories(core engine)

# 1. Definiujemy pliki źródłowe silnika
set(CORE_SOURCES
    core/board.cpp
    core/move_gen.cpp
    core/magic.cpp
    core/zobrist.cpp
)

# 2. Tworzymy bibliotekę z core (opcjonalne, ale ułatwia życie)
add_library(chess_core ${CORE_SOURCES})

set(ENGINE_SOURCES
    engine/evaluate.cpp
    engine/search.cpp
)
add_library(chess_engine ${ENGINE_SOURCES})
target_link_libraries(chess_engine chess_core)

# 3. Definiujemy executable dla Twojego generatora
# Dodajemy magic_generator.cpp i linkujemy go z naszą biblioteką core
add_executable(magic_gen utils/magic_generator.cpp)
target_link_libraries(magic_gen chess_core)

# 4. główny program lub most do Pythona


# Tworzymy główny program z pliku main.cpp
add_executable(chess_app core/main.cpp)
target_link_libraries(chess_app chess_core chess_engine)

# Dodany test Perft
add_executable(perft_test core/perft.cpp)
target_link_libraries(perft_test chess_core)