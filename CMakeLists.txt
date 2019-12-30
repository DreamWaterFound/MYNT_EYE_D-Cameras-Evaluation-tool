cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(Evation_tools)

# ======================== CMake 自身设置 ========================
# 生成模式
set(CMAKE_BUILD_TYPE Release)
# 输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)


# ======================== 编译器标志设置 =========================
# 使能所有警告, 最高等级优化, 使用C++11标准, 针对目标机器CPU自身特点进行针对性优化
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3 -std=c++11 -march=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -std=c++11 -march=native")
# 添加 -fPIC 避免动态链接库和静态链接库在链接的时候可能会出现的问题
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")


# ========================== 搜索依赖项 ==========================
# 搜索Eigen, 这里使用了最简单的方法
include_directories("/home/guoqing/libraries/eigen_333/include/eigen3")

# 搜索OpenCV
set(OpenCV_DIR "/home/guoqing/libraries/opencv_331/share/OpenCV")
find_package(OpenCV 3 REQUIRED)
message(STATUS "==> OpenCV 3: OpenCV_INCLUDE_DIRS = " ${OpenCV_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})
list(APPEND LINK_LIBS ${OpenCV_LIBS})

# 搜索小觅相机驱动
set(mynteyed_DIR "/home/guoqing/libraries/MYNT_EYE_D_180/lib/cmake/mynteyed")
find_package(mynteyed REQUIRED)
message(STATUS "==> Found mynteye: ${mynteyed_VERSION}")
list(APPEND LINK_LIBS mynteye_depth)

if(mynteyed_WITH_OPENCV)
  add_definitions(-DWITH_OPENCV)
  message(STATUS "mynteye with opencv.")
endif()

# ========================== 自带依赖项 ==========================
# 用于支持 GFLAGS
find_package(Threads REQUIRED)
include_directories(${PROJECT_SOURCE_DIR}/ThirdParty/gflags/include)
message("-- ==> USE ThirdParty GFLAGS ${PROJECT_SOURCE_DIR}/Thirdparty/gflags/include")
list(APPEND LINK_LIBS 
    ${PROJECT_SOURCE_DIR}/ThirdParty/gflags/lib/libgflags.a
    ${PROJECT_SOURCE_DIR}/ThirdParty/gflags/lib/libgflags_nothreads.a
    ${CMAKE_THREAD_LIBS_INIT}
)

# GLOG
include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/glog/include)
list(APPEND LINK_LIBS 
    ${CMAKE_SOURCE_DIR}/ThirdParty/glog/lib/libglog.a
)

# Pangolin
set(Pangolin_DIR ${PROJECT_SOURCE_DIR}/ThirdParty/Pangolin/install/lib/cmake/Pangolin)
find_package(Pangolin REQUIRED)
include_directories(${Pangolin_INCLUDE_DIRS})
message(STATUS "Pangolin Found: "  ${Pangolin_INCLUDE_DIRS})
list(APPEND LINK_LIBS ${Pangolin_LIBRARIES})

# indicators
include_directories(${PROJECT_SOURCE_DIR}/Thirdparty/gflags/include)

# 本工程自身的头文件
include_directories(${CMAKE_SOURCE_DIR}/inc)

# ========================== 声明可执行文件 ==========================
add_executable(eva ${CMAKE_SOURCE_DIR}/src/main.cc)
target_link_libraries(eva ${LINK_LIBS})










