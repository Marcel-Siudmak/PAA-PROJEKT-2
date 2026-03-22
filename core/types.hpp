#pragma once

enum class GameStatus { 
    RUNNING, 
    CHECKMATE, 
    STALEMATE,
    DRAW_FIFTY_MOVES,
    DRAW_REPETITION,
    DRAW_INSUFFICIENT_MATERIAL 
};
enum class Color { WHITE, BLACK };
enum class PieceType { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NONE };
enum MoveType { NORMAL, CASTLING, EN_PASSANT, PROMOTION };

// Używamy bitów: 1=WK, 2=WQ, 4=BK, 8=BQ
enum CastlingRights {
    NO_CASTLING = 0,
    WHITE_OO = 1, WHITE_OOO = 2,
    BLACK_OO = 4, BLACK_OOO = 8,
    ALL_CASTLING = 15
};

// Funkcja pomocnicza, o której rozmawialiśmy
static inline Color oppositeColor(Color c) {
    return (c == Color::WHITE) ? Color::BLACK : Color::WHITE;
}