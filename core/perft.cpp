#include <iostream>
#include <chrono>
#include "board.hpp"
#include "magic.hpp"

// Setup standard starting position
void setupStartPosition(Board& b) {
    for(int i=0; i<2; i++) for(int j=0; j<7; j++) b.pieces[i][j] = 0ULL;

    b.pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::PAWN)]   = 0x000000000000FF00ULL;
    b.pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::KNIGHT)] = 0x0000000000000042ULL;
    b.pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::BISHOP)] = 0x0000000000000024ULL;
    b.pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::ROOK)]   = 0x0000000000000081ULL;
    b.pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::QUEEN)]  = 0x0000000000000008ULL;
    b.pieces[static_cast<int>(Color::WHITE)][static_cast<int>(PieceType::KING)]   = 0x0000000000000010ULL;

    b.pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::PAWN)]   = 0x00FF000000000000ULL;
    b.pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::KNIGHT)] = 0x4200000000000000ULL;
    b.pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::BISHOP)] = 0x2400000000000000ULL;
    b.pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::ROOK)]   = 0x8100000000000000ULL;
    b.pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::QUEEN)]  = 0x0800000000000000ULL;
    b.pieces[static_cast<int>(Color::BLACK)][static_cast<int>(PieceType::KING)]   = 0x1000000000000000ULL;
    
    b.sideToMove = Color::WHITE;
    b.castlingRights = 15;
    b.enPassantSquare = 0ULL;
}

int main() {
    init_magic_tables();
    Board board;
    setupStartPosition(board);

    std::cout << "Starting Perf Test at Depth 5..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    uint64_t nodes = board.perft(5);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "Depth 5 Nodes: " << nodes << std::endl;
    std::cout << "Time elapsed: " << diff.count() << " s" << std::endl;
    std::cout << "NPS: " << (uint64_t)(nodes / diff.count()) << std::endl;
    
    if (nodes == 4865609) {
        std::cout << "PERFT TEST PASSED! SUCCESS!" << std::endl;
    } else {
        std::cout << "PERFT TEST FAILED! Expected 4865609." << std::endl;
    }

    return 0;
}
