cmake_minimum_required(VERSION 3.10)
project(CityDriftGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(GLFW QUIET glfw3)
endif()

if(NOT GLFW_FOUND)
    find_package(glfw3 3.3 QUIET)
    if(glfw3_FOUND)
        set(GLFW_LIBRARIES glfw)
    endif()
endif()

if(NOT GLFW_FOUND AND NOT glfw3_FOUND)
    message(FATAL_ERROR "GLFW3 not found. Install with: brew install glfw")
endif()

add_executable(city_drift_game
    src/main.cpp
    src/game.cpp
    src/car.cpp
    src/city.cpp
    src/camera.cpp
    src/shader.cpp
    src/physics.cpp
    src/hud.cpp
    src/shadows.cpp
    src/postprocess.cpp
    src/particles.cpp
    src/objloader.cpp
    src/carmodel.cpp
)

target_include_directories(city_drift_game PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${OPENGL_INCLUDE_DIR}
)

target_link_libraries(city_drift_game
    ${OPENGL_LIBRARIES}
    ${GLFW_LIBRARIES}
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)
