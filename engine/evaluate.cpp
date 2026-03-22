#include "evaluate.hpp"
#include "types.hpp"

namespace engine {

const int PAWN_VALUE = 100;
const int KNIGHT_VALUE = 300;
const int BISHOP_VALUE = 300;
const int ROOK_VALUE = 500;
const int QUEEN_VALUE = 900;
const int MATE_SCORE = 100000;

int getPieceValue(PieceType type) {
  switch (type) {
  case PieceType::PAWN:
    return PAWN_VALUE;
  case PieceType::KNIGHT:
    return KNIGHT_VALUE;
  case PieceType::BISHOP:
    return BISHOP_VALUE;
  case PieceType::ROOK:
    return ROOK_VALUE;
  case PieceType::QUEEN:
    return QUEEN_VALUE;
  default:
    return 0;
  }
}

int popcount(uint64_t v) { return __builtin_popcountll(v); }

int evaluate(const Board &board) {
  int score = 0;

  for (int p = 0; p < 5; p++) {
    uint64_t whitePieces = board.pieces[static_cast<int>(Color::WHITE)][p];
    uint64_t blackPieces = board.pieces[static_cast<int>(Color::BLACK)][p];

    int val = getPieceValue(static_cast<PieceType>(p));

    score += popcount(whitePieces) * val;
    score -= popcount(blackPieces) * val;
  }

  // Dodatnia wartość oznacza przewagę gracza, który MA RUCH.
  return (board.sideToMove == Color::WHITE) ? score : -score;
}

} // namespace engine
