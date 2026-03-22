#pragma once
#include "types.hpp"

struct Move {
    int from, to;
    PieceType piece, captured, promotion;
    MoveType type;

    Move(int f, int t, PieceType p, PieceType c = PieceType::NONE, 
         PieceType prom = PieceType::NONE, MoveType ty = NORMAL)
        : from(f), to(t), piece(p), captured(c), promotion(prom), type(ty) {}
};