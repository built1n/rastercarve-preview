cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(rastercarve-preview CXX C)

add_library(core
  ./src/gcode2svg.cpp
  )

add_subdirectory(gpr)
add_subdirectory(libfml)

# Main entry point.
add_executable(rastercarve-preview
  ./src/main.cpp)

target_link_libraries(rastercarve-preview
  core gpr fml)

include_directories(gpr/src libfml/include)

# Add flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -O3")
