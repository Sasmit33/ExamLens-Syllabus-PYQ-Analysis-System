cmake_minimum_required(VERSION 3.16)
project(ExamAnalyzerUI VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)


set(BACKEND_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../exam_analyzer/src)
if(NOT EXISTS ${BACKEND_DIR}/engine.h)
    message(FATAL_ERROR "Backend not found at ${BACKEND_DIR}. "
                        "Put ExamAnalyzerUI and exam_analyzer in the same parent folder.")
endif()


file(GLOB BACKEND_SOURCES ${BACKEND_DIR}/*.c)
list(FILTER BACKEND_SOURCES EXCLUDE REGEX ".*/main\\.c$")

qt_add_executable(ExamAnalyzerUI
    main.cpp
    mainwindow.h mainwindow.cpp
    analyzerengine.h analyzerengine.cpp
    prioritychart.h prioritychart.cpp
    graphview.h graphview.cpp
    ${BACKEND_SOURCES}
)

target_include_directories(ExamAnalyzerUI PRIVATE ${BACKEND_DIR})
target_link_libraries(ExamAnalyzerUI PRIVATE Qt6::Widgets)
set_target_properties(ExamAnalyzerUI PROPERTIES WIN32_EXECUTABLE ON)
