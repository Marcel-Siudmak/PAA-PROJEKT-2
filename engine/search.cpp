#include "search.hpp"
#include "evaluate.hpp"
#include "types.hpp"
#include "tt.hpp"
#include <algorithm>
#include <vector>
#include <chrono>
#include <cstdlib>

namespace engine {

uint64_t nodesCounter = 0;

int minimax(Board &board, int depth, int alpha, int beta) {
  nodesCounter++;

  // 1. Sprawdzenie liścia (najszybsze)
  if (depth <= 0) {
    return evaluate(board);
  }

  // 2. Sprawdzenie TT
  int ttScore;
  Move ttMove(-1, -1, PieceType::NONE);
  if (g_tt.probe(board.zobristHash, depth, alpha, beta, ttScore, &ttMove)) {
    return ttScore;
  }

  // 3. Wykrywanie remisów (Repetition, Insufficient Material, 50-move rule)
  if (board.halfMoveClock >= 100 || board.isRepetition() || board.isInsufficientMaterial()) {
    return 0;
  }

  std::vector<Move> moves = board.generateLegalMoves();

  if (moves.empty()) {
    if (board.isInCheck(board.sideToMove)) {
      return -100000 - depth;
    }
    return 0; // Stalemate
  }

  // Wstępne sortowanie ruchów
  std::sort(moves.begin(), moves.end(), [&](const Move &a, const Move &b) {
      if (ttMove.from != -1) {
          if (a.from == ttMove.from && a.to == ttMove.to) return true;
          if (b.from == ttMove.from && b.to == ttMove.to) return false;
      }
      return (a.captured != PieceType::NONE) > (b.captured != PieceType::NONE);
  });

  int maxScore = -1000000;
  int originalAlpha = alpha;
  const Move* bestMovePtr = nullptr;

  for (const Move &move : moves) {
    Color side_making_move = board.sideToMove;
    board.makeMove(move, side_making_move);

    int score = -minimax(board, depth - 1, -beta, -alpha);

    board.unmakeMove(move, side_making_move);

    if (score > maxScore) {
      maxScore = score;
      bestMovePtr = &move;
    }
    
    if (maxScore > alpha) {
      alpha = maxScore;
    }
    if (alpha >= beta) {
      break; 
    }
  }

  // Zapis do TT
  TTFlag flag = TTFlag::EXACT;
  if (maxScore <= originalAlpha) flag = TTFlag::UPPER_BOUND;
  else if (maxScore >= beta) flag = TTFlag::LOWER_BOUND;
  
  g_tt.store(board.zobristHash, maxScore, depth, flag, bestMovePtr);

  return maxScore;
}

SearchResult getBestMove(Board &board, int depth) {
  auto start = std::chrono::high_resolution_clock::now();
  nodesCounter = 0;
  
  Move overallBestMove(0, 0, PieceType::NONE);
  int overallBestScore = -1000000;

  // Iterative Deepening
  for (int d = 1; d <= depth; d++) {
    std::vector<Move> moves = board.generateLegalMoves();
    if (moves.empty()) break;

    // Pobierz PV move z TT dla lepszego sortowania na poziomie root
    Move pvMove(-1, -1, PieceType::NONE);
    int dummyScore;
    g_tt.probe(board.zobristHash, d, -1000000, 1000000, dummyScore, &pvMove);

    std::sort(moves.begin(), moves.end(), [&](const Move &a, const Move &b) {
        if (pvMove.from != -1) {
            if (a.from == pvMove.from && a.to == pvMove.to) return true;
            if (b.from == pvMove.from && b.to == pvMove.to) return false;
        }
        return (a.captured != PieceType::NONE) > (b.captured != PieceType::NONE);
    });

    int alpha = -1000000;
    int beta = 1000000;
    int bestScore = -1000000;
    Move bestMove = moves[0];

    for (const Move &move : moves) {
      Color side_making_move = board.sideToMove;
      board.makeMove(move, side_making_move);
      int score = -minimax(board, d - 1, -beta, -alpha);
      board.unmakeMove(move, side_making_move);

      if (score > bestScore) {
        bestScore = score;
        bestMove = move;
      } else if (score == bestScore) {
        if (rand() % 2 == 0) bestMove = move;
      }
      if (bestScore > alpha) {
        alpha = bestScore;
      }
    }
    
    overallBestMove = bestMove;
    overallBestScore = bestScore;
    
    // Opcjonalnie: jeśli mamy już mata, możemy przerwać
    if (overallBestScore > 90000 || overallBestScore < -90000) break;
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end - start;

  return {overallBestMove, overallBestScore, nodesCounter, diff.count()};
}

} // namespace engine
