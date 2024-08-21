cmake_minimum_required(VERSION 3.29)
project(BookByThanh)

set(CMAKE_CXX_STANDARD 14)

include_directories(Library)

add_executable(BookByThanh
        Library/SOIL.h
        Book.cpp)
