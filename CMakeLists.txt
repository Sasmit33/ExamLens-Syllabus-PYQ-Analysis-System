cmake_minimum_required(VERSION 3.15)
project(ExamLens LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# every .c and .cpp under src/ is part of the examlens binary
file(GLOB_RECURSE SOURCES src/*.c src/*.cpp)

add_executable(examlens ${SOURCES})
