#include "search.hpp"
#include "evaluate.hpp"
#include "types.hpp"
#include <vector>

namespace engine {

int minimax(Board &board, int depth, int alpha, int beta) {
  std::vector<Move> moves = board.generateLegalMoves();

  if (moves.empty()) {
    if (board.isInCheck(board.sideToMove)) {
      // Im szybciej mat, tym gorzej dla matowanego (więc zwracamy negatywną,
      // bliską -MATE_SCORE ocenę z perspektywy matowanego).
      // Dołączenie 'depth' preferuje szybsze maty.
      return -100000 - depth;
    }
    return 0; // Stalemate
  }

  if (board.isInsufficientMaterial() || board.isRepetition() ||
      board.halfMoveClock >= 100) {
    return 0;
  }

  if (depth == 0) {
    return evaluate(board);
  }

  int maxScore = -1000000;

  for (const Move &move : moves) {
    Color side_making_move = board.sideToMove;
    board.makeMove(move, side_making_move);

    // Wywołanie NegaMax z Alpha-Beta
    int score = -minimax(board, depth - 1, -beta, -alpha);

    board.unmakeMove(move, side_making_move);

    if (score > maxScore) {
      maxScore = score;
    }
    if (maxScore > alpha) {
      alpha = maxScore;
    }
    if (alpha >= beta) {
      break; // Odcięcie (Pruning)
    }
  }

  return maxScore;
}

SearchResult getBestMove(Board &board, int depth) {
  std::vector<Move> moves = board.generateLegalMoves();
  if (moves.empty()) {
    return {Move(0, 0, PieceType::NONE), 0};
  }

  int alpha = -1000000;
  int beta = 1000000;
  int bestScore = -1000000;
  Move bestMove = moves[0];

  for (const Move &move : moves) {
    Color side_making_move = board.sideToMove;
    board.makeMove(move, side_making_move);
    int score = -minimax(board, depth - 1, -beta, -alpha);
    board.unmakeMove(move, side_making_move);

    if (score > bestScore) {
      bestScore = score;
      bestMove = move;
    }
    if (bestScore > alpha) {
      alpha = bestScore;
    }
  }

  return {bestMove, bestScore};
}

} // namespace engine
