#include "board.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include "include/termcolor.h"

namespace chess {

    Board::Board() {
        // zero fill the board
        std::fill(pieces, pieces + BOARD_SPACES, 0);
        score = 0; // zero the score
        haveCastled = 0;
    }
    
    void Board::setup() {
        
        for (int i = 0; i < BOARD_DIM; ++i) {
            this->pieces[BOARD_DIM * 1 + i] =  PIECE_PAWN;
            this->pieces[BOARD_DIM * 6 + i] = -PIECE_PAWN;
        }
        
        this->pieces[3] = PIECE_QUEEN;
        this->pieces[4] = PIECE_KING;
        this->pieces[0] = this->pieces[7] = PIECE_ROOK;
        this->pieces[1] = this->pieces[6] = PIECE_KNIGHT;
        this->pieces[2] = this->pieces[5] = PIECE_BISHOP;
        
        const int blackOffset = BOARD_DIM * 7;
        this->pieces[blackOffset + 0] = this->pieces[blackOffset + 7] = -PIECE_ROOK;
        this->pieces[blackOffset + 1] = this->pieces[blackOffset + 6] = -PIECE_KNIGHT;
        this->pieces[blackOffset + 2] = this->pieces[blackOffset + 5] = -PIECE_BISHOP;
        this->pieces[blackOffset + 3] = -PIECE_QUEEN;
        this->pieces[blackOffset + 4] = -PIECE_KING;
    }

    void Board::print() const {
        auto& ss = std::cout;
        ss << termcolor::reset << " " << termcolor::grey << termcolor::on_white;
        for (int i = 0; i < BOARD_DIM; ++i) {
            ss << (char) ('a' + i);
        }
        ss << termcolor::reset << std::endl;

        for (int y = 0; y < BOARD_DIM; ++y) {
            ss << termcolor::on_white << termcolor::grey << (y + 1);
            for (int x = 0; x < BOARD_DIM; ++x) {
                int i = y * BOARD_DIM + x;
                Piece p = this->pieces[i];
                if ((i + i / 8) % 2 == 0)
                    ss << termcolor::on_grey;
                else
                    ss << termcolor::on_cyan;

                if (p != 0) {
                    if (p < 0) {
                        p = -p;
                        ss << termcolor::red;
                    } else
                        ss << termcolor::white;

                    ss << pieceGetLetter(p);
                } else
                    ss << " ";

                ss << termcolor::reset;
            }
            ss << termcolor::reset << std::endl;
        }
    }


    /*
     move generator which is crazily (maybe almost stupidly recursively templated)
     */

    namespace mg {
        struct OnlyIfEmpty {
            inline static bool add(Player player, Piece piece) {
                return piece == 0;
            }

            inline static bool cont(Player player, Piece piece) {
                return true;
            }
        };
        struct OnlyIfEmptyOrCapture {
            inline static bool add(Player player, Piece piece) {
                return piece * player <= 0;
            }

            inline static bool cont(Player player, Piece piece) {
                return piece * player == 0;
            }
        };

        struct OnlyIfCapture {
            inline static bool add(Player player, Piece piece) {
                return player * piece < 0;
            }

            inline static bool cont(Player player, Piece piece) {
                return false;
            }
        };


        template<typename T>
        inline T min(T a, T b) {
            return a > b ? a : b;
        }

        template<class STORE, class CONDITIONAL>
        bool moveTo(Board* board, int from, int to, Player player, STORE& iter) {
            if (!CONDITIONAL::add(player, board->pieceAt(to)))
                return false;
            iter.put(Move(board, from, to));
            return CONDITIONAL::cont(player, board->pieceAt(to));
        }

        template<class STORE, class CONDITIONAL, int dx, int dy>
        void addAlongVector(Board* board, int index, Player player, STORE& iter) {
            const int indexOffset = dy * BOARD_DIM + dx;
            const int x = Board::getX(index);
            const int y = Board::getY(index);

            int reps = INT_MAX;
            if (dx != 0) {
                if (dx < 0) {
                    if (reps > x / (-dx))
                        reps = x / (-dx);
                } else {
                    if (reps > (BOARD_DIM - x - 1) / dx)
                        reps = (BOARD_DIM - x - 1) / dx;
                }
            }
            if (dy != 0) {
                if (dy < 0) {
                    if (reps > y / (-dy))
                        reps = y / (-dy);
                } else {
                    if (reps > (BOARD_DIM - y - 1) / dy)
                        reps = (BOARD_DIM - y - 1) / dy;
                }
            }

            for (int i = 1; i <= reps; ++i) {
                if (!moveTo<STORE, CONDITIONAL>(board, index, index + indexOffset * i, player, iter))
                    break ;
            }
        };

        template<class STORE, class CONDITIONAL, int dx, int dy>
        bool moveToIfInBounds(Board* board, int from, Player player, STORE& iter) {
            int x = Board::getX(from) + dx;
            int y = Board::getY(from) + dy;
            if (x < 0 || x >= BOARD_DIM || y < 0 || y >= BOARD_DIM)
                return false;
            if (CONDITIONAL::add(player, board->pieceAt(Board::toIndex(x, y)))) {
                iter.put(Move(board, from, Board::toIndex(x, y)));
                return true;
            }
            return false;
        }

        template<class STORE>
        void addMovesAtPosition(Board* board, int from, Player player, STORE& iter) {
            int x = Board::getX(from);
            int y = Board::getY(from);

            Piece p = board->pieceAt(from) * player;

            switch (p) {
                case PIECE_PAWN:

                    if (player == 1) {
                        if (y == 6) {
                            // pawn promotion!
                            if (OnlyIfEmpty::add(player, board->pieceAt(Board::toIndex(x, 7)))) {
                                // only two pieces that you should ever really want to add...
                                iter.put(Move(board, from, Board::toIndex(x, 7), PIECE_QUEEN));
                                iter.put(Move(board, from, Board::toIndex(x, 7), PIECE_KNIGHT));
                            }
                        } else if (moveToIfInBounds<STORE, OnlyIfEmpty,0,1>(board, from, player, iter) && y == 1) {
                            moveToIfInBounds<STORE, OnlyIfEmpty,0,2>(board, from, player, iter);
                        }
                        moveToIfInBounds<STORE, OnlyIfCapture, 1, 1>(board, from, player, iter);
                        moveToIfInBounds<STORE, OnlyIfCapture, -1, 1>(board, from, player, iter);
                    } else if (player == -1) {
                        if (y == 1) {
                            // pawn promotion!
                            if (OnlyIfEmpty::add(player, board->pieceAt(Board::toIndex(x, 0)))) {
                                // only two pieces that you should ever really want to add...
                                iter.put(Move(board, from, Board::toIndex(x, 0), -PIECE_QUEEN));
                                iter.put(Move(board, from, Board::toIndex(x, 0), -PIECE_KNIGHT));
                            }
                        } else if (moveToIfInBounds<STORE, OnlyIfEmpty,0,-1>(board, from, player, iter) && y == 6) {
                            moveToIfInBounds<STORE, OnlyIfEmpty,0,-2>(board, from, player, iter);
                        }
                        moveToIfInBounds<STORE, OnlyIfCapture, 1, -1>(board, from, player, iter);
                        moveToIfInBounds<STORE, OnlyIfCapture, -1, -1>(board, from, player, iter);
                    }

                    break ;

                case PIECE_KNIGHT:

                    moveToIfInBounds<STORE, OnlyIfEmptyOrCapture, 2,1>(board, from, player, iter);
                    moveToIfInBounds<STORE, OnlyIfEmptyOrCapture, 1,2>(board, from, player, iter);

                    moveToIfInBounds<STORE, OnlyIfEmptyOrCapture, 2,-1>(board, from, player, iter);
                    moveToIfInBounds<STORE, OnlyIfEmptyOrCapture, 1,-2>(board, from, player, iter);

                    moveToIfInBounds<STORE, OnlyIfEmptyOrCapture, -2,1>(board, from, player, iter);
                    moveToIfInBounds<STORE, OnlyIfEmptyOrCapture, -1,2>(board, from, player, iter);

                    moveToIfInBounds<STORE, OnlyIfEmptyOrCapture, -2,-1>(board, from, player, iter);
                    moveToIfInBounds<STORE, OnlyIfEmptyOrCapture, -1,-2>(board, from, player, iter);

                    break ;

                case PIECE_ROOK:

                    addAlongVector<STORE, OnlyIfEmptyOrCapture, 1, 0>(board, from, player, iter);
                    addAlongVector<STORE, OnlyIfEmptyOrCapture, 0, 1>(board, from, player, iter);
                    addAlongVector<STORE, OnlyIfEmptyOrCapture, -1, 0>(board, from, player, iter);
                    addAlongVector<STORE, OnlyIfEmptyOrCapture, 0, -1>(board, from, player, iter);

                    break ;

                case PIECE_BISHOP:

                    addAlongVector<STORE, OnlyIfEmptyOrCapture, 1, 1>(board, from, player, iter);
                    addAlongVector<STORE, OnlyIfEmptyOrCapture, -1, 1>(board, from, player, iter);
                    addAlongVector<STORE, OnlyIfEmptyOrCapture, 1, -1>(board, from, player, iter);
                    addAlongVector<STORE, OnlyIfEmptyOrCapture, -1, -1>(board, from, player, iter);

                    break ;

                case PIECE_QUEEN:

                    addAlongVector<STORE, OnlyIfEmptyOrCapture, 1, 1>(board, from, player, iter);
                    addAlongVector<STORE, OnlyIfEmptyOrCapture, -1, 1>(board, from, player, iter);
                    addAlongVector<STORE, OnlyIfEmptyOrCapture, 1, -1>(board, from, player, iter);
                    addAlongVector<STORE, OnlyIfEmptyOrCapture, -1, -1>(board, from, player, iter);

                    addAlongVector<STORE, OnlyIfEmptyOrCapture, 1, 0>(board, from, player, iter);
                    addAlongVector<STORE, OnlyIfEmptyOrCapture, 0, 1>(board, from, player, iter);
                    addAlongVector<STORE, OnlyIfEmptyOrCapture, -1, 0>(board, from, player, iter);
                    addAlongVector<STORE, OnlyIfEmptyOrCapture, 0, -1>(board, from, player, iter);

                    break ;

                case PIECE_KING:

                    moveToIfInBounds<STORE, OnlyIfEmptyOrCapture, 0, 1>(board, from, player, iter);
                    moveToIfInBounds<STORE, OnlyIfEmptyOrCapture, 0, -1>(board, from, player, iter);
                    moveToIfInBounds<STORE, OnlyIfEmptyOrCapture, 1, 0>(board, from, player, iter);
                    moveToIfInBounds<STORE, OnlyIfEmptyOrCapture, -1, 0>(board, from, player, iter);

                    moveToIfInBounds<STORE, OnlyIfEmptyOrCapture, 1, 1>(board, from, player, iter);
                    moveToIfInBounds<STORE, OnlyIfEmptyOrCapture, 1, -1>(board, from, player, iter);
                    moveToIfInBounds<STORE, OnlyIfEmptyOrCapture, -1, 1>(board, from, player, iter);
                    moveToIfInBounds<STORE, OnlyIfEmptyOrCapture, -1, -1>(board, from, player, iter);

                    break ;

                default:
                    return ;
            }
        }
    }

    template<class STORE> void generateMoves(Board* board, Player player, STORE& iter) {
        for (int i = BOARD_SPACES - 1; i >= 0; --i) {
            if (board->pieceAt(i) * player > 0) {
                mg::addMovesAtPosition<STORE>(board, i, player, iter);
            }
        }
    }


    template void generateMoves<MoveIterator>(Board* board, Player player, MoveIterator& iter);
    
    std::string Move::toString() const {
        std::stringstream ss;
        
        for (int i = 0; i < sizeof(changes) / sizeof(PiecePositionPair); ++i) {
            if (changes[i].position < 0) {
                if (changes[i].position == -2) {
                    ss << "castle" << std::endl;
                    break ;
                }
                break ;
            }
            ss << (char) ('a' + Board::getX(changes[i].position)) << Board::getY(changes[i].position) << "=" << pieceGetLetter(changes[i].piece) << ":";
        }
        return ss.str();
    }

};