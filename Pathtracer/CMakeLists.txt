cmake_minimum_required(VERSION 3.9)
project(Pathtracer)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp file_utils.h file_utils.cpp constants.h camera.h camera.cpp sampler.h ray.h)
add_executable(Pathtracer ${SOURCE_FILES})