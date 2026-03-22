#include "search.hpp"
#include "evaluate.hpp"
#include "types.hpp"
#include <algorithm>
#include <vector>
#include <chrono>
#include <cstdlib>

namespace engine {

uint64_t nodesCounter = 0;

int minimax(Board &board, int depth, int alpha, int beta) {
  nodesCounter++;
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

  // Wstępne sortowanie ruchów: bicia na początek! (Move Ordering)
  // Drastycznie zwiększa to szanse na szybkie odcięcie Alfa-Beta.
  std::sort(moves.begin(), moves.end(), [](const Move &a, const Move &b) {
      return (a.captured != PieceType::NONE) > (b.captured != PieceType::NONE);
  });

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
  auto start = std::chrono::high_resolution_clock::now();
  nodesCounter = 1; // Liczymy pierwszy (root) wezel
  
  std::vector<Move> moves = board.generateLegalMoves();
  if (moves.empty()) {
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    return {Move(0, 0, PieceType::NONE), 0, nodesCounter, diff.count()};
  }

  std::sort(moves.begin(), moves.end(), [](const Move &a, const Move &b) {
      return (a.captured != PieceType::NONE) > (b.captured != PieceType::NONE);
  });

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
    } else if (score == bestScore) {
      // Tie-breaker: prosta losowość przy tych samych ocenach
      if (rand() % 2 == 0) {
        bestMove = move;
      }
    }
    if (bestScore > alpha) {
      alpha = bestScore;
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end - start;

  return {bestMove, bestScore, nodesCounter, diff.count()};
}

} // namespace engine
