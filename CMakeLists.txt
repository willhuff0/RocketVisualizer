cmake_minimum_required(VERSION 3.25)
project(RocketVisualizer LANGUAGES C CXX)
include(FetchContent)
include(CTest)

option(ROCKET_VISUALIZER_BUILD_APP "Build the raylib visualizer window" ON)

if(NOT TARGET Eigen3::Eigen)
    find_package(Eigen3 3.4 QUIET NO_MODULE)
    if(NOT Eigen3_FOUND)
        set(EIGEN_BUILD_TESTING OFF CACHE BOOL "" FORCE)
        set(EIGEN_BUILD_DOC OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(Eigen3
            GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git GIT_TAG 5.0.1)
        FetchContent_MakeAvailable(Eigen3)
    endif()
endif()

add_library(rocket_timeline
    src/RocketState.cpp src/VectorRocketTimeline.cpp src/LiveRocketTimeline.cpp
    src/CoordinateConverter.cpp src/AhrsTelemetryParser.cpp src/StdinAhrsProducer.cpp)
target_compile_features(rocket_timeline PUBLIC cxx_std_20)
target_include_directories(rocket_timeline PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(rocket_timeline PUBLIC Eigen3::Eigen)

if(ROCKET_VISUALIZER_BUILD_APP)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git GIT_TAG 6.0)
    FetchContent_MakeAvailable(raylib)
    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.92.7)
    FetchContent_MakeAvailable(imgui)
    FetchContent_Declare(rlImGui
        GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git GIT_TAG Raylib_6_0)
    FetchContent_MakeAvailable(rlImGui)

    add_executable(RocketVisualizer
        src/main.cpp src/RocketVisualizer.cpp src/OrbitCamera.cpp
        src/PlaybackController.cpp src/PollingStateProducer.cpp src/CsvTimelineLoader.cpp
        ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp ${rlimgui_SOURCE_DIR}/rlImGui.cpp)
    target_include_directories(RocketVisualizer PRIVATE
        ${imgui_SOURCE_DIR} ${rlimgui_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/third-party)
    target_compile_definitions(RocketVisualizer PRIVATE
        ROCKET_VISUALIZER_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources")
    target_link_libraries(RocketVisualizer PRIVATE rocket_timeline raylib)
endif()

if(BUILD_TESTING)
    add_executable(ahrs_timeline_tests tests/AhrsTimelineTests.cpp)
    target_link_libraries(ahrs_timeline_tests PRIVATE rocket_timeline)
    add_test(NAME ahrs_timeline COMMAND ahrs_timeline_tests)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME ahrs_stdin
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/stdin_integration.py
                $<TARGET_FILE:ahrs_timeline_tests>)
    set_tests_properties(ahrs_stdin PROPERTIES TIMEOUT 15)
    add_test(NAME ahrs_serial
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/serial_integration.py
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/serial_telemetry.py $<TARGET_FILE:ahrs_timeline_tests>)
    set_tests_properties(ahrs_serial PROPERTIES TIMEOUT 20 SKIP_RETURN_CODE 77)
endif()
