#pragma once

#include <cstdint>
#include <vector>

#include "types.hpp"
#include "move.hpp"


struct GameState {
    uint8_t castlingRights;
    uint64_t enPassantSquare;
    // Tu w przyszłości dodasz: halfMoveClock (do zasady 50 ruchów) 
    // oraz hash klucza Zobrista (do wykrywania powtórzeń).
};

class Board {
public:
    // --- Dane Stanu Gry ---
    std::vector<GameState> history;

    // Tablica bitboardów: [0=Białe, 1=Czarne][0-5=Typ figury]
    uint64_t pieces[2][6];
    
    // Flagi roszad: 4 bity (np. 0b0001 - Białe krótka, 0b0010 - Białe długa, itd.)
    uint8_t castlingRights;
    
    // Pole możliwe do bicia w przelocie (tylko jeden bit może być zapalony)
    uint64_t enPassantSquare;
    
    Color sideToMove;

    // --- Konstruktor ---
    Board() : 
        castlingRights(0x0F), // Na początku wszyscy mają prawo do obu roszad
        enPassantSquare(0ULL),
        sideToMove(Color::WHITE) 
    {
        // Inicjalizacja tablicy figur stałymi
        pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::PAWN)]   = WHITE_PAWNS_INIT;
        pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::KNIGHT)] = WHITE_KNIGHTS_INIT;
        pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::BISHOP)] = WHITE_BISHOPS_INIT;
        pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::ROOK)]   = WHITE_ROOKS_INIT;
        pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::QUEEN)]  = WHITE_QUEENS_INIT;
        pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::KING)]   = WHITE_KING_INIT;

        pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::PAWN)]   = BLACK_PAWNS_INIT;
        pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::KNIGHT)] = BLACK_KNIGHTS_INIT;
        pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::BISHOP)] = BLACK_BISHOPS_INIT;
        pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::ROOK)]   = BLACK_ROOKS_INIT;
        pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::QUEEN)]  = BLACK_QUEENS_INIT;
        pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::KING)]   = BLACK_KING_INIT;
    }

    // --- Metody pomocnicze (do zaimplementowania w .cpp) ---
    uint64_t getAllOccupied() const;
    uint64_t getColorOccupied(Color color) const;
    uint64_t getPawnSinglePushes(Color color) const;
    uint64_t getPawnDoublePushes(Color color) const;
    uint64_t getPawnAttacks(Color color) const;
    uint64_t getPawnAttacks(Color color, uint64_t pawnBitboard) const;
    uint64_t getKnightAttacks(uint64_t knights) const;
    uint64_t getKingAttacks(uint64_t king) const;
    uint64_t getTowerAttacks(int sq) const;
    uint64_t getBishopAttacks(int sq) const;
    uint64_t getQueenAttacks(int sq) const;
    bool isSquareAttacked(int sq, Color side) const;
    std::vector<Move> generatePseudoLegalMoves(Color side) const;
    PieceType getPieceAt(int sq, Color color) const;

    bool isInCheck(Color side) const;
    GameStatus getGameStatus();


    std::vector<Move> generateLegalMoves(Color side);
    std::vector<Move> generateLegalMoves();



    void makeMove(const Move& m, Color side);
    void unmakeMove(const Move& m, Color side);
    uint64_t perft(int depth);

private:
    // Inicjalizacja pozycji startowej
    // Białe
    static constexpr uint64_t WHITE_PAWNS_INIT   = 0x000000000000FF00ULL;
    static constexpr uint64_t WHITE_KNIGHTS_INIT = 0x0000000000000042ULL;
    static constexpr uint64_t WHITE_BISHOPS_INIT = 0x0000000000000024ULL;
    static constexpr uint64_t WHITE_ROOKS_INIT   = 0x0000000000000081ULL;
    static constexpr uint64_t WHITE_QUEENS_INIT  = 0x0000000000000008ULL; // d1
    static constexpr uint64_t WHITE_KING_INIT    = 0x0000000000000010ULL; // e1
    // Czarne
    static constexpr uint64_t BLACK_PAWNS_INIT   = 0x00FF000000000000ULL;
    static constexpr uint64_t BLACK_KNIGHTS_INIT = 0x4200000000000000ULL;
    static constexpr uint64_t BLACK_BISHOPS_INIT = 0x2400000000000000ULL;
    static constexpr uint64_t BLACK_ROOKS_INIT   = 0x8100000000000000ULL;
    static constexpr uint64_t BLACK_QUEENS_INIT  = 0x0800000000000000ULL; // d8
    static constexpr uint64_t BLACK_KING_INIT    = 0x1000000000000000ULL; // e8

    // Maski pomocnicze
    // Maski wierszy
    static constexpr uint64_t RANK_4_MASK = 0x00000000FF000000ULL; // 4. linia (dla białych)
    static constexpr uint64_t RANK_5_MASK = 0x000000FF00000000ULL; // 5. linia (dla czarnych)

    // Maski kolumn
    static constexpr uint64_t FILE_A = 0x0101010101010101ULL; // Linia a
    static constexpr uint64_t FILE_H = 0x8080808080808080ULL; // Linia h
    static constexpr uint64_t FILE_B = FILE_A << 1;
    static constexpr uint64_t FILE_G = FILE_H >> 1;
    static constexpr uint64_t FILE_AB = FILE_A | FILE_B;
    static constexpr uint64_t FILE_GH = FILE_G | FILE_H;
};