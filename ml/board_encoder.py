"""
board_encoder.py — Konwersja pozycji szachowej (FEN) na wektor cech 768-wymiarowy

Układ cech (spójny z C++ evaluate.cpp :: boardToFeatures):
  feature[color * 6 * 64 + piece_type * 64 + square] = 1.0

color:      0=białe, 1=czarne
piece_type: 0=pion, 1=skoczek, 2=goniec, 3=wieża, 4=hetman, 5=król
square:     0=a1, 1=b1, ..., 63=h8
"""

import numpy as np
import chess


PIECE_TYPE_MAP = {
    chess.PAWN:   0,
    chess.KNIGHT: 1,
    chess.BISHOP: 2,
    chess.ROOK:   3,
    chess.QUEEN:  4,
    chess.KING:   5,
}

COLOR_MAP = {
    chess.WHITE: 0,
    chess.BLACK: 1,
}

FEATURES_SIZE = 768  # 2 * 6 * 64


def fen_to_features(fen: str) -> np.ndarray:
    """
    Konwertuje FEN na wektor float32 o rozmiarze 768.
    Zwraca array z perspektywy białych (kolor nie jest odwracany).
    """
    board = chess.Board(fen)
    feat = np.zeros(FEATURES_SIZE, dtype=np.float32)

    for sq in chess.SQUARES:
        piece = board.piece_at(sq)
        if piece is None:
            continue
        c = COLOR_MAP[piece.color]
        p = PIECE_TYPE_MAP[piece.piece_type]
        idx = c * 6 * 64 + p * 64 + sq
        feat[idx] = 1.0

    return feat


def batch_fen_to_features(fens: list[str]) -> np.ndarray:
    """Vectorized batch konwersja."""
    return np.stack([fen_to_features(f) for f in fens], axis=0)


if __name__ == "__main__":
    # Prosty test — pozycja startowa
    START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    f = fen_to_features(START_FEN)
    assert f.sum() == 32.0, f"Oczekiwano 32 figur, got {f.sum()}"
    print(f"board_encoder OK | features shape: {f.shape} | figur: {int(f.sum())}")
