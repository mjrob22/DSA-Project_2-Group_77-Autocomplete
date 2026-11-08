cmake_minimum_required(VERSION 3.10)
project(DSA_Project2 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)   # timings are only meaningful with optimizations on
endif()

find_package(Threads REQUIRED)

# Main program: terminal menu + Wordplay web app
add_executable(DSA_Project2 main.cpp)
target_link_libraries(DSA_Project2 PRIVATE Threads::Threads)
if(WIN32)
    # cpp-httplib needs Windows 10 headers and the Windows sockets library
    target_compile_definitions(DSA_Project2 PRIVATE _WIN32_WINNT=0x0A00)
    target_link_libraries(DSA_Project2 PRIVATE ws2_32)
endif()
if(MINGW)
    # Put the MinGW runtime inside the .exe so it also runs outside CLion
    target_link_options(DSA_Project2 PRIVATE -static)
endif()

# Trie tests and benchmark
add_executable(benchmark_trie benchmark_trie.cpp)
add_executable(test_trie test_trie.cpp)

# TST tests and benchmark
add_executable(benchmark_tst benchmark_tst.cpp)
add_executable(test_tst test_tst.cpp)
