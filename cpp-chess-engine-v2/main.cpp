#include "board.h"
#include "smartness.h"
#include "benchmarking.h"
#include "include/server-http.hpp"

#include <stdio.h>
#include <boost/filesystem.hpp>

#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost::property_tree;


/*
 utility to make a move given a chess board and the player
 */
void makemove(chess::Board* board, chess::Player player) {
    std::cout << "computing moves for player: " << player << std::endl;
    std::cout << "begin iterative deepening... " << std::endl;
    
    benchmarking::Benchmark benchmark;
    
    std::vector<chess::Move> moves;
    for (int i = 2; i <= 7; ++i) {
        benchmark.push();
        smartness::MinimaxAlphaBeta minimax(board, player, i, moves);
        moves = std::vector<chess::Move>();
        minimax.getMoveVector(moves);
        std::cout << "\tdepth " << i << "(" << benchmark.pop() << " us): ";
        for (auto& move : moves) {
            std::cout << move << " - ";
        }
        std::cout << std::endl;
    }
    
    chess::Move move = moves[0];
    move.apply(board);
    
    board->print();
}


/*
 program interactive modes
 */
int mode_test() {
    std::cout << "Chess engine v2 by Gareth George" << std::endl;
    chess::Board board;
    board.setup();
    
    chess::Move move;
    
    while (true) {
        
        makemove(&board, 1);
        makemove(&board, -1);
        
    }
}

typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;

int mode_webui(int port);


/*
 main entry point
 */
int main(int argc, char* argv[]) {
    std::cout << "please enter mode (web or test): " << std::endl;
    
    std::string mode;
    std::cin >> mode;
    
    if (mode == "test") {
        mode_test();
    } else if (mode == "web") {
        mode_webui(8080);
    } else {
        std::cerr << "no such mode!" << std::endl;
    }
}


int mode_webui(int port) {
    std::cout << "Chess AI by Gareth George" << std::endl;
    std::cout << "\tweb interface loading. port: " << port << std::endl;
    
    //HTTP-server at port 8080 using 4 threads
    HttpServer server(8080, 4);
    
    server.resource["^/ai$"]["POST"]=[](HttpServer::Response& response, shared_ptr<HttpServer::Request> request) {
        std::cout << "got request to /ai" << std::endl;
        chess::Board board;
        
        try {
            
            /*
             read the json request
             */
            ptree pt;
            read_json(request->content, pt);
            
            /*
             read the current turn
             */
            string currentTurnStr = pt.get<string>("turn");
            int currentTurn = currentTurnStr == "black" ? -1 : 1;
            
            std::cout << "\tcurrent turn: " << currentTurnStr << std::endl;
            
            /*
             construct the board game from the json data
             */
            BOOST_FOREACH(ptree::value_type& v, pt.get_child("position"))
            {
                std::cout << "\t" << v.first.data() << ":" << v.second.data() << std::endl;
                
                string positionStr = boost::lexical_cast<string>(v.first.data());
                string pieceStr = boost::lexical_cast<string>(v.second.data());
                if (positionStr.length() < 2 || pieceStr.length() < 2) {
                    std::cout << "\tskipping piece, malformatted location!" << std::endl;
                    continue ;
                }
                
                int x = positionStr.c_str()[0] - 'a';
                int y = positionStr.c_str()[1] - '1';
                int team = pieceStr.c_str()[0] == 'b' ? -1 : 1;
                char piece = pieceStr.c_str()[1];
                if (piece == 'R')
                    piece = chess::PIECE_ROOK;
                else if (piece == 'N')
                    piece = chess::PIECE_KNIGHT;
                else if (piece == 'B')
                    piece = chess::PIECE_BISHOP;
                else if (piece == 'Q')
                    piece = chess::PIECE_QUEEN;
                else if (piece == 'K')
                    piece = chess::PIECE_KING;
                else if (piece == 'P')
                    piece = chess::PIECE_PAWN;
                else
                    piece = 0;
                piece *= team;
                
                board.setPiece(chess::Board::toIndex(x, y), piece);
            }
            
            /*
             make a move!
             */
            board.print();
            
            makemove(&board, currentTurn);
            
            /*
             feed it back
             */
            ptree resTree;
            for (int i = 0; i < chess::BOARD_SPACES; ++i) {
                if (board.pieceAt(i) == chess::PIECE_EMPTY) continue ;
                
                std::stringstream posStream;
                posStream << (char) (chess::Board::getX(i) + 'a') << (char) (chess::Board::getY(i) + '1');
                std::string posStr = posStream.str();
                
                std::stringstream pieceStream;
                pieceStream << (board.pieceAt(i) < 0 ? 'b' : 'w') << chess::pieceGetLetter(board.pieceAt(i));
                std::string pieceStr = pieceStream.str();
                
                resTree.put(posStr, pieceStr);
            }
            
            std::stringstream out;
            write_json(out, resTree);
            std::string outStr = out.str();
            
            response << "HTTP/1.1 200 OK\r\nContent-Length: " << outStr.length() << "\r\n\r\n" << outStr;
        }
        catch(exception& e) {
            std::cout << e.what() << std::endl;
            response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << strlen(e.what()) << "\r\n\r\n" << e.what();
        }
    };
    
    server.default_resource["GET"]=[](HttpServer::Response& response, shared_ptr<HttpServer::Request> request) {
        const auto web_root_path=boost::filesystem::canonical("web");
        boost::filesystem::path path=web_root_path;
        path/=request->path;
        if(boost::filesystem::exists(path)) {
            path=boost::filesystem::canonical(path);
            //Check if path is within web_root_path
            if(distance(web_root_path.begin(), web_root_path.end())<=distance(path.begin(), path.end()) &&
               equal(web_root_path.begin(), web_root_path.end(), path.begin())) {
                if(boost::filesystem::is_directory(path))
                    path/="index.html";
                if(boost::filesystem::exists(path) && boost::filesystem::is_regular_file(path)) {
                    ifstream ifs;
                    ifs.open(path.string(), ifstream::in | ios::binary);
                    
                    if(ifs) {
                        ifs.seekg(0, ios::end);
                        size_t length=ifs.tellg();
                        
                        ifs.seekg(0, ios::beg);
                        
                        response << "HTTP/1.1 200 OK\r\nContent-Length: " << length << "\r\n\r\n";
                        
                        //read and send 128 KB at a time
                        const size_t buffer_size=131072;
                        vector<char> buffer(buffer_size);
                        size_t read_length;
                        try {
                            while((read_length=ifs.read(&buffer[0], buffer_size).gcount())>0) {
                                response.write(&buffer[0], read_length);
                                response.flush();
                            }
                        }
                        catch(const exception &e) {
                            cerr << "Connection interrupted, closing file" << endl;
                        }
                        
                        ifs.close();
                        return;
                    }
                }
            }
        }
        string content="Could not open path "+request->path;
        response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
    };
    
    std::cout << "\tlaunched server..." << std::endl;
    server.start();
    
    return 0;
};