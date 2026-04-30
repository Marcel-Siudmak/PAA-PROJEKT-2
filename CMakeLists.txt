cmake_minimum_required(VERSION 3.10)
project(ChessProject)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Domyślnie buduj w trybie Release z pełną optymalizacją
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Ustawienie standardu C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Flagi optymalizacji dla Release
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

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
    engine/tt.cpp
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

# GUI - Desktop GUI z wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

add_executable(chess_gui gui/chess_gui.cpp)
target_link_libraries(chess_gui chess_core chess_engine ${wxWidgets_LIBRARIES})
target_include_directories(chess_gui PRIVATE ${wxWidgets_INCLUDE_DIRS})