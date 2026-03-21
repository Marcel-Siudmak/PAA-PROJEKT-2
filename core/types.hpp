#pragma once

enum class Color { WHITE, BLACK };
enum class PieceType { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NONE };

// Funkcja pomocnicza, o której rozmawialiśmy
static inline Color oppositeColor(Color c) {
    return (c == Color::WHITE) ? Color::BLACK : Color::WHITE;
}