#pragma once
#include <array>
#include <cstdint>
#include <cstring>

// ==============================
// NNUE — Efficiently Updatable Neural Network Evaluator
// Architektura: 768 → 128 (ReLU) → 32 (ReLU) → 1 (tanh × 10000)
// Wagi ładowane z binarnego pliku nn.bin
// ==============================

namespace nnue {

// Rozmiary warstw
static constexpr int IN  = 768;
static constexpr int H1  = 128;
static constexpr int H2  = 32;
static constexpr int OUT = 1;

struct Weights {
    // Warstwa 1: IN → H1
    float w1[H1][IN];
    float b1[H1];
    // Warstwa 2: H1 → H2
    float w2[H2][H1];
    float b2[H2];
    // Warstwa 3: H2 → OUT
    float w3[OUT][H2];
    float b3[OUT];
};

// Globalne wagi + flaga
extern Weights g_weights;
extern bool    g_loaded;

// Załaduj wagi z pliku .bin
// Format: sekwencyjnie float32 little-endian:
//   w1 (H1*IN), b1 (H1), w2 (H2*H1), b2 (H2), w3 (OUT*H2), b3 (OUT)
bool load(const char *path);

// Forward pass, features musi mieć IN elementów (0.0 lub 1.0)
// Zwraca wynik w centypionach (zakres ok. ±10000)
float forward(const float *features);

} // namespace nnue
