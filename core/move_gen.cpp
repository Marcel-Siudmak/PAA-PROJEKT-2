#include "board.hpp"
#include "magic.hpp"

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

uint64_t Board::getKnightAttacks(uint64_t knights) const {
    // Ruchy skoczka: 8 możliwych kierunków
    uint64_t attacks = 0ULL;

    // 2 w górę + 1 w prawo
    attacks |= (knights << 17) & ~FILE_A;
    // 2 w górę + 1 w lewo
    attacks |= (knights << 15) & ~FILE_H;
    // 2 w dół + 1 w prawo
    attacks |= (knights >> 15) & ~FILE_A;
    // 2 w dół + 1 w lewo
    attacks |= (knights >> 17) & ~FILE_H;
    // 1 w górę + 2 w prawo
    attacks |= (knights << 10) & ~FILE_AB;
    // 1 w górę + 2 w lewo
    attacks |= (knights << 6) & ~FILE_GH;
    // 1 w dół + 2 w prawo
    attacks |= (knights >> 6) & ~FILE_AB;
    // 1 w dół + 2 w lewo
    attacks |= (knights >> 10) & ~FILE_GH;

    return attacks;
}

uint64_t Board::getKingAttacks(uint64_t king) const {
    uint64_t attacks = 0ULL;

    // Ruchy króla: 8 możliwych kierunków
    attacks |= (king << 8); // w górę
    attacks |= (king >> 8); // w dół
    attacks |= (king << 1) & ~FILE_A; // w prawo
    attacks |= (king >> 1) & ~FILE_H; // w lewo
    attacks |= (king << 9) & ~FILE_A; // w górę + w prawo
    attacks |= (king << 7) & ~FILE_H; // w górę + w lewo
    attacks |= (king >> 7) & ~FILE_A; // w dół + w prawo
    attacks |= (king >> 9) & ~FILE_H; // w dół + w lewo

    return attacks;
}

uint64_t Board::getTowerAttacks(int sq) const {
    // Pobieramy wszystkie figury na planszy (blokady)
    uint64_t occupied = getAllOccupied();
    // Wywołujemy naszą super szybką funkcję z magic.cpp
    return get_rook_attacks(sq, occupied);
}

uint64_t Board::getBishopAttacks(int sq) const {
    uint64_t occupied = getAllOccupied();
    return get_bishop_attacks(sq, occupied);
}

uint64_t Board::getQueenAttacks(int sq) const {
    uint64_t occupied = getAllOccupied();
    // Hetman to po prostu wieża i goniec w jednym!
    return get_rook_attacks(sq, occupied) | get_bishop_attacks(sq, occupied);
}



bool Board::isSquareAttacked(int sq, Color side) const {
    

    // 1. Skoczki
    if (getKnightAttacks(1ULL << sq) & pieces[static_cast<int>(side)][static_cast<int>(PieceType::KNIGHT)]) return true;

    // 2. Król
    if (getKingAttacks(1ULL << sq) & pieces[static_cast<int>(side)][static_cast<int>(PieceType::KING)]) return true;

    // 3. Wieże i Hetmany
    if (getTowerAttacks(sq) & (pieces[static_cast<int>(side)][static_cast<int>(PieceType::ROOK)] | pieces[static_cast<int>(side)][static_cast<int>(PieceType::QUEEN)])) return true;

    // 4. Gońce i Hetmany
    if (getBishopAttacks(sq) & (pieces[static_cast<int>(side)][static_cast<int>(PieceType::BISHOP)] | pieces[static_cast<int>(side)][static_cast<int>(PieceType::QUEEN)])) return true;

    // 5. Pionki (używamy Twoich stałych FILE_A i FILE_H z magic.cpp)
    uint64_t pawns = pieces[static_cast<int>(side)][static_cast<int>(PieceType::PAWN)];
    if (side == Color::WHITE) {
        // Ataki białych pionków idą "do góry" (z perspektywy sq, patrzymy kto bije z dołu)
        if (((1ULL << sq) >> 7 & 0x7f7f7f7f7f7f7f7fULL & pawns) || 
            ((1ULL << sq) >> 9 & 0xfefefefefefefefeULL & pawns)) return true;
    } else {
        // Ataki czarnych pionków (patrzymy kto bije z góry)
        if (((1ULL << sq) << 7 & 0xfefefefefefefefeULL & pawns) || 
            ((1ULL << sq) << 9 & 0x7f7f7f7f7f7f7f7fULL & pawns)) return true;
    }

    return false;
}