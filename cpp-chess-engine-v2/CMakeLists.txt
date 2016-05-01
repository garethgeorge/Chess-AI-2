cmake_minimum_required (VERSION 2.6)
project (chess_engine_v2)
set(CMAKE_CXX_FLAGS "-std=c++11 -Ofast")
add_executable (chess_engine_v2 main.cpp board.cpp)

link_directories(/usr/local/lib)
include_directories(/usr/local/include)

target_link_libraries (chess_engine_v2 libboost_regex.dylib libboost_coroutine.dylib libboost_system.dylib libboost_filesystem.dylib)
