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

void visualizeMoves(int fromSq, const std::vector<Move>& moves) {
    uint64_t moveBoard = 0ULL;
    for (const auto& m : moves) {
        if (m.from == fromSq) {
            moveBoard |= (1ULL << m.to);
        }
    }

    std::cout << "Ruchy z pola " << fromSq << ":\n";
    for (int r = 7; r >= 0; r--) {
        std::cout << r + 1 << "  ";
        for (int f = 0; f < 8; f++) {
            int sq = r * 8 + f;
            if (sq == fromSq) std::cout << "S "; // Start
            else if ((moveBoard >> sq) & 1) std::cout << "X "; // Cel
            else std::cout << ". ";
        }
        std::cout << "\n";
    }
    std::cout << "   a b c d e f g h\n\n";
}

int main() {
    // 1. Inicjalizacja magii
    init_magic_tables();

    Board board;

    // Ręczne czyszczenie (na wszelki wypadek) i ustawienie wieży
    // Zakładamy, że pieces jest publiczne. Jeśli nie - użyj swojej metody setup.
    for(int i=0; i<2; i++) for(int j=0; j<6; j++) board.pieces[i][j] = 0ULL;

    // TEST 1: Wieża na d4 (27)
    int d4 = 27;
    board.pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::ROOK)] = (1ULL << d4);

    std::cout << "--- TEST 1: Wieza na d4 (pusta plansza) ---\n";
    std::vector<Move> moves = board.generatePseudoLegalMoves(Color::WHITE);
    visualizeMoves(d4, moves);

    // TEST 2: Blokada i bicie
    // Dodajemy czarnego skoczka na d6 (43) - to bicie
    // Dodajemy białego pionka na d2 (11) - to blokada (nie można wejść ani bić)
    board.pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::KNIGHT)] = (1ULL << 43);
    board.pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::PAWN)] |= (1ULL << 11);

    std::cout << "--- TEST 2: Blokada na d2, bicie na d6 ---\n";
    moves = board.generatePseudoLegalMoves(Color::WHITE);
    visualizeMoves(d4, moves);

    return 0;
}