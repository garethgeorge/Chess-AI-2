#ifndef __SMARTNESS_H_
#define __SMARTNESS_H_

#include "board.h"
#include <stdint.h>
#include <climits>
#include <iostream>
#include <vector>
#include "benchmarking.h"

namespace smartness {
    using namespace chess;

    struct MinimaxAlphaBeta {
        int maxDepth;
        int movesSearched;

        std::vector<Move> bestMovesAtDepths;
        Board* board;
        Player player;

        MinimaxAlphaBeta(Board* board, Player player, int maxDepth) : board(board), player(player), maxDepth(maxDepth) {
            
        }

        MinimaxAlphaBeta(Board* board, Player player, int maxDepth, const std::vector<Move>& bestMoves) : MinimaxAlphaBeta(board, player, maxDepth) {
            bestMovesAtDepths = bestMoves;
        }

        int run(int depth, int alpha, int beta, int color, Move& bestMove) {
            // return when cutoff depth is hit
            if (depth >= maxDepth)
                return board->getScore() * player;

            const Player curTurn = player * color;

            Move trash;
            Move move;
            MoveIterator iter(board, curTurn);

            iter.sort(board);

            if (movesSearched == depth && bestMovesAtDepths.size() > depth) {
                move = bestMovesAtDepths[depth];
            } else {
                if (!iter.getNext(move))
                    return board->getScore() * player;
            }

            if (player == curTurn) {
                int max = INT_MIN;

                do {
                    movesSearched++;
                    move.apply(board);
                    int score = run(depth + 1, alpha, beta, -color, trash);
                    move.apply(board);

                    if (score > max) {
                        bestMove = move;
                        max = score;
                    }
                    if (score > alpha) {
                        alpha = score;
                    }
                    if (beta <= alpha)
                        break ;
                    
                } while (iter.getNext(move));

                return max;
            } else {
                int min = INT_MAX;

                do {
                    movesSearched++;
                    move.apply(board);
                    int score = run(depth + 1, alpha, beta, -color, trash);
                    move.apply(board);

                    if (score < min) {
                        bestMove = move;
                        min = score;
                    }
                    if (score < beta) {
                        beta = score;
                    }
                    if (beta <= alpha)
                        break ;
                } while (iter.getNext(move));

                return min;
            }
        }

        int run(Move& bestMove) {
            movesSearched = 0;
            return run(0, INT_MIN, INT_MAX, 1, bestMove);
        }

        // get a vector of all the recommended moves!
        void getMoveVector(std::vector<Move>& moves) {
            benchmarking::Benchmark benchmark;
            //std::cout << "\tcomputing move vector" << std::endl;
            for (int i = 0; i < maxDepth; ++i) {
                Move bestMove;
                
                //benchmark.push();
                movesSearched = 0;
                run(i, INT_MIN, INT_MAX, i % 2 == 0 ? 1 : -1, bestMove);
                //clock_t timeTook = benchmark.pop();
                //std::cout << "\t\tdepth: " << i << " time: " << timeTook << " moves: " << movesSearched << std::endl;
                
                bestMove.apply(board);
                moves.push_back(bestMove);
            }

            for (int i = maxDepth - 1; i >= 0; --i) {
                moves[i].apply(board);
            }
        }
        
        
    };

    
    /*
        old implementation!
     */
    
    using namespace chess;
    int minimax_alphabeta(Board* board, Player player, int depth, int alpha, int beta, int color, Move& bestMove) {
        if (depth == 0) {
            return board->getScore() * player;
        }

        const Player curTurn = player * color;

        Move trash;
        Move move;
        MoveIterator iter(board, curTurn);

        iter.sort(board); // improve move ordering

        if (player == curTurn) {
            int max = INT_MIN;
            while (iter.getNext(move)) {
                move.apply(board);
                int score = minimax_alphabeta(board, player, depth - 1, alpha, beta, -color, trash);
                move.apply(board);

                if (score > max) {
                    bestMove = move;
                    max = score;
                }
                if (score > alpha) {
                    alpha = score;
                }
                //if (beta <= alpha) {
                //    break ;
                //}
            }
            return max;
        } else if (color == -1) {
            int min = INT_MAX;
            while (iter.getNext(move)) {
                move.apply(board);
                int score = minimax_alphabeta(board, player, depth - 1, alpha, beta, -color, trash);
                move.apply(board);

                if (score < min) {
                    bestMove = move;
                    min = score;
                }
                if (score < beta)
                    beta = score;
                //if (beta <= alpha)
                //    break;
            }
            return min;
        }
        return 0;
    };

    int minimax_alphabeta(Board* board, Player player, int depth, Move& bestMove) {
        return minimax_alphabeta(board, player, depth, INT_MIN, INT_MAX, 1, bestMove);
    };

    void minimax_alphabeta_vector(Board* board, Player player, int depth, std::vector<Move>& moves) {
        for (int i = 0; i < depth; ++i) {
            Move bestMove;
            minimax_alphabeta(board, player, depth - i, bestMove);

            bestMove.apply(board);
            moves.push_back(bestMove);
        }

        for (int i = depth - 1; i >= 0; --i) {
            moves[i].apply(board);
        }
    }
}

#endif