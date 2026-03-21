#pragma once

#include <cstdint>
#include <vector>

struct Magic {
    uint64_t mask;    // Maska pól "istotnych" (bez krawędzi)
    uint64_t magic;   // Magiczna liczba
    int shift;        // Przesunięcie bitowe (64 - rozmiar indeksu)
    uint64_t* attacks; // Wskaźnik do fragmentu dużej tablicy
};

// Deklaracje tablic (zostaną zdefiniowane w magic.cpp)
extern Magic RookMagics[64];
extern Magic BishopMagics[64];

// Funkcje dostępu (muszą być ekstremalnie szybkie)
uint64_t get_rook_attacks(int sq, uint64_t occupied);
uint64_t get_bishop_attacks(int sq, uint64_t occupied);

// Pomocnicze funkcje do generowania magicznych liczb
uint64_t create_rook_mask(int sq);
uint64_t create_bishop_mask(int sq);
uint64_t rook_attacks_on_the_fly(int sq, uint64_t occ);
uint64_t bishop_attacks_on_the_fly(int sq, uint64_t occ);

// Funkcja inicjalizująca (wywoływana raz na starcie programu)
void init_magic_tables();
