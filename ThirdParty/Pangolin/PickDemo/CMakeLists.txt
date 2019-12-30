cmake_minimum_required(VERSION 2.8)

project(pick_demo)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -g -Wall  -O3 -march=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall   -O3 -march=native")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_definitions(-DCOMPILEDWITHC11)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(Pangolin_DIR ${PROJECT_SOURCE_DIR}/../install/lib/cmake/Pangolin)
find_package(Pangolin REQUIRED)
message(STATUS "Pangolin Found: "  ${Pangolin_INCLUDE_DIRS})


include_directories(${Pangolin_INCLUDE_DIRS})
add_executable(pick_demo demo.cc)
target_link_libraries(pick_demo ${Pangolin_LIBRARIES} )
