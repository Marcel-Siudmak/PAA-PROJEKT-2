#include "evaluate.hpp"
#include "nnue.hpp"
#include "types.hpp"
#include <array>
#include <cstdio>

namespace engine {

// ==============================
// STARA HEURYSTYKA (PST-based)
// ==============================

static const int PAWN_VALUE   = 100;
static const int KNIGHT_VALUE = 320;
static const int BISHOP_VALUE = 330;
static const int ROOK_VALUE   = 500;
static const int QUEEN_VALUE  = 900;

static const int PAWN_PST[64] = {
     0,  0,  0,   0,  0,  0,   0,  0,
     5, 10, 10, -20,-20, 10,  10,  5,
     5, -5,-10,   0,  0,-10,  -5,  5,
     0,  0,  0,  20, 20,  0,   0,  0,
     5,  5, 10,  25, 25, 10,   5,  5,
    10, 10, 20,  30, 30, 20,  10, 10,
    50, 50, 50,  50, 50, 50,  50, 50,
     0,  0,  0,   0,  0,  0,   0,  0
};
static const int KNIGHT_PST[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};
static const int BISHOP_PST[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};
static const int ROOK_PST[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};
static const int QUEEN_PST[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};
static const int KING_PST[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

static const int *PIECE_PST[6] = {
    PAWN_PST, KNIGHT_PST, BISHOP_PST,
    ROOK_PST, QUEEN_PST,  KING_PST
};
static const int PIECE_VAL[6] = {
    PAWN_VALUE, KNIGHT_VALUE, BISHOP_VALUE,
    ROOK_VALUE, QUEEN_VALUE,  0
};

int heuristic_evaluate(const Board &board) {
    int score = 0;
    for (int p = 0; p < 6; p++) {
        uint64_t wb = board.pieces[static_cast<int>(Color::WHITE)][p];
        uint64_t bb = board.pieces[static_cast<int>(Color::BLACK)][p];
        const int *pst = PIECE_PST[p];
        int val = PIECE_VAL[p];
        while (wb) {
            int sq = __builtin_ctzll(wb);
            score += val + pst[sq];
            wb &= (wb - 1);
        }
        while (bb) {
            int sq = __builtin_ctzll(bb);
            score -= (val + pst[sq ^ 56]);
            bb &= (bb - 1);
        }
    }
    return (board.sideToMove == Color::WHITE) ? score : -score;
}

// ==============================
// KONWERSJA PLANSZY NA CECHY
// ==============================

// Układ: feature[color*6*64 + piece_type*64 + square]
// Spójny z ml/board_encoder.py
std::array<float, 768> boardToFeatures(const Board &board) {
    std::array<float, 768> feat{};
    for (int c = 0; c < 2; c++) {
        for (int p = 0; p < 6; p++) {
            uint64_t bb = board.pieces[c][p];
            while (bb) {
                int sq = __builtin_ctzll(bb);
                feat[c * 6 * 64 + p * 64 + sq] = 1.0f;
                bb &= (bb - 1);
            }
        }
    }
    return feat;
}

// ==============================
// ŁADOWANIE NNUE
// ==============================

bool loadNNUE(const char *path) {
    bool ok = nnue::load(path);
    if (ok) {
        fprintf(stdout, "[NNUE] Załadowano wagi z: %s\n", path);
    } else {
        fprintf(stdout, "[NNUE] Nie znaleziono %s — używam heurystyki\n", path);
    }
    fflush(stdout);
    return ok;
}

// ==============================
// GŁÓWNA FUNKCJA evaluate()
// ==============================

int evaluate(const Board &board) {
    if (nnue::g_loaded) {
        auto feat = boardToFeatures(board);
        float raw = nnue::forward(feat.data());
        // raw jest z perspektywy białych w centypionach
        int score = static_cast<int>(raw);
        return (board.sideToMove == Color::WHITE) ? score : -score;
    }
    return heuristic_evaluate(board);
}

} // namespace engine
