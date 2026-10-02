cmake_minimum_required(VERSION 4.3)
include(FetchContent)

project(RocketVisualizer)

set(CMAKE_CXX_STANDARD 20)

add_executable(RocketVisualizer)

# === raylib ===

FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 6.0
)
FetchContent_MakeAvailable(raylib)
target_link_libraries(RocketVisualizer PRIVATE raylib)

# ===

# === imgui ===

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG v1.92.7
)
FetchContent_MakeAvailable(imgui)
set(imgui_SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/imgui-src)
set(imgui_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)

FetchContent_Declare(
        rlImGui
        GIT_REPOSITORY https://github.com/raylib-extras/rlImGui
        GIT_TAG Raylib_6_0
)
FetchContent_MakeAvailable(rlImGui)
set(rlImGui_SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/rlimgui-src)
set(rlImGui_SOURCES
        ${rlImGui_SOURCE_DIR}/rlImGui.cpp
)

target_include_directories(RocketVisualizer
    PRIVATE
        ${imgui_SOURCE_DIR}
        ${rlImGui_SOURCE_DIR}
)

# ===

# === Eigen ===

FetchContent_Declare(
        Eigen3
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG 5.0.1
)
FetchContent_MakeAvailable(Eigen3)
target_link_libraries(RocketVisualizer PRIVATE Eigen3::Eigen)

# ===

# === Header only dependencies

target_include_directories(RocketVisualizer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third-party)

# ===

target_sources(RocketVisualizer
    PRIVATE
        src/main.cpp
        src/RocketVisualizer.cpp
        src/OrbitCamera.cpp
        src/RocketState.cpp
        src/VectorRocketTimeline.cpp
        src/LiveRocketTimeline.cpp
        src/PlaybackController.cpp
        src/PollingStateProducer.cpp
        src/CsvTimelineLoader.cpp
        src/CoordinateConverter.cpp
        ${imgui_SOURCES}
        ${rlImGui_SOURCES}
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/RocketVisualizer.hpp
        include/OrbitCamera.hpp
        include/RocketState.hpp
        include/IRocketTimeline.hpp
        include/VectorRocketTimeline.hpp
        include/LiveRocketTimeline.hpp
        include/PlaybackController.hpp
        include/PollingStateProducer.hpp
        include/CsvTimelineLoader.hpp
        include/CoordinateConverter.hpp
)

target_include_directories(RocketVisualizer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)