#pragma once
#include "types.hpp"

struct Move {
    int from;
    int to;
    PieceType piece;
    PieceType captured;
    PieceType promotion;

    Move(int f, int t, PieceType p, PieceType c = PieceType::NONE, PieceType prom = PieceType::NONE)
        : from(f), to(t), piece(p), captured(c), promotion(prom) {}
};