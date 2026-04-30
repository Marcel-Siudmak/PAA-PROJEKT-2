/*
 * selfplay_bridge.cpp — generator danych treningowych do NNUE
 *
 * Rozgrywa partie bot vs bot i zapisuje pozycje jako FEN + wynik
 * do pliku wyjściowego (domyślnie: games.txt)
 *
 * Format wyjścia (jeden rekord na linię):
 *   <FEN> | <wynik>
 * gdzie wynik: 1=białe wygrały, -1=czarne wygrały, 0=remis
 *
 * Użycie:
 *   ./selfplay_gen [--games N] [--depth D] [--output plik.txt]
 */

#include "board.hpp"
#include "magic.hpp"
#include "move.hpp"
#include "search.hpp"
#include "types.hpp"
#include "zobrist.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

// ── FEN encoder ────────────────────────────────────────────────────────────
// Pełna konwersja Board → FEN string
// (sideToMove, castling i en-passant uproszczone — wystarczające do treningu)

static std::string boardToFen(const Board &b) {
    static const char PIECE_CHARS[] = "PNBRQKpnbrqk";
    std::string fen;
    fen.reserve(80);

    for (int rank = 7; rank >= 0; rank--) {
        int empty = 0;
        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            int8_t mb = b.mailbox[sq];
            if (mb < 0) {
                empty++;
            } else {
                if (empty) { fen += char('0' + empty); empty = 0; }
                int color = mb / 8;
                int ptype = mb % 8;
                fen += PIECE_CHARS[color * 6 + ptype];
            }
        }
        if (empty) fen += char('0' + empty);
        if (rank > 0) fen += '/';
    }

    fen += ' ';
    fen += (b.sideToMove == Color::WHITE) ? 'w' : 'b';

    // Roszady
    fen += ' ';
    if (b.castlingRights == 0) {
        fen += '-';
    } else {
        if (b.castlingRights & WHITE_OO)  fen += 'K';
        if (b.castlingRights & WHITE_OOO) fen += 'Q';
        if (b.castlingRights & BLACK_OO)  fen += 'k';
        if (b.castlingRights & BLACK_OOO) fen += 'q';
    }

    // En-passant (uproszczone — pomijamy dla treningu)
    fen += " - 0 1";
    return fen;
}

// ── Self-play jedna partia ──────────────────────────────────────────────────
// Zwraca: +1 = białe wygrały, -1 = czarne, 0 = remis
// fens_out: wektor par (FEN, sideToMove) pozycji podczas partii

static int playGame(int depth, std::vector<std::pair<std::string,int>> &fens_out) {
    Board board;
    fens_out.clear();

    for (int move_nr = 0; move_nr < 200; move_nr++) {
        GameStatus st = board.getGameStatus();
        if (st != GameStatus::RUNNING) {
            if (st == GameStatus::CHECKMATE)
                return (board.sideToMove == Color::WHITE) ? -1 : 1;
            return 0; // remis
        }

        // Zapisz pozycję (ale nie pierwsze 4 ruchy — zbyt zbliżone do startu)
        if (move_nr >= 4) {
            fens_out.push_back({
                boardToFen(board),
                (board.sideToMove == Color::WHITE) ? 1 : -1
            });
        }

        auto res = engine::getBestMove(board, depth);
        if (res.bestMove.from < 0) break;
        board.makeMove(res.bestMove, board.sideToMove);
    }

    return 0; // przekroczono limit ruchów → remis
}

// ── Main ────────────────────────────────────────────────────────────────────

int main(int argc, char **argv) {
    srand(static_cast<unsigned>(time(nullptr)));
    init_magic_tables();
    init_zobrist();

    int         n_games    = 200;
    int         depth      = 4;
    const char *output_path = "ml/games.txt";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--games")  == 0 && i + 1 < argc) n_games    = atoi(argv[++i]);
        if (strcmp(argv[i], "--depth")  == 0 && i + 1 < argc) depth      = atoi(argv[++i]);
        if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) output_path = argv[++i];
    }

    FILE *out = fopen(output_path, "w");
    if (!out) {
        fprintf(stderr, "Nie można otworzyć pliku: %s\n", output_path);
        return 1;
    }

    long long total_positions = 0;
    int w = 0, d = 0, bl = 0;

    printf("selfplay_gen: %d gier, głębokość %d → %s\n", n_games, depth, output_path);
    fflush(stdout);

    for (int g = 0; g < n_games; g++) {
        std::vector<std::pair<std::string,int>> fens;
        int result = playGame(depth, fens);

        if (result > 0) w++;
        else if (result < 0) bl++;
        else d++;

        // Zapisz każdą pozycję z wynikiem gry
        for (const auto &[fen, side] : fens) {
            // Wynik z perspektywy strony toczącej ruch
            // result: +1=białe wygrały; jeśli ruch czarnych i białe wygrały → -1
            int rel_result = result * side;
            fprintf(out, "%s | %d\n", fen.c_str(), rel_result);
        }
        total_positions += (long long)fens.size();

        if ((g + 1) % 10 == 0 || g == n_games - 1) {
            printf("Gra %d/%d | W:%d D:%d L:%d | Pozycji: %lld\n",
                   g + 1, n_games, w, d, bl, total_positions);
            fflush(stdout);
        }
    }

    fclose(out);
    printf("\nGotowe! Zapisano %lld pozycji do %s\n", total_positions, output_path);
    return 0;
}
