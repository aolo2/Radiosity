cmake_minimum_required(VERSION 3.9)
project(Radiosity)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp main.cpp shader.cpp shader.h radiosity.h radiosity.cpp utils.h utils.cpp shared.h)
add_executable(Radiosity ${SOURCE_FILES})
target_link_libraries(Radiosity GL GLEW glfw)
