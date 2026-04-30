#pragma once
#include "board.hpp"
#include <array>

namespace engine {

// Stara heurystyka (PST-based), zawsze dostępna jako fallback
int heuristic_evaluate(const Board &board);

// Nowa ocena: NNUE jeśli wagi załadowane, inaczej heurystyka
int evaluate(const Board &board);

// Konwersja stanu planszy na wektor 768 cech binarnych:
//   feature[color*6*64 + piece_type*64 + square] = 1.0 jeśli figura tam stoi
// color: 0=białe, 1=czarne | piece_type: 0=pion…5=król
std::array<float, 768> boardToFeatures(const Board &board);

// Załaduj wagi NNUE z pliku (wywołaj raz przy starcie)
bool loadNNUE(const char *path);

} // namespace engine
