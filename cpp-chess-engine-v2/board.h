#ifndef __BOARD_H_
#define __BOARD_H_

#include <stdint.h>
#include <type_traits>
#include <cassert>
#include <algorithm>
#include <string>

namespace chess {

typedef int8_t Piece;
typedef int8_t Position;
typedef int32_t Player;
typedef int Score;

const int BOARD_SPACES = 64;
const int BOARD_DIM = 8;

const Piece PIECE_EMPTY = 0;
const Piece PIECE_NULL = INT8_MAX;
const Piece PIECE_PAWN = 1;
const Piece PIECE_KNIGHT = 2;
const Piece PIECE_BISHOP = 3;
const Piece PIECE_ROOK = 4;
const Piece PIECE_KING = 5;
const Piece PIECE_QUEEN = 6;

const int8_t FLAG_WHITE_CASTLED = 2;
const int8_t FLAG_BLACK_CASTLED = 4;


/*
 get piece values
 */
inline int pieceGetValue(Piece piece) {
    switch (piece) {
        case PIECE_PAWN:
            return 1;
        case PIECE_KNIGHT:
            return 3;
        case PIECE_BISHOP:
            return 3;
        case PIECE_ROOK:
            return 5;
        case PIECE_QUEEN:
            return 9;
        case PIECE_KING:
            return 1000;
        case PIECE_NULL:
            return -1;
        default:
            return 0;
    }
}

inline int pieceGetValueSigned(Piece piece) {
    int value = pieceGetValue(piece < 0 ? -piece : piece);
    return piece < 0 ? -value : value;
}

/*
 get piece letters, also very fast
 */

inline char pieceGetLetter(Piece piece) {
    if (piece < 0)
        piece = -piece;
    switch (piece) {
        case PIECE_PAWN:
            return 'P';
        case PIECE_KNIGHT:
            return 'N';
        case PIECE_BISHOP:
            return 'B';
        case PIECE_ROOK:
            return 'R';
        case PIECE_QUEEN:
            return 'Q';
        case PIECE_KING:
            return 'K';
        case PIECE_NULL:
            return '-';
        case PIECE_EMPTY:
            return ' ';
        default:
            return '?';
    }
}


/*
 a chess board!
 */
struct Board {
    int score;
    int8_t haveCastled;
    Piece pieces[BOARD_SPACES];

    Board();

    inline void setPiece(Position position, Piece piece) {
        if (pieces[position] != 0)
            score -= pieceGetValueSigned(pieces[position]);
        if (piece != 0)
            score += pieceGetValueSigned(piece);

        pieces[position] = piece;
    }

    inline Piece pieceAt(Position position) { return pieces[position]; };

    inline int getScore(const Player player) { return score * player; };
    inline int getScore() { return score; };

    inline static int getX(int index) { return index % BOARD_DIM; };
    inline static int getY(int index) { return index / BOARD_DIM; };
    inline static int toIndex(int x, int y) { return x + y * BOARD_DIM; };

    inline bool hasPlayerCastled(Player player) {
        if (player == 1)
            return (haveCastled & FLAG_WHITE_CASTLED) > 0;
        else if(player == -1)
            return (haveCastled & FLAG_BLACK_CASTLED) > 0;
        return false;
    }
    
    void setup();

    void print() const;
};

/*
 essentially a move in a chess game
 */
struct Move {
    struct PiecePositionPair {
        Piece piece;
        Position position;
    };
    PiecePositionPair changes[4];

    inline Move() {
        changes[0].position = -1;
    }

    inline Move(Board* board, int from, int to) {
        changes[0].position = from;
        changes[0].piece = PIECE_EMPTY;
        changes[1].position = to;
        changes[1].piece = board->pieceAt(from);
        changes[2].position = -1;
    }

    // used for pawn promotion
    inline Move(Board* board, int from, int to, Piece promotion) {
        changes[0].position = from;
        changes[0].piece = PIECE_EMPTY;
        changes[1].position = to;
        changes[1].piece = promotion;
        changes[2].position = -1;
    }

    inline void castle(Board* board, int from, int to) {
        changes[0].position = from;
        changes[0].piece = board->pieceAt(to);
        changes[1].position = to;
        changes[1].piece = board->pieceAt(from);
        changes[2].position = -2;
        if (board->pieceAt(to) > 0) {
            changes[2].piece = (board->haveCastled | FLAG_WHITE_CASTLED);
        } else {
            changes[2].piece = (board->haveCastled | FLAG_BLACK_CASTLED);
        }
    }

    inline void apply(Board* board) {
        assert(changes[0].position != -1);
        for (int i = 0; i < sizeof(changes) / sizeof(PiecePositionPair); ++i) {
            if (changes[i].position < 0) {
                if (changes[i].position == -2) {
                    int8_t temp = changes[i].piece;
                    board->haveCastled = changes[i].piece;
                    changes[i].piece = temp;
                }
                break ;
            }
            Piece temp = board->pieceAt(changes[i].position);
            board->setPiece(changes[i].position, changes[i].piece);
            changes[i].piece = temp;
        }
    }

    std::string toString() const;
};

inline std::ostream& operator << (std::ostream& o, const Move& move) {
    o << move.toString();
    return o;
}

/*
 iterate the moves available to the player given a board state
 */
template<class STORE> void generateMoves(Board* board, Player player, STORE& store);

struct MoveIterator {
    int moveCount;
    Move moves[128];

    MoveIterator(Board* board, Player player) {
        moveCount = 0;
        generateMoves<MoveIterator>(board, player, *this);
    }

    void put(const Move& move) {
        assert(moveCount <= 128);
        moves[moveCount++] = move;
    }

     void sort(Board* board) {
         /*std::sort(moves, moves + moveCount, [board](Move& move1, Move& move2) {
             move1.apply(board);
             int score1 = board->getScore();
             move1.apply(board);
             move2.apply(board);
             int score2 = board->getScore();
             move2.apply(board);

             return score1 > score2;
         });*/
     }

    bool getNext(Move& move) {
        if (moveCount == 0)
            return false;
        move = moves[--moveCount];
        return true;
    }
};
 
inline bool operator == (const Move& a, const Move& b) {
    for (int i = 0; i < 4; ++i) {
        if (a.changes[i].position < 0 && b.changes[i].position < 0)
            break;

        if (a.changes[i].piece != b.changes[i].piece || b.changes[i].position != a.changes[i].position)
            return false;
    }
    return true;
}
    
    

};

#endif