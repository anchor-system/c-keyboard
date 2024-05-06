cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(c-keyboard)

set(BUILD_SHARED_LIBS OFF)

#set(RTMIDI_BUILD_STATIC_LIBS "yes")

add_executable(c-keyboard main.cpp)

# RTMIDI: midi library
add_subdirectory(external_libraries/rtmidi)

# GLAD: opengl function loader

include_directories(external_libraries/glad_opengl_3.3_core/include)
add_subdirectory(external_libraries/glad_opengl_3.3_core)

#GLFW: graphics library frame work
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(external_libraries/glfw)

# linmath.h: basic linear algebra
include_directories(external_libraries/linmath.h)

target_link_libraries(c-keyboard rtmidi glfw glad)
