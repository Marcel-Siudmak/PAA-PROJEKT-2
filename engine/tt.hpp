#pragma once
#include "../core/move.hpp"
#include <cstdint>
#include <vector>

namespace engine {

enum class TTFlag : uint8_t {
    EXACT,      // Wynik dokładny
    LOWER_BOUND, // Wynik >= (Beta cutoff)
    UPPER_BOUND  // Wynik <= (Alpha cutoff)
};

struct TTEntry {
    uint64_t hash;
    int score;
    int depth;
    TTFlag flag;
    int moveFrom; // Spakowany ruch: -1 jeśli brak
    int moveTo;
    
    TTEntry() : hash(0), score(0), depth(-1), flag(TTFlag::EXACT), moveFrom(-1), moveTo(-1) {}
};

class TranspositionTable {
public:
    TranspositionTable(size_t sizeMB);
    
    void clear();
    void store(uint64_t hash, int score, int depth, TTFlag flag, const Move* move);
    bool probe(uint64_t hash, int depth, int alpha, int beta, int& score, Move* bestMove);

private:
    std::vector<TTEntry> table;
    size_t numEntries;
};

extern TranspositionTable g_tt;

} // namespace engine
