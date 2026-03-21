#include "magic.hpp"
#include <iostream>

// --- 1. Wielkie tablice na gotowe ataki (ok. 2.5 MB) ---
static uint64_t RookTable[0x19000];   
static uint64_t BishopTable[0x1480];

Magic RookMagics[64];
Magic BishopMagics[64];

// --- 2. Twoje wygenerowane liczby magiczne ---
const uint64_t RookMagicNumbers[64] = {
    0x180001826400080ULL, 0x8440004010042000ULL, 0x3480088010012000ULL, 0x200102042000804ULL,
    0x1600200200041108ULL, 0x200010490084200ULL, 0x8080008002000100ULL, 0x20008210044820cULL,
    0x2320800840062080ULL, 0x1008804004802000ULL, 0x1420801000200080ULL, 0x108a000842002010ULL,
    0x1101000800100502ULL, 0x1096001884102200ULL, 0x411000100220004ULL, 0x1000802251800900ULL,
    0x80004000200050ULL, 0x2050004040002008ULL, 0x810022001240800ULL, 0x2001010010000c21ULL,
    0x203050008011100ULL, 0x4808004000200ULL, 0x8900040002080110ULL, 0x5205a0011a04304ULL,
    0x40008020800aULL, 0x80200080804000ULL, 0x100200100104100ULL, 0x420080480100181ULL,
    0x11009100180114ULL, 0x6104000202001008ULL, 0x500a50026002cULL, 0x801880004100ULL,
    0x400081c002800230ULL, 0x5001201000c00042ULL, 0x1c62008612002040ULL, 0x100080800800ULL,
    0x808020040c01400ULL, 0x4410020080800400ULL, 0x800020804000110ULL, 0x801040801100ULL,
    0x80204000908002ULL, 0x2220201000414008ULL, 0x2050108200420020ULL, 0x2000100101090020ULL,
    0x4088020004004040ULL, 0xd4040002008080ULL, 0x2181001200050004ULL, 0xa80a804400a20001ULL,
    0x524c210242920200ULL, 0x408200802a4b0200ULL, 0x202102001094100ULL, 0x4100008210100ULL,
    0x1000040080080080ULL, 0x102000204008080ULL, 0x1009000600140d00ULL, 0x4100842200ULL,
    0x18010020c2118001ULL, 0x2042221284400101ULL, 0x2002084100842ULL, 0x83842009001001ULL,
    0x2003008208001005ULL, 0x2000801441082ULL, 0x102000104080082ULL, 0x2130080440103a6ULL
};

const uint64_t BishopMagicNumbers[64] = {
    0x4c81004202040ULL, 0x60240120470080ULL, 0x19142c00800016ULL, 0x8060042100400ULL,
    0x6001104012064101ULL, 0x2080404000500ULL, 0x1164040c409206ULL, 0x404220202200240ULL,
    0x21041002320401ULL, 0x1014102200d90601ULL, 0x4082830448102ULL, 0x5002840410890809ULL,
    0x4942420100c2aULL, 0x400430198400000ULL, 0x1a0002a088205000ULL, 0x190001844c100500ULL,
    0xe880040b0048880ULL, 0x4870440202084100ULL, 0x4004214061200ULL, 0x8020082044003ULL,
    0x400121022000aULL, 0x10880010080c020ULL, 0x104120880c41000ULL, 0x101020080411000ULL,
    0x48c11020140900ULL, 0x5004048010014828ULL, 0x80220004140400ULL, 0x30040000401021ULL,
    0x410101011004012ULL, 0x12220044480201ULL, 0xa00a10810200d000ULL, 0x81052400a010400ULL,
    0x1604024002200408ULL, 0x8004100408080104ULL, 0x4000140400020801ULL, 0x1208040400080210ULL,
    0x28020400011010ULL, 0x122501070002014eULL, 0x62420202000400a4ULL, 0x208084008a004200ULL,
    0x202012120000840ULL, 0x4414820820801ULL, 0x80000a2804000800ULL, 0x4201a2000400ULL,
    0x210202008420ULL, 0x8202020421000809ULL, 0x4018084802448282ULL, 0x58100080a1002080ULL,
    0x2650420204002ULL, 0x8520084204080ULL, 0x10401040101ULL, 0x1903000084040602ULL,
    0x34504008222000ULL, 0x41200510108844ULL, 0xc1840122020001ULL, 0x80300e0089120008ULL,
    0x4000840088240240ULL, 0x8011008208118400ULL, 0x8502441c0442ULL, 0x8220000000208806ULL,
    0x112208014304c08ULL, 0x808100950010200ULL, 0x4004041104080080ULL, 0x148194808015080ULL
};

// --- 3. Rozmiary masek (muszą być takie same jak w generatorze) ---
static const int rook_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12, 11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11, 12, 11, 11, 11, 11, 11, 11, 12
};

static const int bishop_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5, 5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5, 5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 6
};

// --- 4. Funkcje generujące maski i ataki "on the fly" ---
uint64_t create_rook_mask(int sq) {
    uint64_t mask = 0ULL;
    int r = sq / 8, f = sq % 8;
    for (int i = r + 1; i <= 6; i++) mask |= (1ULL << (i * 8 + f));
    for (int i = r - 1; i >= 1; i--) mask |= (1ULL << (i * 8 + f));
    for (int i = f + 1; i <= 6; i++) mask |= (1ULL << (r * 8 + i));
    for (int i = f - 1; i >= 1; i--) mask |= (1ULL << (r * 8 + i));
    return mask;
}

uint64_t create_bishop_mask(int sq) {
    uint64_t mask = 0ULL;
    int r = sq / 8, f = sq % 8;
    for (int i = r + 1, j = f + 1; i <= 6 && j <= 6; i++, j++) mask |= (1ULL << (i * 8 + j));
    for (int i = r + 1, j = f - 1; i <= 6 && j >= 1; i++, j--) mask |= (1ULL << (i * 8 + j));
    for (int i = r - 1, j = f + 1; i >= 1 && j <= 6; i--, j++) mask |= (1ULL << (i * 8 + j));
    for (int i = r - 1, j = f - 1; i >= 1 && j >= 1; i--, j--) mask |= (1ULL << (i * 8 + j));
    return mask;
}

uint64_t rook_attacks_on_the_fly(int sq, uint64_t occ) {
    uint64_t attacks = 0ULL;
    int r = sq / 8, f = sq % 8;
    for (int i = r + 1; i <= 7; i++) { attacks |= (1ULL << (i * 8 + f)); if (occ & (1ULL << (i * 8 + f))) break; }
    for (int i = r - 1; i >= 0; i--) { attacks |= (1ULL << (i * 8 + f)); if (occ & (1ULL << (i * 8 + f))) break; }
    for (int i = f + 1; i <= 7; i++) { attacks |= (1ULL << (r * 8 + i)); if (occ & (1ULL << (r * 8 + i))) break; }
    for (int i = f - 1; i >= 0; i--) { attacks |= (1ULL << (r * 8 + i)); if (occ & (1ULL << (r * 8 + i))) break; }
    return attacks;
}

uint64_t bishop_attacks_on_the_fly(int sq, uint64_t occ) {
    uint64_t attacks = 0ULL;
    int r = sq / 8, f = sq % 8;
    for (int i = r + 1, j = f + 1; i <= 7 && j <= 7; i++, j++) { attacks |= (1ULL << (i * 8 + j)); if (occ & (1ULL << (i * 8 + j))) break; }
    for (int i = r + 1, j = f - 1; i <= 7 && j >= 0; i++, j--) { attacks |= (1ULL << (i * 8 + j)); if (occ & (1ULL << (i * 8 + j))) break; }
    for (int i = r - 1, j = f + 1; i >= 0 && j <= 7; i--, j++) { attacks |= (1ULL << (i * 8 + j)); if (occ & (1ULL << (i * 8 + j))) break; }
    for (int i = r - 1, j = f - 1; i >= 0 && j >= 0; i--, j--) { attacks |= (1ULL << (i * 8 + j)); if (occ & (1ULL << (i * 8 + j))) break; }
    return attacks;
}

// Pomocnicza funkcja set_occupancy (też potrzebna do inicjalizacji)
uint64_t set_occupancy(int index, int bits_in_mask, uint64_t mask) {
    uint64_t occ = 0ULL;
    for (int i = 0; i < bits_in_mask; i++) {
        int square = __builtin_ctzll(mask);
        mask &= (mask - 1);
        if (index & (1 << i)) occ |= (1ULL << square);
    }
    return occ;
}

// --- 5. Funkcje dostępu (API) ---
uint64_t get_rook_attacks(int sq, uint64_t occ) {
    occ &= RookMagics[sq].mask;
    occ *= RookMagics[sq].magic;
    occ >>= RookMagics[sq].shift;
    return RookMagics[sq].attacks[occ];
}

uint64_t get_bishop_attacks(int sq, uint64_t occ) {
    occ &= BishopMagics[sq].mask;
    occ *= BishopMagics[sq].magic;
    occ >>= BishopMagics[sq].shift;
    return BishopMagics[sq].attacks[occ];
}

// --- 6. GŁÓWNA INICJALIZACJA ---
void init_magic_tables() {
    int rook_offset = 0;
    int bishop_offset = 0;

    for (int sq = 0; sq < 64; sq++) {
        // Wieże
        RookMagics[sq].mask = create_rook_mask(sq);
        RookMagics[sq].magic = RookMagicNumbers[sq];
        RookMagics[sq].shift = 64 - rook_bits[sq];
        RookMagics[sq].attacks = &RookTable[rook_offset];

        int r_indices = 1 << rook_bits[sq];
        for (int i = 0; i < r_indices; i++) {
            uint64_t occ = set_occupancy(i, rook_bits[sq], RookMagics[sq].mask);
            int idx = (int)((occ * RookMagics[sq].magic) >> RookMagics[sq].shift);
            RookMagics[sq].attacks[idx] = rook_attacks_on_the_fly(sq, occ);
        }
        rook_offset += r_indices;

        // Gońce
        BishopMagics[sq].mask = create_bishop_mask(sq);
        BishopMagics[sq].magic = BishopMagicNumbers[sq];
        BishopMagics[sq].shift = 64 - bishop_bits[sq];
        BishopMagics[sq].attacks = &BishopTable[bishop_offset];

        int b_indices = 1 << bishop_bits[sq];
        for (int i = 0; i < b_indices; i++) {
            uint64_t occ = set_occupancy(i, bishop_bits[sq], BishopMagics[sq].mask);
            int idx = (int)((occ * BishopMagics[sq].magic) >> BishopMagics[sq].shift);
            BishopMagics[sq].attacks[idx] = bishop_attacks_on_the_fly(sq, occ);
        }
        bishop_offset += b_indices;
    }
}