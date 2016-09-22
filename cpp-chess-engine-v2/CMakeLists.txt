cmake_minimum_required (VERSION 2.6)
project (chess_engine_v2)
set(CMAKE_CXX_FLAGS "-std=c++11 -Ofast")
add_executable (chess_engine_v2 main.cpp board.cpp)

# link_directories(/usr/local/lib)
# include_directories(/usr/local/include)

find_package( Boost COMPONENTS system thread filesystem coroutine regex REQUIRED )

include_directories(
   ${CMAKE_CURRENT_BINARY_DIR}
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${Boost_INCLUDE_DIRS}
)
target_link_libraries(chess_engine_v2
   ${Boost_LIBRARIES}
)

# target_link_libraries (chess_engine_v2 libboost_regex.dylib libboost_coroutine.dylib libboost_system.dylib libboost_filesystem.dylib)

# find_package(Boost)

# if (Boost_FOUND)
#    include_directories(${Boost_INCLUDE_DIRS})
#    target_link_libraries(chess_engine_v2 ${Boost_LIBRARIES})
# endif()

