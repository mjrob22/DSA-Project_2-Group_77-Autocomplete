cmake_minimum_required(VERSION 4.1)
project(DSA_Project2)

set(CMAKE_CXX_STANDARD 20)

add_executable(DSA_Project2 main.cpp trie.h
        benchmark_trie.cpp)
add_executable(benchmark_trie benchmark_trie.cpp trie.h)
add_executable(test_trie test_trie.cpp trie.h)
