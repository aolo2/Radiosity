cmake_minimum_required(VERSION 3.9)
project(Radiosity)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        main.cpp
        shader.cpp
        radiosity.cpp
        utils.cpp
        camera.cpp
        bvh.cpp
        shader.h
        radiosity.h
        utils.h
        camera.h
        bvh.h
        shared.h
        )
add_executable(Radiosity ${SOURCE_FILES})
target_link_libraries(Radiosity GL GLEW glfw)
