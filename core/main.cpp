#include <iostream>
#include "board.hpp"
#include "magic.hpp"

void printBitboard(uint64_t bb) {
    for (int r = 7; r >= 0; r--) {
        for (int f = 0; f < 8; f++) {
            std::cout << ((bb >> (r * 8 + f)) & 1 ? "X " : ". ");
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

int main() {
    // 1. Inicjalizacja magii (bez tego program wybuchnie!)
    init_magic_tables();

    Board board;
    // ... ustawienie jakiejś pozycji (opcjonalnie) ...

    // Testujemy wieżę na polu d4 (indeks 27)
    int testSq = 27; 
    uint64_t attacks = board.getTowerAttacks(testSq);

    std::cout << "Ataki wiezy na d4:\n";
    printBitboard(attacks);

    return 0;
}