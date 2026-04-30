#include "nnue.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace nnue {

Weights g_weights;
bool    g_loaded = false;

// ── Pomocnicze: ReLU i tanh ────────────────────────────────────────────────
static inline float relu(float x) { return x > 0.0f ? x : 0.0f; }

// ── Ładowanie wag z pliku binarnego ────────────────────────────────────────
bool load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    bool ok = true;
    auto rd = [&](void *dst, size_t n) {
        if (ok && fread(dst, sizeof(float), n, f) != n) ok = false;
    };

    rd(g_weights.w1, H1 * IN);
    rd(g_weights.b1, H1);
    rd(g_weights.w2, H2 * H1);
    rd(g_weights.b2, H2);
    rd(g_weights.w3, OUT * H2);
    rd(g_weights.b3, OUT);

    fclose(f);
    g_loaded = ok;
    return ok;
}

// ── Forward pass ───────────────────────────────────────────────────────────
float forward(const float *features) {
    // Warstwa 1: IN → H1
    float h1[H1];
    for (int i = 0; i < H1; i++) {
        float acc = g_weights.b1[i];
        const float *row = g_weights.w1[i];
        // Sparse multiply — features są głównie zerami (binary input)
        for (int j = 0; j < IN; j++) {
            if (features[j] != 0.0f) acc += row[j] * features[j];
        }
        h1[i] = relu(acc);
    }

    // Warstwa 2: H1 → H2
    float h2[H2];
    for (int i = 0; i < H2; i++) {
        float acc = g_weights.b2[i];
        const float *row = g_weights.w2[i];
        for (int j = 0; j < H1; j++) acc += row[j] * h1[j];
        h2[i] = relu(acc);
    }

    // Warstwa wyjściowa: H2 → 1 (tanh)
    float out = g_weights.b3[0];
    for (int j = 0; j < H2; j++) out += g_weights.w3[0][j] * h2[j];

    // Skaluj tanh do centypionów: ±10000
    return std::tanh(out) * 10000.0f;
}

} // namespace nnue
