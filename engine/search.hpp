#pragma once
#include "board.hpp"
#include "move.hpp"
#include <cstdint>

namespace engine {

struct SearchResult {
  Move bestMove;
  int score;
  uint64_t nodes;
  double timeSeconds;
};

SearchResult getBestMove(Board &board, int depth);

} // namespace engine
