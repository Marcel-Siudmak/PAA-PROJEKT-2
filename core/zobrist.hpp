#pragma once
#include <cstdint>

// Zobrist keys arrays
extern uint64_t ZOBRIST_PIECES[2][6][64]; // [Color][PieceType][Square]
extern uint64_t ZOBRIST_CASTLING[16];     // Castling rights 0-15
extern uint64_t ZOBRIST_EP[64];           // En passant square (0-63)
extern uint64_t ZOBRIST_SIDE;             // Side to move (Black)

void init_zobrist();
