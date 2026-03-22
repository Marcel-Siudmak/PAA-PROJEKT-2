#include "board.hpp"



GameStatus Board::getGameStatus() {
    auto legalMoves = generateLegalMoves(); // To używa sideToMove

    if (legalMoves.empty()) {
        if (isInCheck(sideToMove)) {
            return GameStatus::CHECKMATE;
        } else {
            return GameStatus::STALEMATE;
        }
    }

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
    uint64_t kingBB = pieces[static_cast<int>(side)][static_cast<int>(PieceType::KING)];
    
    // 2. Jeśli z jakiegoś powodu nie ma króla (np. błąd inicjalizacji), zwróć false
    if (!kingBB) return false;
    
    // 3. Pobierz indeks pola króla (0-63)
    int kingSq = __builtin_ctzll(kingBB);
    
    // 4. Sprawdź, czy to pole jest atakowane przez przeciwnika
    return isSquareAttacked(kingSq, oppositeColor(side));
}

static const uint8_t CASTLING_MASK[64] = {
    13, 15, 15, 15, 12, 15, 15, 14, // a1 to h1
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    7,  15, 15, 15, 3,  15, 15, 11  // a8 to h8
};

void Board::makeMove(const Move& m, Color side) {
    history.push_back({castlingRights, enPassantSquare});
    Color opp = oppositeColor(side);
    
    // Resetuj EP, chyba że to skok pionka o 2
    enPassantSquare = 0ULL;

    // 1. Obsługa bić
    if (m.type == EN_PASSANT) {
        int capSq = (side == Color::WHITE) ? m.to - 8 : m.to + 8;
        pieces[static_cast<int>(opp)][static_cast<int>(PieceType::PAWN)] ^= (1ULL << capSq);
    } else if (m.captured != PieceType::NONE) {
        pieces[static_cast<int>(opp)][static_cast<int>(m.captured)] ^= (1ULL << m.to);
    }

    // 2. Ruch figury
    pieces[static_cast<int>(side)][static_cast<int>(m.piece)] ^= (1ULL << m.from);
    PieceType finalPiece = (m.type == PROMOTION) ? m.promotion : m.piece;
    pieces[static_cast<int>(side)][static_cast<int>(finalPiece)] ^= (1ULL << m.to);

    // 3. Roszada
    if (m.type == CASTLING) {
        int rFrom = (m.to > m.from) ? m.from + 3 : m.from - 4;
        int rTo = (m.to > m.from) ? m.from + 1 : m.from - 1;
        pieces[static_cast<int>(side)][static_cast<int>(PieceType::ROOK)] ^= (1ULL << rFrom) | (1ULL << rTo);
    }

    // Ustawienie EP dla następnego ruchu
    if (m.piece == PieceType::PAWN && std::abs(m.to - m.from) == 16) {
        enPassantSquare = (1ULL << (m.from + (m.to - m.from) / 2));
    }

    castlingRights &= CASTLING_MASK[m.from] & CASTLING_MASK[m.to];

    sideToMove = opp;
}

void Board::unmakeMove(const Move& m, Color side) {
    if (history.empty()) return;
    
    // Przywróć stan
    castlingRights = history.back().castlingRights;
    enPassantSquare = history.back().enPassantSquare;
    history.pop_back();
    sideToMove = side;

    Color opp = oppositeColor(side);

    // 1. Cofnij ruch figury
    PieceType finalPiece = (m.type == PROMOTION) ? m.promotion : m.piece;
    pieces[static_cast<int>(side)][static_cast<int>(finalPiece)] ^= (1ULL << m.to);
    pieces[static_cast<int>(side)][static_cast<int>(m.piece)] ^= (1ULL << m.from);

    // 2. Cofnij bicia
    if (m.type == EN_PASSANT) {
        int capSq = (side == Color::WHITE) ? m.to - 8 : m.to + 8;
        pieces[static_cast<int>(opp)][static_cast<int>(PieceType::PAWN)] ^= (1ULL << capSq);
    } else if (m.captured != PieceType::NONE) {
        pieces[static_cast<int>(opp)][static_cast<int>(m.captured)] ^= (1ULL << m.to);
    }

    // 3. Cofnij roszadę
    if (m.type == CASTLING) {
        int rFrom = (m.to > m.from) ? m.from + 3 : m.from - 4;
        int rTo = (m.to > m.from) ? m.from + 1 : m.from - 1;
        pieces[static_cast<int>(side)][static_cast<int>(PieceType::ROOK)] ^= (1ULL << rFrom) | (1ULL << rTo);
    }
}

uint64_t Board::perft(int depth) {
    if (depth == 0) return 1;

    std::vector<Move> moves = generateLegalMoves(sideToMove);
    uint64_t nodes = 0;

    for (const auto& m : moves) {
        makeMove(m, sideToMove);
        nodes += perft(depth - 1);
        unmakeMove(m, oppositeColor(sideToMove));
    }

    return nodes;
}