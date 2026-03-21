#include <iostream>
#include <vector>
#include <random>
#include <cstdint>
#include <cstring>
#include "magic.hpp"

using namespace std;

typedef uint64_t Bitboard;

// Liczba bitów w maskach dla każdego pola (standardowe wartości)
int rook_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12, 11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11, 12, 11, 11, 11, 11, 11, 11, 12
};

int bishop_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5, 5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5, 5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 6
};

// --- Funkcje pomocnicze do losowania ---
mt19937_64 rng(123456789); // Stałe ziarno dla powtarzalności

Bitboard random_bitboard() {
    return (Bitboard)rng();
}

Bitboard random_bitboard_sparse() {
    // Generuje liczbę z małą ilością bitów (idealną na kandydata do magii)
    return random_bitboard() & random_bitboard() & random_bitboard();
}

// --- Funkcje z Twojego poprzedniego etapu (muszą tu być do testów) ---
static Bitboard set_occupancy(int index, int bits_in_mask, Bitboard mask) {
    Bitboard occ = 0ULL;
    for (int i = 0; i < bits_in_mask; i++) {
        int square = __builtin_ctzll(mask);
        mask &= (mask - 1);
        if (index & (1 << i)) occ |= (1ULL << square);
    }
    return occ;
}

// Tutaj musisz mieć też te funkcje "on_the_fly" (Ray-casting), które napisałeś wcześniej
// Służą one jako "wyrocznia" do sprawdzenia, czy magiczna liczba działa poprawnie.

// --- Serce generatora ---
Bitboard find_magic(int sq, int m_bits, bool is_bishop) {
    Bitboard occupancies[4096], attacks[4096], used[4096];
    Bitboard mask = is_bishop ? create_bishop_mask(sq) : create_rook_mask(sq);
    int num_indices = 1 << m_bits;

    // 1. Przygotuj wszystkie możliwe blokady i odpowiadające im ataki
    for (int i = 0; i < num_indices; i++) {
        occupancies[i] = set_occupancy(i, m_bits, mask);
        attacks[i] = is_bishop ? bishop_attacks_on_the_fly(sq, occupancies[i]) 
                               : rook_attacks_on_the_fly(sq, occupancies[i]);
    }

    // 2. Szukaj liczby, która nie powoduje kolizji
    for (int k = 0; k < 100000000; k++) {
        Bitboard magic = random_bitboard_sparse();
        
        // Szybki test: czy góra iloczynu ma dość bitów?
        if (__builtin_popcountll((mask * magic) & 0xFF00000000000000ULL) < 6) continue;

        memset(used, 0, sizeof(used));
        bool fail = false;

        for (int i = 0; i < num_indices; i++) {
            int idx = (int)((occupancies[i] * magic) >> (64 - m_bits));
            if (used[idx] == 0) {
                used[idx] = attacks[i]; // Pierwszy raz widzimy ten indeks - zapisujemy wynik
            } else if (used[idx] != attacks[i]) {
                fail = true; // KOLIZJA! Ten sam indeks dla różnych ataków
                break;
            }
        }
        if (!fail) return magic;
    }
    return 0; // Jeśli nie znaleziono (bardzo rzadkie)
}

int main() {
    cout << "Generowanie magicznych liczb dla Gońców..." << endl;
    cout << "uint64_t BishopMagics[64] = {" << endl;
    for (int i = 0; i < 64; i++) {
        cout << "  0x" << hex << find_magic(i, bishop_bits[i], true) << "ULL," << endl;
    }
    cout << "};" << endl << endl;

    cout << "Generowanie magicznych liczb dla Wież..." << endl;
    cout << "uint64_t RookMagics[64] = {" << endl;
    for (int i = 0; i < 64; i++) {
        cout << "  0x" << hex << find_magic(i, rook_bits[i], false) << "ULL," << endl;
    }
    cout << "};" << endl;

    return 0;
}