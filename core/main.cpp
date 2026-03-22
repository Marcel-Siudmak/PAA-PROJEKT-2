#include "board.hpp"
#include "magic.hpp"
#include "move.hpp"
#include "types.hpp"
#include "zobrist.hpp"
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

// --- POMOCNICZE FUNKCJE WIZUALIZACJI ---

std::string sqToNotation(int sq) {
  return std::string(1, static_cast<char>('a' + (sq % 8))) +
         std::to_string((sq / 8) + 1);
}

void printBoard(const Board &board) {
  std::cout << "\n      a b c d e f g h\n";
  std::cout << "    +-----------------+\n";
  for (int r = 7; r >= 0; r--) {
    std::cout << "  " << r + 1 << " | ";
    for (int f = 0; f < 8; f++) {
      int sq = r * 8 + f;
      char pieceChar = '.';

      for (int p = 0; p < 6; p++) {
        uint64_t bit = (1ULL << sq);
        if (board.pieces[static_cast<int>(Color::WHITE)][p] & bit) {
          pieceChar = "PNBRQK"[p];
        } else if (board.pieces[static_cast<int>(Color::BLACK)][p] & bit) {
          pieceChar = "pnbrqk"[p];
        }
      }
      std::cout << pieceChar << " ";
    }
    std::cout << "| " << r + 1 << "\n";
  }
  std::cout << "    +-----------------+\n";
  std::cout << "      a b c d e f g h\n\n";
  std::cout << "Kolej: "
            << (board.sideToMove == Color::WHITE ? "BIALE (DUZE)"
                                                 : "CZARNE (male)")
            << "\n";
}

#include "../engine/search.hpp"

// Pomocnicza funkcja do zamiany Move na napis np. e2e4
std::string moveToString(const Move &m) {
  std::string promo = "";
  if (m.promotion != PieceType::NONE) {
    if (m.promotion == PieceType::QUEEN)
      promo = "q";
    else if (m.promotion == PieceType::ROOK)
      promo = "r";
    else if (m.promotion == PieceType::BISHOP)
      promo = "b";
    else if (m.promotion == PieceType::KNIGHT)
      promo = "n";
  }
  return sqToNotation(m.from) + sqToNotation(m.to) + promo;
}

// --- GŁÓWNA PĘTLA TESTOWA ---

int main() {
  srand(time(NULL));
  init_magic_tables();
  init_zobrist();

  Board board;

  int moveCount = 0;

  std::cout << "--- Prosty program szachowy ---" << std::endl;
  std::cout << "Aby wykonac ruch, wpisz zrodlo i cel w notacji np. e2e4."
            << std::endl;
  std::cout << "Promocja np. e7e8q." << std::endl;
  std::cout << "Wpisz 'bot', aby przekazac ruch do bota, lub 'quit' aby wyjsc."
            << std::endl;

  while (true) {
    printBoard(board);

    GameStatus status = board.getGameStatus();

    if (status != GameStatus::RUNNING) {
      if (status == GameStatus::CHECKMATE) {
        std::cout << "\n==============================\n";
        std::cout << "       SZACH-MAT!             \n";
        std::cout << " Wygrywaja: "
                  << (board.sideToMove == Color::WHITE ? "CZARNE" : "BIALE")
                  << "\n";
        std::cout << "==============================\n";
      } else if (status == GameStatus::STALEMATE) {
        std::cout << "\n--- PAT (REMIS) ---\n";
      } else if (status == GameStatus::DRAW_FIFTY_MOVES) {
        std::cout << "\n--- REMIS (Zasada 50 ruchow) ---\n";
      } else if (status == GameStatus::DRAW_REPETITION) {
        std::cout << "\n--- REMIS (Trzykrotne powtorzenie pozycji) ---\n";
      } else if (status == GameStatus::DRAW_INSUFFICIENT_MATERIAL) {
        std::cout << "\n--- REMIS (Martwa pozycja - brak materialu) ---\n";
      }
      break;
    }

    if (board.isInCheck(board.sideToMove)) {
      std::cout << "!!! SZACH !!!\n";
    }

    std::vector<Move> moves = board.generateLegalMoves();

    // Kto rusza w tym momencie
    if (board.sideToMove == Color::WHITE) {
      // Ruch uzytkownika
      bool validMove = false;
      while (!validMove) {
        std::cout << "Twoj ruch (Czarne/Bot wpisz 'bot'): ";
        std::string input;
        std::cin >> input;

        if (input == "quit") {
          return 0;
        }

        if (input == "bot") {
          std::cout << "Bot mysli (glebokosc 6)..." << std::endl;
          engine::SearchResult res = engine::getBestMove(board, 6);
          std::cout << "Bot ("
                    << (board.sideToMove == Color::WHITE ? "Biale" : "Czarne")
                    << ") gra: " << moveToString(res.bestMove) << "\n";
          board.makeMove(res.bestMove, board.sideToMove);
          validMove = true;
          break;
        }

        // Sprawdz czy to poprawny ruch
        bool found = false;
        for (const auto &m : moves) {
          if (moveToString(m) == input) {
            board.makeMove(m, board.sideToMove);
            found = true;
            validMove = true;
            break;
          }
        }

        if (!found) {
          std::cout << "Niepoprawny ruch lub format! Sprobuj np. e2e4"
                    << std::endl;
        }
      }
    } else {
      // Ruch Bota (Czarne)
      std::cout << "Bot mysli (glebokosc 6)..." << std::endl;
      engine::SearchResult res =
          engine::getBestMove(board, 6); // alpha-beta pozwala na glebsze szukanie
      std::cout << "Bot zagra: " << moveToString(res.bestMove)
                << " (eval: " << res.score << ")\n";
      board.makeMove(res.bestMove, board.sideToMove);
    }

    moveCount++;
  }

  return 0;
}