#include "board.hpp"

uint64_t Board::getColorOccupied(Color color) const {
    uint64_t occupied = 0ULL;
    for (int pieceType = 0; pieceType < 6; ++pieceType)
        occupied |= pieces[static_cast<int>(color)][pieceType];
    return occupied;
}

uint64_t Board::getAllOccupied() const {
    return getColorOccupied(Color::WHITE) | getColorOccupied(Color::BLACK);
}