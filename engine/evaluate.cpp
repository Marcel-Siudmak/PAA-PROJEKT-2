#include "evaluate.hpp"
#include "types.hpp"

namespace engine {

const int PAWN_VALUE = 100;
const int KNIGHT_VALUE = 320;
const int BISHOP_VALUE = 330;
const int ROOK_VALUE = 500;
const int QUEEN_VALUE = 900;
const int MATE_SCORE = 100000;

// Tablice wartości pól (PST) - perspektywa Białych (a1 = 0, h8 = 63)
// Wartości dodatnie premiują zajmowanie danego pola.

const int PAWN_PST[64] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    5, 10, 10,-20,-20, 10, 10,  5,
    5, -5,-10,  0,  0,-10, -5,  5,
    0,  0,  0, 20, 20,  0,  0,  0,
    5,  5, 10, 25, 25, 10,  5,  5,
   10, 10, 20, 30, 30, 20, 10, 10,
   50, 50, 50, 50, 50, 50, 50, 50,
    0,  0,  0,  0,  0,  0,  0,  0
};

const int KNIGHT_PST[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

const int BISHOP_PST[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

const int ROOK_PST[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};

const int QUEEN_PST[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

const int KING_PST[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

const int* PIECE_PST[6] = {
    PAWN_PST, KNIGHT_PST, BISHOP_PST, ROOK_PST, QUEEN_PST, KING_PST
};

int getPieceValue(PieceType type) {
  switch (type) {
  case PieceType::PAWN:   return PAWN_VALUE;
  case PieceType::KNIGHT: return KNIGHT_VALUE;
  case PieceType::BISHOP: return BISHOP_VALUE;
  case PieceType::ROOK:   return ROOK_VALUE;
  case PieceType::QUEEN:  return QUEEN_VALUE;
  default: return 0;
  }
}

int evaluate(const Board &board) {
  int score = 0;

  for (int p = 0; p < 6; p++) {
    uint64_t whitePieces = board.pieces[static_cast<int>(Color::WHITE)][p];
    uint64_t blackPieces = board.pieces[static_cast<int>(Color::BLACK)][p];

    int val = getPieceValue(static_cast<PieceType>(p));
    const int* pst = PIECE_PST[p];

    while (whitePieces) {
      int sq = __builtin_ctzll(whitePieces);
      score += val + pst[sq];
      whitePieces &= (whitePieces - 1);
    }

    while (blackPieces) {
      int sq = __builtin_ctzll(blackPieces);
      // Dla czarnych lustrzane odbicie wierszy (sq ^ 56)
      score -= (val + pst[sq ^ 56]);
      blackPieces &= (blackPieces - 1);
    }
  }

  return (board.sideToMove == Color::WHITE) ? score : -score;
}

} // namespace engine
