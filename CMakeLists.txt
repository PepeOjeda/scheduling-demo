cmake_minimum_required(VERSION 3.15)
project(scheduling-demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(ament_imgui/imgui_gl)

add_executable(demo src/main.cpp src/Scheduler.cpp)
target_link_libraries(demo imgui_gl)