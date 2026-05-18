cmake_minimum_required(VERSION 3.20)

project(ray-tracing-lab-iii LANGUAGES CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

list(APPEND CMAKE_PREFIX_PATH "$ENV{USERPROFILE}/vcpkg/installed/x64-windows")

find_package(OpenMP)
find_package(SDL2 CONFIG REQUIRED)

add_subdirectory(3rdParty)

set(ENTITIES_SOURCE_GROUP
    AABB.hpp
    BVHNode.hpp
    BVHLeafNode.hpp
    Entity.hpp
    Renderable.hpp
    Scene.hpp
    Triangle.hpp
    Mesh.hpp
)

set(LIGHTS_SOURCE_GROUP
    Light.hpp
    PointLight.hpp
    DirectionalLight.hpp
)

set(SHADERS_SOURCE_GROUP
    Shader.hpp
    LambertianShader.hpp
    TexturedLambertianShader.hpp
    TexturedPhongShader.hpp
    PhongShader.hpp
    MirrorShader.hpp
    TexCoordTestShader.hpp
)

source_group("Header Files\\Entities" FILES ${ENTITIES_SOURCE_GROUP})
source_group("Header Files\\Lights" FILES ${LIGHTS_SOURCE_GROUP})
source_group("Header Files\\Shaders" FILES ${SHADERS_SOURCE_GROUP})

add_executable(main
    main.cpp

    GeomUtil.hpp

    Ray.hpp
    HitInfo.hpp
    Camera.hpp

    Model.cpp
    Model.hpp

    BitMasks.hpp

    ${ENTITIES_SOURCE_GROUP}
    ${LIGHTS_SOURCE_GROUP}
    ${SHADERS_SOURCE_GROUP}
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(main PUBLIC OpenMP::OpenMP_CXX lodepng SDL2::SDL2 SDL2::SDL2main)
else()
    target_link_libraries(main PUBLIC lodepng SDL2::SDL2 SDL2::SDL2main)
endif()

include_directories(../../3rdParty/eigen-3.4.0)
include_directories(3rdParty/lodepng)
include_directories(../../3rdParty/nlohmann)

