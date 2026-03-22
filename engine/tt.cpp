#include "tt.hpp"
#include <algorithm>

namespace engine {

TranspositionTable g_tt(64); // Domyślnie 64MB

TranspositionTable::TranspositionTable(size_t sizeMB) {
    numEntries = (sizeMB * 1024 * 1024) / sizeof(TTEntry);
    table.resize(numEntries);
}

void TranspositionTable::clear() {
    std::fill(table.begin(), table.end(), TTEntry());
}

void TranspositionTable::store(uint64_t hash, int score, int depth, TTFlag flag, const Move* move) {
    size_t idx = hash % numEntries;
    
    // Prosta strategia zastępowania: zawsze nadpisuj, jeśli głębokość jest większa lub równa
    if (depth >= table[idx].depth) {
        table[idx].hash = hash;
        table[idx].score = score;
        table[idx].depth = depth;
        table[idx].flag = flag;
        if (move) {
            table[idx].moveFrom = move->from;
            table[idx].moveTo = move->to;
        } else {
            table[idx].moveFrom = -1;
            table[idx].moveTo = -1;
        }
    }
}

bool TranspositionTable::probe(uint64_t hash, int depth, int alpha, int beta, int& score, Move* bestMove) {
    size_t idx = hash % numEntries;
    
    if (table[idx].hash == hash) {
        // Zawsze wyciągamy ruch dla Move Ordering (PV Move)
        if (table[idx].moveFrom != -1 && bestMove) {
            bestMove->from = table[idx].moveFrom;
            bestMove->to = table[idx].moveTo;
        }

        if (table[idx].depth >= depth) {
            if (table[idx].flag == TTFlag::EXACT) {
                score = table[idx].score;
                return true;
            }
            if (table[idx].flag == TTFlag::LOWER_BOUND && table[idx].score >= beta) {
                score = table[idx].score;
                return true;
            }
            if (table[idx].flag == TTFlag::UPPER_BOUND && table[idx].score <= alpha) {
                score = table[idx].score;
                return true;
            }
        }
    }
    return false;
}

} // namespace engine
