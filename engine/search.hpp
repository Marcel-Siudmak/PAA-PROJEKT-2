#pragma once
#include "board.hpp"
#include "move.hpp"

namespace engine {

struct SearchResult {
  Move bestMove;
  int score;
};

SearchResult getBestMove(Board &board, int depth);

} // namespace engine
