# ExamLens build file
# Tells CMake how to turn our C++ code into one runnable program.

# Minimum CMake version needed
cmake_minimum_required(VERSION 3.15)

# Project name + language (CXX = C++)
project(ExamLens LANGUAGES CXX)

# Use modern C++17, and don't silently fall back to something older
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Header files (.h) live in include/
include_directories(include)

# Collect every .cpp inside src/ automatically (new files need no edit here)
file(GLOB_RECURSE SOURCES src/*.cpp)

# Build everything into one program called "examlens"
add_executable(examlens ${SOURCES})
