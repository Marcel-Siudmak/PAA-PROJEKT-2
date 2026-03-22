#include "board.hpp"
#include "magic.hpp"

uint64_t Board::getPawnSinglePushes(Color color) const {
    uint64_t pawns = pieces[static_cast<int>(color)][static_cast<int>(PieceType::PAWN)];
    uint64_t emptySquares = ~getAllOccupied();

    if (color == Color::WHITE) {
        // Białe piony przesuwają się w górę (bit przesunięty o 8 w lewo)
        return ((pawns << 8) & emptySquares);
    } else {
        // Czarne piony przesuwają się w dół (bit przesunięty o 8 w prawo)
        return ((pawns >> 8) & emptySquares);
    }
}

uint64_t Board::getPawnDoublePushes(Color color) const {
    uint64_t singlePushes = getPawnSinglePushes(color);
    uint64_t emptySquares = ~getAllOccupied();


    if (color == Color::WHITE) {
        // Białe piony mogą wykonać podwójny ruch, jeśli są na 2. linii (bit przesunięty o 16 w lewo)
        return ((singlePushes << 8) & emptySquares & RANK_4_MASK);
    } else {
        // Czarne piony mogą wykonać podwójny ruch, jeśli są na 7. linii (bit przesunięty o 16 w prawo)
        return ((singlePushes >> 8) & emptySquares & RANK_5_MASK);
    }
}

uint64_t Board::getPawnAttacks(Color color) const {
    uint64_t pawns = pieces[static_cast<int>(color)][static_cast<int>(PieceType::PAWN)];
    uint64_t opponentPieces = getColorOccupied(color == Color::WHITE ? Color::BLACK : Color::WHITE);



    if (color == Color::WHITE) {
    // Atak w stronę linii A (bit << 7) - usuwamy to, co zawinęło się na linię H
    uint64_t attackLeft = (pawns << 7) & ~FILE_H; 
    // Atak w stronę linii H (bit << 9) - usuwamy to, co zawinęło się na linię A
    uint64_t attackRight = (pawns << 9) & ~FILE_A;
    return (attackLeft | attackRight) & opponentPieces;
    } else {
        // Atak w stronę linii H (bit >> 7) - usuwamy to, co zawinęło się na linię A
        uint64_t attackRight = (pawns >> 7) & ~FILE_A;
        // Atak w stronę linii A (bit >> 9) - usuwamy to, co zawinęło się na linię H
        uint64_t attackLeft = (pawns >> 9) & ~FILE_H;
        return (attackLeft | attackRight) & opponentPieces;
    }
}

uint64_t Board::getPawnAttacks(Color color, uint64_t pawnBitboard) const {
    uint64_t attacks = 0ULL;
    if (color == Color::WHITE) {
        attacks |= (pawnBitboard << 7) & 0x7f7f7f7f7f7f7f7fULL;
        attacks |= (pawnBitboard << 9) & 0xfefefefefefefefeULL;
    } else {
        attacks |= (pawnBitboard >> 7) & 0xfefefefefefefefeULL;
        attacks |= (pawnBitboard >> 9) & 0x7f7f7f7f7f7f7f7fULL;
    }
    return attacks;
}

uint64_t Board::getKnightAttacks(uint64_t knights) const {
    // Ruchy skoczka: 8 możliwych kierunków
    uint64_t attacks = 0ULL;

    // 2 w górę + 1 w prawo
    attacks |= (knights << 17) & ~FILE_A;
    // 2 w górę + 1 w lewo
    attacks |= (knights << 15) & ~FILE_H;
    // 2 w dół + 1 w prawo
    attacks |= (knights >> 15) & ~FILE_A;
    // 2 w dół + 1 w lewo
    attacks |= (knights >> 17) & ~FILE_H;
    // 1 w górę + 2 w prawo
    attacks |= (knights << 10) & ~FILE_AB;
    // 1 w górę + 2 w lewo
    attacks |= (knights << 6) & ~FILE_GH;
    // 1 w dół + 2 w prawo
    attacks |= (knights >> 6) & ~FILE_AB;
    // 1 w dół + 2 w lewo
    attacks |= (knights >> 10) & ~FILE_GH;

    return attacks;
}

uint64_t Board::getKingAttacks(uint64_t king) const {
    uint64_t attacks = 0ULL;

    // Ruchy króla: 8 możliwych kierunków
    attacks |= (king << 8); // w górę
    attacks |= (king >> 8); // w dół
    attacks |= (king << 1) & ~FILE_A; // w prawo
    attacks |= (king >> 1) & ~FILE_H; // w lewo
    attacks |= (king << 9) & ~FILE_A; // w górę + w prawo
    attacks |= (king << 7) & ~FILE_H; // w górę + w lewo
    attacks |= (king >> 7) & ~FILE_A; // w dół + w prawo
    attacks |= (king >> 9) & ~FILE_H; // w dół + w lewo

    return attacks;
}

uint64_t Board::getTowerAttacks(int sq) const {
    // Pobieramy wszystkie figury na planszy (blokady)
    uint64_t occupied = getAllOccupied();
    // Wywołujemy naszą super szybką funkcję z magic.cpp
    return get_rook_attacks(sq, occupied);
}

uint64_t Board::getBishopAttacks(int sq) const {
    uint64_t occupied = getAllOccupied();
    return get_bishop_attacks(sq, occupied);
}

uint64_t Board::getQueenAttacks(int sq) const {
    uint64_t occupied = getAllOccupied();
    // Hetman to po prostu wieża i goniec w jednym!
    return get_rook_attacks(sq, occupied) | get_bishop_attacks(sq, occupied);
}



bool Board::isSquareAttacked(int sq, Color side) const {
    

    // 1. Skoczki
    if (getKnightAttacks(1ULL << sq) & pieces[static_cast<int>(side)][static_cast<int>(PieceType::KNIGHT)]) return true;

    // 2. Król
    if (getKingAttacks(1ULL << sq) & pieces[static_cast<int>(side)][static_cast<int>(PieceType::KING)]) return true;

    // 3. Wieże i Hetmany
    if (getTowerAttacks(sq) & (pieces[static_cast<int>(side)][static_cast<int>(PieceType::ROOK)] | pieces[static_cast<int>(side)][static_cast<int>(PieceType::QUEEN)])) return true;

    // 4. Gońce i Hetmany
    if (getBishopAttacks(sq) & (pieces[static_cast<int>(side)][static_cast<int>(PieceType::BISHOP)] | pieces[static_cast<int>(side)][static_cast<int>(PieceType::QUEEN)])) return true;

    // 5. Pionki
    if (getPawnAttacks(oppositeColor(side), 1ULL << sq) & pieces[static_cast<int>(side)][static_cast<int>(PieceType::PAWN)]) return true;

    return false;
}



PieceType Board::getPieceAt(int sq, Color color) const {
    uint64_t bit = (1ULL << sq);
    for (int p = 0; p < 6; p++) {
        if (pieces[static_cast<int>(color)][p] & bit) {
            return static_cast<PieceType>(p);
        }
    }
    return PieceType::NONE;
}




std::vector<Move> Board::generatePseudoLegalMoves(Color side) const {
    std::vector<Move> moves;
    uint64_t myPieces = getColorOccupied(side);
    Color oppColor = oppositeColor(side);
    uint64_t opponentPieces = getColorOccupied(oppColor);
    uint64_t allPieces = myPieces | opponentPieces;

    // --- GENEROWANIE RUCHÓW SKOCZKÓW ---
    uint64_t knights = pieces[static_cast<int>(side)][static_cast<int>(PieceType::KNIGHT)];
    while (knights) {
        int fromSq = __builtin_ctzll(knights);
        uint64_t validDestinations = getKnightAttacks(1ULL << fromSq) & ~myPieces;

        while (validDestinations) {
            int toSq = __builtin_ctzll(validDestinations);
            PieceType captured = getPieceAt(toSq, oppColor);
            moves.emplace_back(fromSq, toSq, PieceType::KNIGHT, captured);
            validDestinations &= (validDestinations - 1);
        }
        knights &= (knights - 1);
    }

    // --- FIGURY DALEKOBIEŻNE (Sliding Pieces) ---
    PieceType sliders[] = { PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN };
    for (PieceType type : sliders) {
        uint64_t sliderBB = pieces[static_cast<int>(side)][static_cast<int>(type)];
        while (sliderBB) {
            int fromSq = __builtin_ctzll(sliderBB);
            uint64_t attacks;
            if (type == PieceType::ROOK) attacks = getTowerAttacks(fromSq);
            else if (type == PieceType::BISHOP) attacks = getBishopAttacks(fromSq);
            else attacks = getQueenAttacks(fromSq);

            uint64_t validDestinations = attacks & ~myPieces;
            while (validDestinations) {
                int toSq = __builtin_ctzll(validDestinations);
                PieceType captured = getPieceAt(toSq, oppColor);
                moves.emplace_back(fromSq, toSq, type, captured);
                validDestinations &= (validDestinations - 1);
            }
            sliderBB &= (sliderBB - 1);
        }
    }

    // --- GENEROWANIE RUCHÓW KRÓLA + ROSZADA ---
    uint64_t king = pieces[static_cast<int>(side)][static_cast<int>(PieceType::KING)];
    if (king) {
        int fromSq = __builtin_ctzll(king);
        uint64_t validDestinations = getKingAttacks(1ULL << fromSq) & ~myPieces;

        while (validDestinations) {
            int toSq = __builtin_ctzll(validDestinations);
            PieceType captured = getPieceAt(toSq, oppColor);
            moves.emplace_back(fromSq, toSq, PieceType::KING, captured);
            validDestinations &= (validDestinations - 1);
        }

        // --- ROSZADY ---
        // Warunki: Prawa do roszady, brak figur pomiędzy, król nie jest w szachu (sprawdzane w LegalMoves)
        if (side == Color::WHITE) {
            // Krótka (WK) - bit 0 (wartość 1)
            if ((castlingRights & 1) && !((1ULL << 5) & allPieces) && !((1ULL << 6) & allPieces)) {
                // Sprawdzamy czy pola f1, g1 nie są atakowane
                if (!isSquareAttacked(4, Color::BLACK) && !isSquareAttacked(5, Color::BLACK) && !isSquareAttacked(6, Color::BLACK))
                    moves.emplace_back(4, 6, PieceType::KING, PieceType::NONE, PieceType::NONE, CASTLING); 
            }
            // Długa (WQ) - bit 1 (wartość 2)
            if ((castlingRights & 2) && !((1ULL << 1) & allPieces) && !((1ULL << 2) & allPieces) && !((1ULL << 3) & allPieces)) {
                if (!isSquareAttacked(4, Color::BLACK) && !isSquareAttacked(3, Color::BLACK) && !isSquareAttacked(2, Color::BLACK))
                    moves.emplace_back(4, 2, PieceType::KING, PieceType::NONE, PieceType::NONE, CASTLING);
            }
        } else {
            // Czarna Krótka (BK) - bit 2 (wartość 4)
            if ((castlingRights & 4) && !((1ULL << 61) & allPieces) && !((1ULL << 62) & allPieces)) {
                if (!isSquareAttacked(60, Color::WHITE) && !isSquareAttacked(61, Color::WHITE) && !isSquareAttacked(62, Color::WHITE))
                    moves.emplace_back(60, 62, PieceType::KING, PieceType::NONE, PieceType::NONE, CASTLING);
            }
            // Czarna Długa (BQ) - bit 3 (wartość 8)
            if ((castlingRights & 8) && !((1ULL << 57) & allPieces) && !((1ULL << 58) & allPieces) && !((1ULL << 59) & allPieces)) {
                if (!isSquareAttacked(60, Color::WHITE) && !isSquareAttacked(59, Color::WHITE) && !isSquareAttacked(58, Color::WHITE))
                    moves.emplace_back(60, 58, PieceType::KING, PieceType::NONE, PieceType::NONE, CASTLING);
            }
        }
    }

    // --- PIONKI ---
    uint64_t pawns = pieces[static_cast<int>(side)][static_cast<int>(PieceType::PAWN)];
    int direction = (side == Color::WHITE) ? 8 : -8;
    int startRank = (side == Color::WHITE) ? 1 : 6;
    int promotionRank = (side == Color::WHITE) ? 7 : 0;

    while (pawns) {
        int fromSq = __builtin_ctzll(pawns);
        int toSq = fromSq + direction;

        // 1. Ruch o jedno pole do przodu
        if (!((1ULL << toSq) & allPieces)) {
            if (toSq / 8 == promotionRank) {
                moves.emplace_back(fromSq, toSq, PieceType::PAWN, PieceType::NONE, PieceType::QUEEN, PROMOTION);
                moves.emplace_back(fromSq, toSq, PieceType::PAWN, PieceType::NONE, PieceType::ROOK, PROMOTION);
                moves.emplace_back(fromSq, toSq, PieceType::PAWN, PieceType::NONE, PieceType::BISHOP, PROMOTION);
                moves.emplace_back(fromSq, toSq, PieceType::PAWN, PieceType::NONE, PieceType::KNIGHT, PROMOTION);
            } else {
                moves.emplace_back(fromSq, toSq, PieceType::PAWN, PieceType::NONE);
                // 2. Ruch o dwa pola
                int doublePushSq = fromSq + 2 * direction;
                if (fromSq / 8 == startRank && !((1ULL << doublePushSq) & allPieces)) {
                    moves.emplace_back(fromSq, doublePushSq, PieceType::PAWN, PieceType::NONE);
                }
            }
        }

        // 3. Bicia standardowe
        uint64_t pawnAttacks = getPawnAttacks(side, (1ULL << fromSq));
        uint64_t captures = pawnAttacks & opponentPieces;
        while (captures) {
            int capSq = __builtin_ctzll(captures);
            PieceType capPiece = getPieceAt(capSq, oppColor);
            if (capSq / 8 == promotionRank) {
                moves.emplace_back(fromSq, capSq, PieceType::PAWN, capPiece, PieceType::QUEEN, PROMOTION);
                moves.emplace_back(fromSq, capSq, PieceType::PAWN, capPiece, PieceType::ROOK, PROMOTION);
                moves.emplace_back(fromSq, capSq, PieceType::PAWN, capPiece, PieceType::BISHOP, PROMOTION);
                moves.emplace_back(fromSq, capSq, PieceType::PAWN, capPiece, PieceType::KNIGHT, PROMOTION);
            } else {
                moves.emplace_back(fromSq, capSq, PieceType::PAWN, capPiece);
            }
            captures &= (captures - 1);
        }

        // 4. Bicie w przelocie (En Passant)
        uint64_t epCapture = pawnAttacks & enPassantSquare;
        if (epCapture) {
            int epSq = __builtin_ctzll(epCapture);
            // Uwaga: w EP bijemy pionka, mimo że docelowe pole jest puste!
            moves.emplace_back(fromSq, epSq, PieceType::PAWN, PieceType::PAWN, PieceType::NONE, EN_PASSANT);
        }

        pawns &= (pawns - 1);
    }
    return moves;
}



std::vector<Move> Board::generateLegalMoves(Color side) {
    std::vector<Move> pseudoMoves = generatePseudoLegalMoves(side);
    std::vector<Move> legalMoves;

    for (const auto& m : pseudoMoves) {
        makeMove(m, side);
        
        // Znajdź gdzie jest król po ruchu
        int kingSq = __builtin_ctzll(pieces[static_cast<int>(side)][static_cast<int>(PieceType::KING)]);
        
        // Jeśli pole króla nie jest atakowane przez przeciwnika - ruch jest legalny!
        if (!isSquareAttacked(kingSq, oppositeColor(side))) {
            legalMoves.push_back(m);
        }

        unmakeMove(m, side);
    }
    return legalMoves;
}


std::vector<Move> Board::generateLegalMoves() {
    Color side = sideToMove;
    std::vector<Move> pseudoMoves = generatePseudoLegalMoves(side);
    std::vector<Move> legalMoves;

    for (const auto& m : pseudoMoves) {
        makeMove(m, side);
        
        int kingSq = __builtin_ctzll(pieces[static_cast<int>(side)][static_cast<int>(PieceType::KING)]);
        if (!isSquareAttacked(kingSq, oppositeColor(side))) {
            legalMoves.push_back(m);
        }

        unmakeMove(m, side);
    }
    return legalMoves;
}