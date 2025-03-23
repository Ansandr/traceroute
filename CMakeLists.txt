cmake_minimum_required(VERSION 3.12..3.31)
project("traceroute" CXX)

# Project standarts
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB TRACER_SOURCE_FILES src/*.cpp src/*.hpp)

add_executable(${PROJECT_NAME} src/main.cpp src/main.h)

if (MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()
