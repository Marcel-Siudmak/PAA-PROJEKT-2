#include "board.hpp"

uint64_t Board::getPawnSinglePushes(Color color) const {
    uint64_t pawns = pieces[static_cast<int>(color)][static_cast<int>(PieceType::PAWN)];
    uint64_t emptySquares = ~getAllOccupied();

    if (color == Color::WHITE) {
        // Białe piony przesuwają się w górę (bit przesunięty o 8 w lewo)
        return ((pawns << 8) & emptySquares);
    } else {
        // Czarne piony przesuwają się w dół (bit przesunięty o 8 w prawo)
        return ((pawns >> 8) & emptySquares);
    }
}

uint64_t Board::getPawnDoublePushes(Color color) const {
    uint64_t singlePushes = getPawnSinglePushes(color);
    uint64_t emptySquares = ~getAllOccupied();


    if (color == Color::WHITE) {
        // Białe piony mogą wykonać podwójny ruch, jeśli są na 2. linii (bit przesunięty o 16 w lewo)
        return ((singlePushes << 8) & emptySquares & RANK_4_MASK);
    } else {
        // Czarne piony mogą wykonać podwójny ruch, jeśli są na 7. linii (bit przesunięty o 16 w prawo)
        return ((singlePushes >> 8) & emptySquares & RANK_5_MASK);
    }
}

uint64_t Board::getPawnAttacks(Color color) const {
    uint64_t pawns = pieces[static_cast<int>(color)][static_cast<int>(PieceType::PAWN)];
    uint64_t opponentPieces = getColorOccupied(color == Color::WHITE ? Color::BLACK : Color::WHITE);



    if (color == Color::WHITE) {
    // Atak w stronę linii A (bit << 7) - usuwamy to, co zawinęło się na linię H
    uint64_t attackLeft = (pawns << 7) & ~FILE_H; 
    // Atak w stronę linii H (bit << 9) - usuwamy to, co zawinęło się na linię A
    uint64_t attackRight = (pawns << 9) & ~FILE_A;
    return (attackLeft | attackRight) & opponentPieces;
    } else {
        // Atak w stronę linii H (bit >> 7) - usuwamy to, co zawinęło się na linię A
        uint64_t attackRight = (pawns >> 7) & ~FILE_A;
        // Atak w stronę linii A (bit >> 9) - usuwamy to, co zawinęło się na linię H
        uint64_t attackLeft = (pawns >> 9) & ~FILE_H;
        return (attackLeft | attackRight) & opponentPieces;
    }
}