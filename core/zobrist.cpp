#include "zobrist.hpp"
#include <random>

uint64_t ZOBRIST_PIECES[2][6][64];
uint64_t ZOBRIST_CASTLING[16];
uint64_t ZOBRIST_EP[64];
uint64_t ZOBRIST_SIDE;

void init_zobrist() {
    std::mt19937_64 rng(0x12345678ULL); // Ustalony seed (deterministic)
    
    for (int c = 0; c < 2; c++) {
        for (int p = 0; p < 6; p++) {
            for (int sq = 0; sq < 64; sq++) {
                ZOBRIST_PIECES[c][p][sq] = rng();
            }
        }
    }
    for (int i = 0; i < 16; i++) {
        ZOBRIST_CASTLING[i] = rng();
    }
    for (int i = 0; i < 64; i++) {
        ZOBRIST_EP[i] = rng();
    }
    ZOBRIST_SIDE = rng();
}
