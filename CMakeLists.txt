cmake_minimum_required (VERSION 2.8)

project (NetworkCameras)

list(APPEND CMAKE_CXX_FLAGS "-std=c++11")

set(flycapture_path "/home/pi/Projects/VRCam/flycapture.2.9.3.43_armhf")
set (EXECUTABLE_OUTPUT_PATH "bin")

# include FlyCam
include_directories(${flycapture_path}/include)
link_directories(${flycapture_path}/lib)

# flags
SET(GCC_COVERAGE_COMPILE_FLAGS "-g -l pthread")
SET(GCC_COVERAGE_LINK_FLAGS    "")

SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )
SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}" )


file(GLOB SOURCE
    "NetworkCameras/*.h"
    "NetworkCameras/*.cpp"
    ${flycapture_path}/include/FlyCapture2.h
)

add_executable(NetworkCameras ${SOURCE})

target_link_libraries(NetworkCameras libflycapture.so)