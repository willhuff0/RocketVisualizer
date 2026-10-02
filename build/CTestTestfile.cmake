# CMake generated Testfile for 
# Source directory: /Users/will/dev/Swamplaunch/RocketVisualizer
# Build directory: /Users/will/dev/Swamplaunch/RocketVisualizer/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[ahrs_timeline]=] "/Users/will/dev/Swamplaunch/RocketVisualizer/build/ahrs_timeline_tests")
set_tests_properties([=[ahrs_timeline]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/will/dev/Swamplaunch/RocketVisualizer/CMakeLists.txt;54;add_test;/Users/will/dev/Swamplaunch/RocketVisualizer/CMakeLists.txt;0;")
add_test([=[ahrs_stdin]=] "/opt/homebrew/Frameworks/Python.framework/Versions/3.14/bin/python3.14" "/Users/will/dev/Swamplaunch/RocketVisualizer/tests/stdin_integration.py" "/Users/will/dev/Swamplaunch/RocketVisualizer/build/ahrs_timeline_tests")
set_tests_properties([=[ahrs_stdin]=] PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "/Users/will/dev/Swamplaunch/RocketVisualizer/CMakeLists.txt;56;add_test;/Users/will/dev/Swamplaunch/RocketVisualizer/CMakeLists.txt;0;")
subdirs("_deps/eigen3-build")
subdirs("_deps/raylib-build")
