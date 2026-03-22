#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <limits>
#include "board.hpp"
#include "types.hpp"
#include "move.hpp"
#include "magic.hpp"
#include "zobrist.hpp"

// --- POMOCNICZE FUNKCJE WIZUALIZACJI ---

std::string sqToNotation(int sq) {
    return std::string(1, static_cast<char>('a' + (sq % 8))) + std::to_string((sq / 8) + 1);
}

void printBoard(const Board& board) {
    std::cout << "\n      a b c d e f g h\n";
    std::cout << "    +-----------------+\n";
    for (int r = 7; r >= 0; r--) {
        std::cout << "  " << r + 1 << " | ";
        for (int f = 0; f < 8; f++) {
            int sq = r * 8 + f;
            char pieceChar = '.';

            for (int p = 0; p < 6; p++) {
                uint64_t bit = (1ULL << sq);
                if (board.pieces[static_cast<int>(Color::WHITE)][p] & bit) {
                    pieceChar = "PNBRQK"[p];
                } else if (board.pieces[static_cast<int>(Color::BLACK)][p] & bit) {
                    pieceChar = "pnbrqk"[p];
                }
            }
            std::cout << pieceChar << " ";
        }
        std::cout << "| " << r + 1 << "\n";
    }
    std::cout << "    +-----------------+\n";
    std::cout << "      a b c d e f g h\n\n";
    std::cout << "Kolej: " << (board.sideToMove == Color::WHITE ? "BIALE (DUZE)" : "CZARNE (male)") << "\n";
}

// --- FUNKCJA USTAWIAJĄCA POZYCJĘ STARTOWĄ ---
// Jeśli nie masz jeszcze parsera FEN, użyj tego do testów:
void setupStartPosition(Board& b) {
    for(int i=0; i<2; i++) for(int j=0; j<7; j++) b.pieces[i][j] = 0ULL;

    // Pionki
    b.pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::PAWN)] = 0x000000000000FF00ULL;
    b.pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::PAWN)] = 0x00FF000000000000ULL;
    // Reszta figur (uproszczone dla testu)
    b.pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::ROOK)] = 0x81ULL;
    b.pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::ROOK)] = 0x8100000000000000ULL;
    b.pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::KNIGHT)] = 0x42ULL;
    b.pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::KNIGHT)] = 0x4200000000000000ULL;
    b.pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::BISHOP)] = 0x24ULL;
    b.pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::BISHOP)] = 0x2400000000000000ULL;
    b.pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::QUEEN)] = 0x08ULL;
    b.pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::QUEEN)] = 0x0800000000000000ULL;
    b.pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::KING)] = 0x10ULL;
    b.pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::KING)] = 0x1000000000000000ULL;
    
    b.sideToMove = Color::WHITE;
}

// --- GŁÓWNA PĘTLA TESTOWA ---

int main() {
    srand(time(NULL));
    init_magic_tables();
    init_zobrist();

    Board board;
    setupStartPosition(board);

    int moveCount = 0;

    while (true) {
        printBoard(board);

        // Sprawdzamy stan gry na początku tury
        GameStatus status = board.getGameStatus();

        if (status != GameStatus::RUNNING) {
            if (status == GameStatus::CHECKMATE) {
                std::cout << "\n==============================\n";
                std::cout << "       SZACH-MAT!             \n";
                std::cout << " Wygrywaja: " << (board.sideToMove == Color::WHITE ? "CZARNE" : "BIALE") << "\n";
                std::cout << "==============================\n";
            } else if (status == GameStatus::STALEMATE) {
                std::cout << "\n--- PAT (REMIS) ---\n";
            } else if (status == GameStatus::DRAW_FIFTY_MOVES) {
                std::cout << "\n--- REMIS (Zasada 50 ruchow) ---\n";
            } else if (status == GameStatus::DRAW_REPETITION) {
                std::cout << "\n--- REMIS (Trzykrotne powtorzenie pozycji) ---\n";
            } else if (status == GameStatus::DRAW_INSUFFICIENT_MATERIAL) {
                std::cout << "\n--- REMIS (Martwa pozycja - brak materialu) ---\n";
            }
            break; // Koniec pętli gry
        }

        if (board.isInCheck(board.sideToMove)) {
            std::cout << "!!! SZACH !!!\n";
        }

        // 1. Generuj legalne ruchy
        std::vector<Move> moves = board.generateLegalMoves();

        std::cout << "Ruch nr " << moveCount + 1 << ". Mozliwych opcji: " << moves.size() << "\n";
        std::cout << "-> Nacisnij ENTER, aby wykonac losowy ruch...";
        
        // Czekanie na interakcję
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        // 3. Wybierz i wykonaj losowy ruch
        Move m = moves[rand() % moves.size()];
        
        std::cout << "\nWYKONANO: " << sqToNotation(m.from) << " -> " << sqToNotation(m.to);
        if (m.captured != PieceType::NONE) std::cout << " (BICIE!)";
        std::cout << "\n" << std::string(30, '-') << "\n";

        board.makeMove(m, board.sideToMove);
        moveCount++;
    }

    return 0;
}