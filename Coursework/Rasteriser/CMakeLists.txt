cmake_minimum_required(VERSION 3.27.1)

project(rasteriser)

add_subdirectory(3rdParty)

include_directories(3rdParty/lodepng)
include_directories(../../3rdParty/eigen-3.4.0)

add_executable(rasteriser
    rasteriser.cpp
    Mesh.hpp
    Light.hpp
    LinAlg.hpp
    Image.hpp
    Shading.hpp
    )


target_link_libraries(rasteriser 
    lodepng
    )