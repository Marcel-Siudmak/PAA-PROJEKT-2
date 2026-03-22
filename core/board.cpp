#include "board.hpp"
#include "zobrist.hpp"

uint64_t Board::calculateZobristHash() const {
  uint64_t h = 0ULL;
  for (int c = 0; c < 2; c++) {
    for (int p = 0; p < 6; p++) {
      uint64_t bb = pieces[c][p];
      while (bb) {
        int sq = __builtin_ctzll(bb);
        h ^= ZOBRIST_PIECES[c][p][sq];
        bb &= (bb - 1);
      }
    }
  }
  h ^= ZOBRIST_CASTLING[castlingRights];
  if (enPassantSquare) {
    h ^= ZOBRIST_EP[__builtin_ctzll(enPassantSquare)];
  }
  if (sideToMove == Color::BLACK) {
    h ^= ZOBRIST_SIDE;
  }
  return h;
}

bool Board::isRepetition() const {
  int count = 1;
  int limit = std::min((int)history.size(), (int)halfMoveClock);
  for (int i = 0; i < limit; i++) {
    if (history[history.size() - 1 - i].zobristHash == zobristHash) {
      count++;
      if (count >= 3)
        return true;
    }
  }
  return false;
}

bool Board::isInsufficientMaterial() const {
  int wp = __builtin_popcountll(pieces[static_cast<int>(Color::WHITE)]
                                      [static_cast<int>(PieceType::PAWN)]);
  int bp = __builtin_popcountll(pieces[static_cast<int>(Color::BLACK)]
                                      [static_cast<int>(PieceType::PAWN)]);
  int wr = __builtin_popcountll(pieces[static_cast<int>(Color::WHITE)]
                                      [static_cast<int>(PieceType::ROOK)]);
  int br = __builtin_popcountll(pieces[static_cast<int>(Color::BLACK)]
                                      [static_cast<int>(PieceType::ROOK)]);
  int wq = __builtin_popcountll(pieces[static_cast<int>(Color::WHITE)]
                                      [static_cast<int>(PieceType::QUEEN)]);
  int bq = __builtin_popcountll(pieces[static_cast<int>(Color::BLACK)]
                                      [static_cast<int>(PieceType::QUEEN)]);

  if (wp > 0 || bp > 0 || wr > 0 || br > 0 || wq > 0 || bq > 0)
    return false;

  int wb = __builtin_popcountll(pieces[static_cast<int>(Color::WHITE)]
                                      [static_cast<int>(PieceType::BISHOP)]);
  int bb = __builtin_popcountll(pieces[static_cast<int>(Color::BLACK)]
                                      [static_cast<int>(PieceType::BISHOP)]);
  int wn = __builtin_popcountll(pieces[static_cast<int>(Color::WHITE)]
                                      [static_cast<int>(PieceType::KNIGHT)]);
  int bn = __builtin_popcountll(pieces[static_cast<int>(Color::BLACK)]
                                      [static_cast<int>(PieceType::KNIGHT)]);

  int wMinor = wb + wn;
  int bMinor = bb + bn;

  if (wMinor == 0 && bMinor == 0)
    return true;
  if ((wMinor == 1 && bMinor == 0) || (wMinor == 0 && bMinor == 1))
    return true;

  if (wn == 0 && bn == 0 && wb == 1 && bb == 1) {
    int wsq = __builtin_ctzll(pieces[static_cast<int>(Color::WHITE)]
                                    [static_cast<int>(PieceType::BISHOP)]);
    int bsq = __builtin_ctzll(pieces[static_cast<int>(Color::BLACK)]
                                    [static_cast<int>(PieceType::BISHOP)]);
    int wcol = (wsq / 8 + wsq % 8) % 2;
    int bcol = (bsq / 8 + bsq % 8) % 2;
    if (wcol == bcol)
      return true;
  }

  return false;
}

GameStatus Board::getGameStatus() {
  auto legalMoves = generateLegalMoves();

  if (legalMoves.empty()) {
    if (isInCheck(sideToMove)) {
      return GameStatus::CHECKMATE;
    } else {
      return GameStatus::STALEMATE;
    }
  }

  if (isInsufficientMaterial())
    return GameStatus::DRAW_INSUFFICIENT_MATERIAL;
  if (halfMoveClock >= 100)
    return GameStatus::DRAW_FIFTY_MOVES;
  if (isRepetition())
    return GameStatus::DRAW_REPETITION;

  return GameStatus::RUNNING;
}

uint64_t Board::getColorOccupied(Color color) const {
  uint64_t occupied = 0ULL;
  for (int pieceType = 0; pieceType < 6; ++pieceType)
    occupied |= pieces[static_cast<int>(color)][pieceType];
  return occupied;
}

uint64_t Board::getAllOccupied() const {
  return getColorOccupied(Color::WHITE) | getColorOccupied(Color::BLACK);
}

bool Board::isInCheck(Color side) const {
  // 1. Znajdź bitboard króla dla danego koloru
  uint64_t kingBB =
      pieces[static_cast<int>(side)][static_cast<int>(PieceType::KING)];

  // 2. Jeśli z jakiegoś powodu nie ma króla (np. błąd inicjalizacji), zwróć
  // false
  if (!kingBB)
    return false;

  // 3. Pobierz indeks pola króla (0-63)
  int kingSq = __builtin_ctzll(kingBB);

  // 4. Sprawdź, czy to pole jest atakowane przez przeciwnika
  return isSquareAttacked(kingSq, oppositeColor(side));
}

static const uint8_t CASTLING_MASK[64] =
    {
        13, 15, 15, 15, 12, 15, 15, 14, // a1 to h1
        15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 7,  15, 15, 15, 3,  15, 15, 11 // a8 to h8
};

void Board::makeMove(const Move &m, Color side) {
  history.push_back(
      {castlingRights, enPassantSquare, halfMoveClock, zobristHash});
  Color opp = oppositeColor(side);
  int sideIdx = static_cast<int>(side);
  int oppIdx = static_cast<int>(opp);

  if (m.piece == PieceType::PAWN || m.captured != PieceType::NONE) {
    halfMoveClock = 0;
  } else {
    halfMoveClock++;
  }

  enPassantSquare = 0ULL;

  // 1. Obsługa bić
  if (m.type == EN_PASSANT) {
    int capSq = (side == Color::WHITE) ? m.to - 8 : m.to + 8;
    pieces[oppIdx][static_cast<int>(PieceType::PAWN)] ^= (1ULL << capSq);
    mailbox[capSq] = -1;
  } else if (m.captured != PieceType::NONE) {
    pieces[oppIdx][static_cast<int>(m.captured)] ^= (1ULL << m.to);
    // mailbox[m.to] zostanie nadpisany poniżej
  }

  // 2. Ruch figury (bitboard)
  pieces[sideIdx][static_cast<int>(m.piece)] ^= (1ULL << m.from);
  PieceType finalPiece = (m.type == PROMOTION) ? m.promotion : m.piece;
  pieces[sideIdx][static_cast<int>(finalPiece)] ^= (1ULL << m.to);

  // 2b. Aktualizacja mailbox
  mailbox[m.from] = -1;
  mailbox[m.to] = (int8_t)(sideIdx * 8 + static_cast<int>(finalPiece));

  // 3. Roszada
  if (m.type == CASTLING) {
    int rFrom = (m.to > m.from) ? m.from + 3 : m.from - 4;
    int rTo = (m.to > m.from) ? m.from + 1 : m.from - 1;
    pieces[sideIdx][static_cast<int>(PieceType::ROOK)] ^=
        (1ULL << rFrom) | (1ULL << rTo);
    mailbox[rFrom] = -1;
    mailbox[rTo] = (int8_t)(sideIdx * 8 + static_cast<int>(PieceType::ROOK));
  }

  if (m.piece == PieceType::PAWN && std::abs(m.to - m.from) == 16) {
    enPassantSquare = (1ULL << (m.from + (m.to - m.from) / 2));
  }

  castlingRights &= CASTLING_MASK[m.from] & CASTLING_MASK[m.to];

  sideToMove = opp;
  zobristHash = calculateZobristHash();
}

void Board::unmakeMove(const Move &m, Color side) {
  if (history.empty())
    return;

  castlingRights = history.back().castlingRights;
  enPassantSquare = history.back().enPassantSquare;
  halfMoveClock = history.back().halfMoveClock;
  zobristHash = history.back().zobristHash;
  history.pop_back();
  sideToMove = side;

  Color opp = oppositeColor(side);
  int sideIdx = static_cast<int>(side);
  int oppIdx = static_cast<int>(opp);

  // 1. Cofnij ruch figury (bitboard)
  PieceType finalPiece = (m.type == PROMOTION) ? m.promotion : m.piece;
  pieces[sideIdx][static_cast<int>(finalPiece)] ^= (1ULL << m.to);
  pieces[sideIdx][static_cast<int>(m.piece)] ^= (1ULL << m.from);

  // 1b. Mailbox
  mailbox[m.to] = -1;
  mailbox[m.from] = (int8_t)(sideIdx * 8 + static_cast<int>(m.piece));

  // 2. Cofnij bicia
  if (m.type == EN_PASSANT) {
    int capSq = (side == Color::WHITE) ? m.to - 8 : m.to + 8;
    pieces[oppIdx][static_cast<int>(PieceType::PAWN)] ^= (1ULL << capSq);
    mailbox[capSq] = (int8_t)(oppIdx * 8 + static_cast<int>(PieceType::PAWN));
  } else if (m.captured != PieceType::NONE) {
    pieces[oppIdx][static_cast<int>(m.captured)] ^= (1ULL << m.to);
    mailbox[m.to] = (int8_t)(oppIdx * 8 + static_cast<int>(m.captured));
  }

  // 3. Cofnij roszadę
  if (m.type == CASTLING) {
    int rFrom = (m.to > m.from) ? m.from + 3 : m.from - 4;
    int rTo = (m.to > m.from) ? m.from + 1 : m.from - 1;
    pieces[sideIdx][static_cast<int>(PieceType::ROOK)] ^=
        (1ULL << rFrom) | (1ULL << rTo);
    mailbox[rTo] = -1;
    mailbox[rFrom] = (int8_t)(sideIdx * 8 + static_cast<int>(PieceType::ROOK));
  }
}

uint64_t Board::perft(int depth) {
  if (depth == 0)
    return 1;

  std::vector<Move> moves = generateLegalMoves(sideToMove);
  uint64_t nodes = 0;

  for (const auto &m : moves) {
    makeMove(m, sideToMove);
    nodes += perft(depth - 1);
    unmakeMove(m, oppositeColor(sideToMove));
  }

  return nodes;
}